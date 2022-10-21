
#ifndef VDP2_H
#define VDP2_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8 *Vdp2Ram;
extern u8 *Vdp2ColorRam;
extern u8  Vdp2ColorRamUpdated[];
extern u8  Vdp2ColorRamToSync[];
extern u8  A0_Updated;
extern u8  A1_Updated;
extern u8  B0_Updated;
extern u8  B1_Updated;

u8 FASTCALL   Vdp2RamReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp2RamReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp2RamReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp2RamWriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp2RamWriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp2RamWriteLong(SH2_struct *context, u8 *, u32, u32);

u8 FASTCALL   Vdp2ColorRamReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp2ColorRamReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp2ColorRamReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp2ColorRamWriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp2ColorRamWriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp2ColorRamWriteLong(SH2_struct *context, u8 *, u32, u32);

typedef struct
{
    u16 TVMD;
    u16 EXTEN;
    u16 TVSTAT;
    u16 VRSIZE;
    u16 HCNT;
    u16 VCNT;
    u16 RAMCTL;
    u16 CYCA0L;
    u16 CYCA0U;
    u16 CYCA1L;
    u16 CYCA1U;
    u16 CYCB0L;
    u16 CYCB0U;
    u16 CYCB1L;
    u16 CYCB1U;
    u16 BGON;
    u16 MZCTL;
    u16 SFSEL;
    u16 SFCODE;
    u16 CHCTLA;
    u16 CHCTLB;
    u16 BMPNA;
    u16 BMPNB;
    u16 PNCN0;
    u16 PNCN1;
    u16 PNCN2;
    u16 PNCN3;
    u16 PNCR;
    u16 PLSZ;
    u16 MPOFN;
    u16 MPOFR;
    u16 MPABN0;
    u16 MPCDN0;
    u16 MPABN1;
    u16 MPCDN1;
    u16 MPABN2;
    u16 MPCDN2;
    u16 MPABN3;
    u16 MPCDN3;
    u16 MPABRA;
    u16 MPCDRA;
    u16 MPEFRA;
    u16 MPGHRA;
    u16 MPIJRA;
    u16 MPKLRA;
    u16 MPMNRA;
    u16 MPOPRA;
    u16 MPABRB;
    u16 MPCDRB;
    u16 MPEFRB;
    u16 MPGHRB;
    u16 MPIJRB;
    u16 MPKLRB;
    u16 MPMNRB;
    u16 MPOPRB;
    u16 SCXIN0;
    u16 SCXDN0;
    u16 SCYIN0;
    u16 SCYDN0;

#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 I : 16;
            u32 D : 16;
        } part;
        u32 all;
    } ZMXN0;

    union
    {
        struct
        {
            u32 I : 16;
            u32 D : 16;
        } part;
        u32 all;
    } ZMYN0;
#else
    union
    {
        struct
        {
            u32 D : 16;
            u32 I : 16;
        } part;
        u32 all;
    } ZMXN0;

    union
    {
        struct
        {
            u32 D : 16;
            u32 I : 16;
        } part;
        u32 all;
    } ZMYN0;
#endif

    u16 SCXIN1;
    u16 SCXDN1;
    u16 SCYIN1;
    u16 SCYDN1;

#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 I : 16;
            u32 D : 16;
        } part;
        u32 all;
    } ZMXN1;

    union
    {
        struct
        {
            u32 I : 16;
            u32 D : 16;
        } part;
        u32 all;
    } ZMYN1;
#else
    union
    {
        struct
        {
            u32 D : 16;
            u32 I : 16;
        } part;
        u32 all;
    } ZMXN1;

    union
    {
        struct
        {
            u32 D : 16;
            u32 I : 16;
        } part;
        u32 all;
    } ZMYN1;
