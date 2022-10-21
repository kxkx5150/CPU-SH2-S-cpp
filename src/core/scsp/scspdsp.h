
#ifndef SCSPDSP_H
#define SCSPDSP_H

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    u16 coef[64];
    u16 madrs[32];
    u64 mpro[128];
    s32 temp[128];
    s32 mems[32];
    s32 mixs[16];
    s16 efreg[16];
    s16 exts[2];

    u32 mdec_ct;
    s32 inputs;
    s32 b;
    s32 x;
    s16 y;
    s32 acc;
    s32 shifted;
    s32 y_reg;
    u16 frc_reg;
    u16 adrs_reg;

    s32 mul_out;

    u32 mrd_value;

    int rbl;
    int rbp;

    int need_read;
    int need_nofl;
    u32 io_addr;
    int need_write;
    u16 write_data;
    int updated;
    int last_step;

    s64 product;
    u32 read_value;
    u32 write_value;
    int read_pending;
    int write_pending;
    u32 shift_reg;

} ScspDsp;

#ifdef WORDS_BIGENDIAN
union ScspDspInstruction
{
    struct
    {
        u64 unknown : 1;
        u64 tra : 7;
        u64 twt : 1;
        u64 twa : 7;

        u64 xsel : 1;
        u64 ysel : 2;
        u64 unknown2 : 1;
        u64 ira : 6;
        u64 iwt : 1;
        u64 iwa : 5;

        u64 table : 1;
        u64 mwt : 1;
        u64 mrd : 1;
        u64 ewt : 1;
        u64 ewa : 4;
        u64 adrl : 1;
        u64 frcl : 1;
        u64 shift : 2;
        u64 yrl : 1;
        u64 negb : 1;
        u64 zero : 1;
        u64 bsel : 1;

        u64 nofl : 1;
        u64 coef : 6;
        u64 unknown3 : 2;
        u64 masa : 5;
        u64 adreb : 1;
        u64 nxadr : 1;
    } part;
    u32 all;
};
#else
union ScspDspInstruction
{
    struct
    {
        u64 nxadr : 1;
        u64 adreb : 1;
        u64 masa : 5;
        u64 unknown3 : 2;
        u64 coef : 6;
        u64 nofl : 1;
        u64 bsel : 1;
        u64 zero : 1;
        u64 negb : 1;
        u64 yrl : 1;
        u64 shift0 : 1;
        u64 shift1 : 1;
#if 0
      u64 shift : 2;
#endif
        u64 frcl : 1;
        u64 adrl : 1;
        u64 ewa : 4;
        u64 ewt : 1;
        u64 mrd : 1;
        u64 mwt : 1;
        u64 table : 1;
        u64 iwa : 5;
        u64 iwt : 1;
        u64 ira : 6;
        u64 unknown2 : 1;
        u64 ysel : 2;
        u64 xsel : 1;
        u64 twa : 7;
        u64 twt : 1;
        u64 tra : 7;
        u64 unknown : 1;
    } part;
    u64 all;
};
#endif

void ScspDspDisasm(u8 addr, char *outstring);
void ScspDspExec(ScspDsp *dsp, int addr, u8 *sound_ram);

extern ScspDsp scsp_dsp;

#ifdef __cplusplus
}
#endif

#endif
