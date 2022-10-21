
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "huffman.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define MAKE_LOOKUP(code, bits) (((code) << 5) | ((bits)&0x1f))

struct huffman_decoder *create_huffman_decoder(int numcodes, int maxbits)
{
    if (maxbits > 24)
        return NULL;

    struct huffman_decoder *decoder = (struct huffman_decoder *)malloc(sizeof(struct huffman_decoder));
    decoder->numcodes               = numcodes;
    decoder->maxbits                = maxbits;
    decoder->lookup                 = (lookup_value *)malloc(sizeof(lookup_value) * (1 << maxbits));
    decoder->huffnode               = (struct node_t *)malloc(sizeof(struct node_t) * numcodes);
    decoder->datahisto              = NULL;
    decoder->prevdata               = 0;
    decoder->rleremaining           = 0;
    return decoder;
}

uint32_t huffman_decode_one(struct huffman_decoder *decoder, struct bitstream *bitbuf)
{
    uint32_t bits = bitstream_peek(bitbuf, decoder->maxbits);

    lookup_value lookup = decoder->lookup[bits];
    bitstream_remove(bitbuf, lookup & 0x1f);

    return lookup >> 5;
}

enum huffman_error huffman_import_tree_rle(struct huffman_decoder *decoder, struct bitstream *bitbuf)
{
    int numbits;
    if (decoder->maxbits >= 16)
        numbits = 5;
    else if (decoder->maxbits >= 8)
        numbits = 4;
    else
        numbits = 3;

    int curnode;
    for (curnode = 0; curnode < decoder->numcodes;) {
        int nodebits = bitstream_read(bitbuf, numbits);
        if (nodebits != 1)
            decoder->huffnode[curnode++].numbits = nodebits;

        else {
            nodebits = bitstream_read(bitbuf, numbits);
            if (nodebits == 1)
                decoder->huffnode[curnode++].numbits = nodebits;

            else {
                int repcount = bitstream_read(bitbuf, numbits) + 3;
                while (repcount--)
                    decoder->huffnode[curnode++].numbits = nodebits;
            }
        }
    }

    if (curnode != decoder->numcodes)
        return HUFFERR_INVALID_DATA;

    enum huffman_error error = huffman_assign_canonical_codes(decoder);
    if (error != HUFFERR_NONE)
        return error;

    huffman_build_lookup_table(decoder);

    return bitstream_overflow(bitbuf) ? HUFFERR_INPUT_BUFFER_TOO_SMALL : HUFFERR_NONE;
}

enum huffman_error huffman_import_tree_huffman(struct huffman_decoder *decoder, struct bitstream *bitbuf)
{
    struct huffman_decoder *smallhuff = create_huffman_decoder(24, 6);
    smallhuff->huffnode[0].numbits    = bitstream_read(bitbuf, 3);
    int start                         = bitstream_read(bitbuf, 3) + 1;
    int count                         = 0;
    for (int index = 1; index < 24; index++) {
        if (index < start || count == 7)
            smallhuff->huffnode[index].numbits = 0;
        else {
            count                              = bitstream_read(bitbuf, 3);
            smallhuff->huffnode[index].numbits = (count == 7) ? 0 : count;
        }
    }

    enum huffman_error error = huffman_assign_canonical_codes(smallhuff);
    if (error != HUFFERR_NONE)
        return error;
    huffman_build_lookup_table(smallhuff);

    uint32_t temp        = decoder->numcodes - 9;
    uint8_t  rlefullbits = 0;
    while (temp != 0)
        temp >>= 1, rlefullbits++;

    int last = 0;
    int curcode;
    for (curcode = 0; curcode < decoder->numcodes;) {
        int value = huffman_decode_one(smallhuff, bitbuf);
        if (value != 0)
            decoder->huffnode[curcode++].numbits = last = value - 1;
        else {
            int count = bitstream_read(bitbuf, 3) + 2;
            if (count == 7 + 2)
                count += bitstream_read(bitbuf, rlefullbits);
            for (; count != 0 && curcode < decoder->numcodes; count--)
                decoder->huffnode[curcode++].numbits = last;
        }
    }

    if (curcode != decoder->numcodes)
        return HUFFERR_INVALID_DATA;

    error = huffman_assign_canonical_codes(decoder);
    if (error != HUFFERR_NONE)
        return error;

    huffman_build_lookup_table(decoder);

    return bitstream_overflow(bitbuf) ? HUFFERR_INPUT_BUFFER_TOO_SMALL : HUFFERR_NONE;
}

enum huffman_error huffman_compute_tree_from_histo(struct huffman_decoder *decoder)
{
    uint32_t sdatacount = 0;
    for (int i = 0; i < decoder->numcodes; i++)
        sdatacount += decoder->datahisto[i];

    uint32_t lowerweight = 0;
    uint32_t upperweight = sdatacount * 2;
    while (1) {
        uint32_t curweight  = (upperweight + lowerweight) / 2;
        int      curmaxbits = huffman_build_tree(decoder, sdatacount, curweight);

        if (curmaxbits <= decoder->maxbits) {
            lowerweight = curweight;

            if (curweight == sdatacount || (upperweight - lowerweight) <= 1)
                break;
        } else
            upperweight = curweight;
    }

