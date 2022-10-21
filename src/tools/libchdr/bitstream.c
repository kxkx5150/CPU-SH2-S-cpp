
#include "bitstream.h"
#include <stdlib.h>

int bitstream_overflow(struct bitstream *bitstream)
{
    return ((bitstream->doffset - bitstream->bits / 8) > bitstream->dlength);
}

struct bitstream *create_bitstream(const void *src, uint32_t srclength)
{
    struct bitstream *bitstream = (struct bitstream *)malloc(sizeof(struct bitstream));
    bitstream->buffer           = 0;
    bitstream->bits             = 0;
    bitstream->read             = (const uint8_t *)src;
    bitstream->doffset          = 0;
    bitstream->dlength          = srclength;
    return bitstream;
}

uint32_t bitstream_peek(struct bitstream *bitstream, int numbits)
{
    if (numbits == 0)
        return 0;

    if (numbits > bitstream->bits) {
        while (bitstream->bits <= 24) {
            if (bitstream->doffset < bitstream->dlength)
                bitstream->buffer |= bitstream->read[bitstream->doffset] << (24 - bitstream->bits);
            bitstream->doffset++;
            bitstream->bits += 8;
        }
    }

    return bitstream->buffer >> (32 - numbits);
}

void bitstream_remove(struct bitstream *bitstream, int numbits)
{
    bitstream->buffer <<= numbits;
    bitstream->bits -= numbits;
}

uint32_t bitstream_read(struct bitstream *bitstream, int numbits)
{
    uint32_t result = bitstream_peek(bitstream, numbits);
    bitstream_remove(bitstream, numbits);
    return result;
}

uint32_t bitstream_read_offset(struct bitstream *bitstream)
{
    uint32_t result = bitstream->doffset;
    int      bits   = bitstream->bits;
    while (bits >= 8) {
        result--;
        bits -= 8;
    }
    return result;
}

uint32_t bitstream_flush(struct bitstream *bitstream)
{
    while (bitstream->bits >= 8) {
        bitstream->doffset--;
        bitstream->bits -= 8;
    }
    bitstream->bits = bitstream->buffer = 0;
    return bitstream->doffset;
}
