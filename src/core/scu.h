
#ifndef SCU_H
#define SCU_H

#include "core.h"
#include "sh2core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    u32 addr;
} scucodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct
{
    u8  vector;
    u8  level;
    u16 mask;
    u32 statusbit;
} scuinterrupt_struct;

typedef struct
{
    int mode;
    u32 ReadAddress;
    u32 WriteAddress;
    s32 TransferNumber;
    u32 AddValue;
    u32 ModeAddressUpdate;
    u32 ReadAdd;
    u32 WriteAdd;
    u32 InDirectAdress;
} scudmainfo_struct;

typedef struct
{
    u32 D0R;
    u32 D0W;
    u32 D0C;
    u32 D0AD;
    u32 D0EN;
    u32 D0MD;

    u32 D1R;
    u32 D1W;
    u32 D1C;
    u32 D1AD;
    u32 D1EN;
    u32 D1MD;

    u32 D2R;
    u32 D2W;
    u32 D2C;
    u32 D2AD;
    u32 D2EN;
    u32 D2MD;

    u32 DSTP;
    u32 DSTA;

    u32 PPAF;
    u32 PPD;
    u32 PDA;
    u32 PDD;

    u32 T0C;
    u32 T1S;
    u32 T1MD;

    u32 IMS;
    u32 IST;

    u32 AIACK;
    u32 ASR0;
    u32 ASR1;
    u32 AREF;

    u32 RSEL;
    u32 VER;

    u32                 timer0;
    u32                 timer1;
    scuinterrupt_struct interrupts[30];
    u32                 NumberOfInterrupts;
    s32                 timer1_counter;
    u32                 timer0_set;
    u32                 timer1_set;
    s32                 timer1_preset;

    s32 dma0_time;
    s32 dma1_time;
    s32 dma2_time;

    s32 ITEdge;

    scudmainfo_struct dma0;
    scudmainfo_struct dma1;
    scudmainfo_struct dma2;

} Scu;

extern Scu *ScuRegs;

typedef struct
{
    scucodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
    int                      numcodebreakpoints;
    void (*BreakpointCallBack)(u32);
    u8 inbreakpoint;
} scubp_struct;

typedef struct
{
    u32 ProgramRam[256];
    u32 MD[4][64];
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 unused1 : 5;
            u32 PR : 1;
            u32 EP : 1;
            u32 unused2 : 1;
            u32 T0 : 1;
            u32 S : 1;
            u32 Z : 1;
            u32 C : 1;
            u32 V : 1;
            u32 E : 1;
            u32 ES : 1;
            u32 EX : 1;
            u32 LE : 1;
            u32 unused3 : 7;
            u32 P : 8;
        } part;
        u32 all;
    } ProgControlPort;
#else
    union
    {
        struct
        {
            u32 P : 8;
            u32 unused3 : 7;
            u32 LE : 1;
            u32 EX : 1;
            u32 ES : 1;
            u32 E : 1;
            u32 V : 1;
            u32 C : 1;
            u32 Z : 1;
            u32 S : 1;
            u32 T0 : 1;
            u32 unused2 : 1;
            u32 EP : 1;
            u32 PR : 1;
            u32 unused1 : 5;
        } part;
        u32 all;
    } ProgControlPort;
#endif
    u8  PC;
    u8  TOP;
    u16 LOP;
    s32 jmpaddr;
    int delayed;
    u8  DataRamPage;
    u8  DataRamReadAddress;
    u8  CT[4];
    s32 RX;
    s32 RY;
    u32 RA0;
    u32 WA0;

#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            s64 unused : 16;
            s64 H : 16;
            s64 L : 32;
        } part;
        s64 all;
    } AC;

    union
    {
        struct
        {
            s64 unused : 16;
            s64 H : 16;
            s64 L : 32;
        } part;
        s64 all;
    } P;

    union
    {
        struct
        {
            s64 unused : 16;
            s64 H : 16;
            s64 L : 32;
        } part;
        s64 all;
    } ALU;

    union
    {
        struct
        {
            s64 unused : 16;
            s64 H : 16;
            s64 L : 32;
        } part;
        s64 all;
    } MUL;