    return huffman_assign_canonical_codes(decoder);
}

static int huffman_tree_node_compare(const void *item1, const void *item2)
{
    const struct node_t *node1 = *(const struct node_t **)item1;
    const struct node_t *node2 = *(const struct node_t **)item2;
    if (node2->weight != node1->weight)
        return node2->weight - node1->weight;
    if (node2->bits - node1->bits == 0)
        fprintf(stderr, "identical node sort keys, should not happen!\n");
    return (int)node1->bits - (int)node2->bits;
}

int huffman_build_tree(struct huffman_decoder *decoder, uint32_t totaldata, uint32_t totalweight)
{
    struct node_t **list      = (struct node_t **)malloc(sizeof(struct node_t *) * decoder->numcodes * 2);
    int             listitems = 0;
    memset(decoder->huffnode, 0, decoder->numcodes * sizeof(decoder->huffnode[0]));
    for (int curcode = 0; curcode < decoder->numcodes; curcode++)
        if (decoder->datahisto[curcode] != 0) {
            list[listitems++]                = &decoder->huffnode[curcode];
            decoder->huffnode[curcode].count = decoder->datahisto[curcode];
            decoder->huffnode[curcode].bits  = curcode;

            decoder->huffnode[curcode].weight =
                ((uint64_t)decoder->datahisto[curcode]) * ((uint64_t)totalweight) / ((uint64_t)totaldata);
            if (decoder->huffnode[curcode].weight == 0)
                decoder->huffnode[curcode].weight = 1;
        }

#if 0
        fprintf(stderr, "Pre-sort:\n");
        for (int i = 0; i < listitems; i++) {
            fprintf(stderr, "weight: %d code: %d\n", list[i]->m_weight, list[i]->m_bits);
        }
#endif

    qsort(&list[0], listitems, sizeof(list[0]), huffman_tree_node_compare);

#if 0
        fprintf(stderr, "Post-sort:\n");
        for (int i = 0; i < listitems; i++) {
            fprintf(stderr, "weight: %d code: %d\n", list[i]->m_weight, list[i]->m_bits);
        }
        fprintf(stderr, "===================\n");
#endif

    int nextalloc = decoder->numcodes;
    while (listitems > 1) {
        struct node_t *node1 = &(*list[--listitems]);
        struct node_t *node0 = &(*list[--listitems]);

        struct node_t *newnode = &decoder->huffnode[nextalloc++];
        newnode->parent        = NULL;
        node0->parent = node1->parent = newnode;
        newnode->weight               = node0->weight + node1->weight;

        int curitem;
        for (curitem = 0; curitem < listitems; curitem++)
            if (newnode->weight > list[curitem]->weight) {
                memmove(&list[curitem + 1], &list[curitem], (listitems - curitem) * sizeof(list[0]));
                break;
            }
        list[curitem] = newnode;
        listitems++;
    }

    int maxbits = 0;
    for (int curcode = 0; curcode < decoder->numcodes; curcode++) {
        struct node_t *node = &decoder->huffnode[curcode];
        node->numbits       = 0;
        node->bits          = 0;

        if (node->weight > 0) {
            for (struct node_t *curnode = node; curnode->parent != NULL; curnode = curnode->parent)
                node->numbits++;
            if (node->numbits == 0)
                node->numbits = 1;

            maxbits = MAX(maxbits, ((int)node->numbits));
        }
    }
    return maxbits;
}

enum huffman_error huffman_assign_canonical_codes(struct huffman_decoder *decoder)
{
    uint32_t bithisto[33] = {0};
    for (int curcode = 0; curcode < decoder->numcodes; curcode++) {
        struct node_t *node = &decoder->huffnode[curcode];
        if (node->numbits > decoder->maxbits)
            return HUFFERR_INTERNAL_INCONSISTENCY;
        if (node->numbits <= 32)
            bithisto[node->numbits]++;
    }

    uint32_t curstart = 0;
    for (int codelen = 32; codelen > 0; codelen--) {
        uint32_t nextstart = (curstart + bithisto[codelen]) >> 1;
        if (codelen != 1 && nextstart * 2 != (curstart + bithisto[codelen]))
            return HUFFERR_INTERNAL_INCONSISTENCY;
        bithisto[codelen] = curstart;
        curstart          = nextstart;
    }

    for (int curcode = 0; curcode < decoder->numcodes; curcode++) {
        struct node_t *node = &decoder->huffnode[curcode];
        if (node->numbits > 0)
            node->bits = bithisto[node->numbits]++;
    }
    return HUFFERR_NONE;
}

void huffman_build_lookup_table(struct huffman_decoder *decoder)
{
    for (int curcode = 0; curcode < decoder->numcodes; curcode++) {
        struct node_t *node = &decoder->huffnode[curcode];
        if (node->numbits > 0) {
            lookup_value value = MAKE_LOOKUP(curcode, node->numbits);

            int           shift   = decoder->maxbits - node->numbits;
            lookup_value *dest    = &decoder->lookup[node->bits << shift];
            lookup_value *destend = &decoder->lookup[((node->bits + 1) << shift) - 1];
            while (dest <= destend)
                *dest++ = value;
        }
    }
}