#endif

    u16 SCXN2;
    u16 SCYN2;
    u16 SCXN3;
    u16 SCYN3;
    u16 ZMCTL;
    u16 SCRCTL;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } VCSTA;

    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } LSTA0;

    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } LSTA1;

    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } LCTA;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } VCSTA;

    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } LSTA0;

    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } LSTA1;

    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } LCTA;
#endif

    u16 BKTAU;
    u16 BKTAL;
    u16 RPMD;
    u16 RPRCTL;
    u16 KTCTL;
    u16 KTAOF;
    u16 OVPNRA;
    u16 OVPNRB;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } RPTA;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } RPTA;
#endif
    u16 WPSX0;
    u16 WPSY0;
    u16 WPEX0;
    u16 WPEY0;
    u16 WPSX1;
    u16 WPSY1;
    u16 WPEX1;
    u16 WPEY1;
    u16 WCTLA;
    u16 WCTLB;
    u16 WCTLC;
    u16 WCTLD;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } LWTA0;

    union
    {
        struct
        {
            u32 U : 16;
            u32 L : 16;
        } part;
        u32 all;
    } LWTA1;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } LWTA0;

    union
    {
        struct
        {
            u32 L : 16;
            u32 U : 16;
        } part;
        u32 all;
    } LWTA1;
#endif

    u16 SPCTL;
    u16 SDCTL;
    u16 CRAOFA;
    u16 CRAOFB;
    u16 LNCLEN;
    u16 SFPRMD;
    u16 CCCTL;
    u16 SFCCMD;
    u16 PRISA;
    u16 PRISB;
    u16 PRISC;
    u16 PRISD;
    u16 PRINA;
    u16 PRINB;
    u16 PRIR;
    u16 CCRSA;
    u16 CCRSB;
    u16 CCRSC;
    u16 CCRSD;
    u16 CCRNA;
    u16 CCRNB;
    u16 CCRR;
    u16 CCRLB;
    u16 CLOFEN;
    u16 CLOFSL;
    u16 COAR;
    u16 COAG;
    u16 COAB;
    u16 COBR;
    u16 COBG;
    u16 COBB;
} Vdp2;

extern Vdp2 *Vdp2Regs;

typedef struct
{
    int ColorMode;
} Vdp2Internal_struct;

extern Vdp2Internal_struct Vdp2Internal;
extern int                 vdp2_is_odd_frame;
extern Vdp2                Vdp2Lines[270];

struct CellScrollData
{
    u32 data[88];
};

extern struct CellScrollData cell_scroll_data[270];

typedef struct
{
    int disptoggle;
    int cpu_cycle_a;
    int cpu_cycle_b;
    u8  AC_VRAM[4][8];
    int frame_render_flg;
} Vdp2External_struct;

extern Vdp2External_struct Vdp2External;

int  Vdp2Init(void);
void Vdp2DeInit(void);
void Vdp2Reset(void);
void Vdp2VBlankIN(void);
void Vdp2HBlankIN(void);
void Vdp2HBlankOUT(void);
void Vdp2VBlankOUT(void);
void Vdp2SendExternalLatch(int hcnt, int vcnt);
void SpeedThrottleEnable(void);
void SpeedThrottleDisable(void);

u8 Vdp2RamIsUpdated(void);

u8 FASTCALL   Vdp2ReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp2ReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp2ReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp2WriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp2WriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp2WriteLong(SH2_struct *context, u8 *, u32, u32);

int Vdp2SaveState(void **stream);
int Vdp2LoadState(const void *stream, int version, int size);

void ToggleNBG0(void);
void ToggleNBG1(void);
void ToggleNBG2(void);
void ToggleNBG3(void);
void ToggleRBG0(void);
void ToggleRBG1(void);
void ToggleFullScreen(void);

Vdp2 *Vdp2RestoreRegs(int line, Vdp2 *lines);

#include "threads.h"

int  VideoSetFilterType(int video_filter_type);
void vdp2ReqDump();
void vdp2ReqRestore();

#ifdef __cplusplus
}
#endif

#endif