#else
    union
    {
        struct
        {
            s64 L : 32;
            s64 H : 16;
            s64 unused : 16;
        } part;
        s64 all;
    } AC;

    union
    {
        struct
        {
            s64 L : 32;
            s64 H : 16;
            s64 unused : 16;
        } part;
        s64 all;
    } P;

    union
    {
        struct
        {
            s64 L : 32;
            s64 H : 16;
            s64 unused : 16;
        } part;
        s64 all;
    } ALU;

    union
    {
        struct
        {
            s64 L : 32;
            s64 H : 16;
            s64 unused : 16;
        } part;
        s64 all;
    } MUL;
#endif
    u32 dsp_dma_instruction;
    s32 dsp_dma_wait;
    u32 dsp_dma_size;
    u32 WA0M;
    u32 RA0M;
    u32 dmy;
} scudspregs_struct;

int  ScuInit(void);
void ScuDeInit(void);
void ScuReset(void);
void ScuExec(u32 timing);

u8 FASTCALL   ScuReadByte(SH2_struct *sh, u8 *, u32);
u16 FASTCALL  ScuReadWord(SH2_struct *sh, u8 *, u32);
u32 FASTCALL  ScuReadLong(SH2_struct *sh, u8 *, u32);
void FASTCALL ScuWriteByte(SH2_struct *sh, u8 *, u32, u8);
void FASTCALL ScuWriteWord(SH2_struct *sh, u8 *, u32, u16);
void FASTCALL ScuWriteLong(SH2_struct *sh, u8 *, u32, u32);

void ScuSendVBlankIN(void);
void ScuSendVBlankOUT(void);
void ScuSendHBlankIN(void);
void ScuSendTimer0(void);
void ScuSendTimer1(void);
void ScuSendDSPEnd(void);
void ScuSendSoundRequest(void);
void ScuSendSystemManager(void);
void ScuSendPadInterrupt(void);
void ScuSendLevel2DMAEnd(void);
void ScuSendLevel1DMAEnd(void);
void ScuSendLevel0DMAEnd(void);
void ScuSendDMAIllegal(void);
void ScuSendDrawEnd(void);
void ScuSendExternalInterrupt00(void);
void ScuSendExternalInterrupt01(void);
void ScuSendExternalInterrupt02(void);
void ScuSendExternalInterrupt03(void);
void ScuSendExternalInterrupt04(void);
void ScuSendExternalInterrupt05(void);
void ScuSendExternalInterrupt06(void);
void ScuSendExternalInterrupt07(void);
void ScuSendExternalInterrupt08(void);
void ScuSendExternalInterrupt09(void);
void ScuSendExternalInterrupt10(void);
void ScuSendExternalInterrupt11(void);
void ScuSendExternalInterrupt12(void);
void ScuSendExternalInterrupt13(void);
void ScuSendExternalInterrupt14(void);
void ScuSendExternalInterrupt15(void);

void                      ScuDspDisasm(u8 addr, char *outstring);
void                      ScuDspStep(void);
int                       ScuDspSaveProgram(const char *filename);
int                       ScuDspSaveMD(const char *filename, int num);
void                      ScuDspGetRegisters(scudspregs_struct *regs);
void                      ScuDspSetRegisters(scudspregs_struct *regs);
void                      ScuDspSetBreakpointCallBack(void (*func)(u32));
int                       ScuDspAddCodeBreakpoint(u32 addr);
int                       ScuDspDelCodeBreakpoint(u32 addr);
scucodebreakpoint_struct *ScuDspGetBreakpointList(void);
void                      ScuDspClearCodeBreakpoints(void);
int                       ScuSaveState(void **stream);
int                       ScuLoadState(const void *stream, int version, int size);

#ifdef __cplusplus
}
#endif

#endif
