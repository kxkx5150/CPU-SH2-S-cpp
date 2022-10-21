
#pragma once

#ifndef __CDROM_H__
#define __CDROM_H__

#include <stdint.h>

extern const uint32_t CD_TRACK_PADDING;

#define CD_MAX_TRACKS       (99)
#define CD_MAX_SECTOR_DATA  (2352)
#define CD_MAX_SUBCODE_DATA (96)

#define CD_FRAME_SIZE      (CD_MAX_SECTOR_DATA + CD_MAX_SUBCODE_DATA)
#define CD_FRAMES_PER_HUNK (8)

#define CD_METADATA_WORDS (1 + (CD_MAX_TRACKS * 6))

enum
{
    CD_TRACK_MODE1 = 0,
    CD_TRACK_MODE1_RAW,
    CD_TRACK_MODE2,
    CD_TRACK_MODE2_FORM1,
    CD_TRACK_MODE2_FORM2,
    CD_TRACK_MODE2_FORM_MIX,
    CD_TRACK_MODE2_RAW,
    CD_TRACK_AUDIO,

    CD_TRACK_RAW_DONTCARE
};

enum
{
    CD_SUB_NORMAL = 0,
    CD_SUB_RAW,
    CD_SUB_NONE
};

#define CD_FLAG_GDROM   0x00000001
#define CD_FLAG_GDROMLE 0x00000002

int  ecc_verify(const uint8_t *sector);
void ecc_generate(uint8_t *sector);
void ecc_clear(uint8_t *sector);

static inline uint32_t msf_to_lba(uint32_t msf)
{
    return (((msf & 0x00ff0000) >> 16) * 60 * 75) + (((msf & 0x0000ff00) >> 8) * 75) + ((msf & 0x000000ff) >> 0);
}

static inline uint32_t lba_to_msf(uint32_t lba)
{
    uint8_t m, s, f;

    m = lba / (60 * 75);
    lba -= m * (60 * 75);
    s = lba / 75;
    f = lba % 75;

    return ((m / 10) << 20) | ((m % 10) << 16) | ((s / 10) << 12) | ((s % 10) << 8) | ((f / 10) << 4) | ((f % 10) << 0);
}

static inline uint32_t lba_to_msf_alt(int lba)
{
    uint32_t ret = 0;

    ret |= ((lba / (60 * 75)) & 0xff) << 16;
    ret |= (((lba / 75) % 60) & 0xff) << 8;
    ret |= ((lba % 75) & 0xff) << 0;

    return ret;
}

#endif
