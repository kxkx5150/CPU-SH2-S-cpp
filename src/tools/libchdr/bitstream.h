
#pragma once

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>

struct bitstream
{
    uint32_t       buffer;
    int            bits;
    const uint8_t *read;
    uint32_t       doffset;
    uint32_t       dlength;
};

struct bitstream *create_bitstream(const void *src, uint32_t srclength);
int               bitstream_overflow(struct bitstream *bitstream);
uint32_t          bitstream_read_offset(struct bitstream *bitstream);

uint32_t bitstream_read(struct bitstream *bitstream, int numbits);
uint32_t bitstream_peek(struct bitstream *bitstream, int numbits);
void     bitstream_remove(struct bitstream *bitstream, int numbits);
uint32_t bitstream_flush(struct bitstream *bitstream);

#endif
