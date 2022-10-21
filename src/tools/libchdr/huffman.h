
#pragma once

#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include "bitstream.h"

enum huffman_error
{
    HUFFERR_NONE = 0,
    HUFFERR_TOO_MANY_BITS,
    HUFFERR_INVALID_DATA,
    HUFFERR_INPUT_BUFFER_TOO_SMALL,
    HUFFERR_OUTPUT_BUFFER_TOO_SMALL,
    HUFFERR_INTERNAL_INCONSISTENCY,
    HUFFERR_TOO_MANY_CONTEXTS
};

typedef uint16_t lookup_value;

struct node_t
{
    struct node_t *parent;
    uint32_t       count;
    uint32_t       weight;
    uint32_t       bits;
    uint8_t        numbits;
};

struct huffman_decoder
{
    uint32_t       numcodes;
    uint8_t        maxbits;
    uint8_t        prevdata;
    int            rleremaining;
    lookup_value  *lookup;
    struct node_t *huffnode;
    uint32_t      *datahisto;

#if 0
	node_t*			huffnode_array;
	lookup_value*	lookup_array;
#endif
};

struct huffman_decoder *create_huffman_decoder(int numcodes, int maxbits);

uint32_t huffman_decode_one(struct huffman_decoder *decoder, struct bitstream *bitbuf);

enum huffman_error huffman_import_tree_rle(struct huffman_decoder *decoder, struct bitstream *bitbuf);
enum huffman_error huffman_import_tree_huffman(struct huffman_decoder *decoder, struct bitstream *bitbuf);

int                huffman_build_tree(struct huffman_decoder *decoder, uint32_t totaldata, uint32_t totalweight);
enum huffman_error huffman_assign_canonical_codes(struct huffman_decoder *decoder);
enum huffman_error huffman_compute_tree_from_histo(struct huffman_decoder *decoder);

void huffman_build_lookup_table(struct huffman_decoder *decoder);

#endif
