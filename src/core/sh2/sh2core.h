#ifndef SH2CORE_H
#define SH2CORE_H

#include "core.h"
#include "threads.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SH2CORE_DEFAULT -1
#define MAX_INTERRUPTS  50

#ifdef MACH
#undef MACH
#endif

#define MAX_DMPHISTORY (512)

#define BBR_CPA_NONE (0 << 6)
#define BBR_CPA_CPU  (1 << 6)
#define BBR_CPA_PER  (2 << 6)

#define BBR_IDA_NONE (0 << 4)
#define BBR_IDA_INST (1 << 4)
#define BBR_IDA_DATA (2 << 4)

#define BBR_RWA_NONE  (0 << 2)
#define BBR_RWA_READ  (1 << 2)
#define BBR_RWA_WRITE (2 << 2)

#define BBR_SZA_NONE     (0 << 0)
#define BBR_SZA_BYTE     (1 << 0)
#define BBR_SZA_WORD     (2 << 0)
#define BBR_SZA_LONGWORD (3 << 0)

#define BRCR_CMFCA (1 << 15)
#define BRCR_CMFPA (1 << 14)
#define BRCR_EBBA  (1 << 13)
#define BRCR_UMD   (1 << 12)
#define BRCR_PCBA  (1 << 10)

#define BRCR_CMFCB (1 << 7)
#define BRCR_CMFPB (1 << 6)
#define BRCR_SEQ   (1 << 4)
#define BRCR_DBEB  (1 << 3)
#define BRCR_PCBB  (1 << 2)

typedef struct
{
    u32 R[16];

#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 reserved1 : 22;
            u32 M : 1;
            u32 Q : 1;
            u32 I : 4;
            u32 reserved0 : 2;
            u32 S : 1;
            u32 T : 1;
        } part;
        u32 all;
    } SR;
#else
    union
    {
        struct
        {
            u32 T : 1;
            u32 S : 1;
            u32 reserved0 : 2;
            u32 I : 4;
            u32 Q : 1;
            u32 M : 1;
            u32 reserved1 : 22;
        } part;
        u32 all;
    } SR;
#endif

    u32 GBR;
    u32 VBR;
    u32 MACH;
    u32 MACL;
    u32 PR;
    u32 PC;
} sh2regs_struct;

typedef struct
{
    u8 SMR;
    u8 BRR;
    u8 SCR;
    u8 TDR;
    u8 SSR;
    u8 RDR;
    u8 TIER;
    u8 FTCSR;

#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u16 H : 8;
            u16 L : 8;
        } part;
        u16 all;
    } FRC;
#else
    union
    {
        struct
        {
            u16 L : 8;
            u16 H : 8;
        } part;
        u16 all;
    } FRC;
#endif
    u16 OCRA;
    u16 OCRB;
    u8  TCR;
    u8  TOCR;
    u16 FICR;
    u16 IPRB;
    u16 VCRA;
    u16 VCRB;
    u16 VCRC;
    u16 VCRD;
    u8  DRCR0;
    u8  DRCR1;
    u8  WTCSR;
    u8  WTCNT;
    u8  RSTCSR;
    u8  SBYCR;
    u8  CCR;
    u16 ICR;
    u16 IPRA;
    u16 VCRWDT;
    u32 DVSR;
    u32 DVDNT;
    u32 DVCR;
    u32 VCRDIV;
    u32 DVDNTH;
    u32 DVDNTL;
    u32 DVDNTUH;
    u32 DVDNTUL;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BARA;

    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BAMRA;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BARA;

    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BAMRA;
#endif
    u32 BBRA;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BARB;

    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BAMRB;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BARB;

    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BAMRB;
#endif
    u32 BBRB;
#ifdef WORDS_BIGENDIAN
    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BDRB;

    union
    {
        struct
        {
            u32 H : 16;
            u32 L : 16;
        } part;
        u16 all;
    } BDMRB;
#else
    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BDRB;

    union
    {
        struct
        {
            u32 L : 16;
            u32 H : 16;
        } part;
        u16 all;
    } BDMRB;
#endif
    u32 BRCR;
    u32 SAR0;
    u32 DAR0;
    u32 TCR0;
    u32 CHCR0;
    u32 SAR1;
    u32 DAR1;
    u32 TCR1;
    u32 CHCR1;
    u32 CHCR1M;
    u32 VCRDMA0;
    u32 VCRDMA1;
    u32 DMAOR;
    u16 BCR1;
    u16 BCR2;
    u16 WCR;
    u16 MCR;
    u16 RTCSR;
    u16 RTCNT;
    u16 RTCOR;
} Onchip_struct;

typedef struct
{
    u8 vector;
    u8 level;
} interrupt_struct;

enum SH2STEPTYPE
{
    SH2ST_STEPOVER,
    SH2ST_STEPOUT
};
typedef struct
{
    u32 *CHCR;
    u32 *SAR;
    u32 *DAR;
    u32 *TCR;
    u32 *CHCRM;
    u32 *VCRDMA;
    int  copy_clock;
} Dmac;
typedef struct
{
    u32 addr;
    u64 count;
} tilInfo_struct;
typedef struct
{
    u32 addr[256];
    int numbacktrace;
} backtrace_struct;

typedef struct SH2_struct_s
{
    sh2regs_struct regs;
    Onchip_struct  onchip;

    struct
    {
        u32 leftover;
        u32 shift;
    } frc;

    struct
    {
        int isenable;
        int isinterval;
        u32 leftover;
        u32 shift;
    } wdt;

    interrupt_struct interrupts[MAX_INTERRUPTS];
    u32              NumberOfInterrupts;
    u32              AddressArray[0x100];
    u8               DataArray[0x1000];
    u32              delay;
    u32              cycles;
    u8               isslave;
    u8               isSleeping;
    u16              instruction;
    int              depth;

#ifdef DMPHISTORY
    u32            pchistory[MAX_DMPHISTORY];
    sh2regs_struct regshistory[MAX_DMPHISTORY];
    u32            pchistory_index;
#endif

    void *ext;

    u8  cacheOn;
    u8  nbCacheWay;
    u8  cacheLRU[64];
    u8  cacheData[64][4][16];
    u8  tagWay[64][0x80000];
    u32 cacheTagArray[64][4];
    u32 cycleFrac;
    u32 cycleLost;
    int cdiff;
    int trace;
    u32 frtcycles;
    u32 wdtcycles;

    Dmac dma_ch0;
    Dmac dma_ch1;

    backtrace_struct bt;
    struct
    {
        u8              enabled;
        tilInfo_struct *match;
        int             num;
        int             maxNum;
    } trackInfLoop;
    struct
    {
        u8 enabled;
        void (*callBack)(void *, u32, void *);
        enum SH2STEPTYPE type;
        union
        {
            s32 levels;
            u32 address;
        };
    } stepOverOut;
} SH2_struct;

typedef struct
{
    int         id;
    const char *Name;

    int (*Init)(void);
    void (*DeInit)(void);
    void (*Reset)(SH2_struct *context);
    void FASTCALL (*Exec)(SH2_struct *context, u32 cycles);
    void FASTCALL (*TestExec)(SH2_struct *context, u32 cycles);

    void (*GetRegisters)(SH2_struct *context, sh2regs_struct *regs);
    u32 (*GetGPR)(SH2_struct *context, int num);
    u32 (*GetSR)(SH2_struct *context);
    u32 (*GetGBR)(SH2_struct *context);
    u32 (*GetVBR)(SH2_struct *context);
    u32 (*GetMACH)(SH2_struct *context);
    u32 (*GetMACL)(SH2_struct *context);
    u32 (*GetPR)(SH2_struct *context);
    u32 (*GetPC)(SH2_struct *context);

    void (*SetRegisters)(SH2_struct *context, const sh2regs_struct *regs);
    void (*SetGPR)(SH2_struct *context, int num, u32 value);
    void (*SetSR)(SH2_struct *context, u32 value);
    void (*SetGBR)(SH2_struct *context, u32 value);
    void (*SetVBR)(SH2_struct *context, u32 value);
    void (*SetMACH)(SH2_struct *context, u32 value);
    void (*SetMACL)(SH2_struct *context, u32 value);
    void (*SetPR)(SH2_struct *context, u32 value);
    void (*SetPC)(SH2_struct *context, u32 value);
    void (*OnFrame)(SH2_struct *context);
    void (*SendInterrupt)(SH2_struct *context, u8 vector, u8 level);
    void (*RemoveInterrupt)(SH2_struct *context, u8 vector, u8 level);
    int (*GetInterrupts)(SH2_struct *context, interrupt_struct interrupts[MAX_INTERRUPTS]);
    void (*SetInterrupts)(SH2_struct *context, int num_interrupts, const interrupt_struct interrupts[MAX_INTERRUPTS]);

    void (*WriteNotify)(u32 start, u32 length);
    void (*AddCycle)(SH2_struct *context, u32 value);
} SH2Interface_struct;

extern SH2_struct          *MSH2;
extern SH2_struct          *SSH2;
extern SH2Interface_struct *SH2Core;

int           SH2Init(int coreid);
void          SH2DeInit(void);
void          SH2Reset(SH2_struct *context);
void          SH2PowerOn(SH2_struct *context);
void FASTCALL SH2Exec(SH2_struct *context, u32 cycles);
void FASTCALL SH2TestExec(SH2_struct *context, u32 cycles);
void          SH2SendInterrupt(SH2_struct *context, u8 vector, u8 level);
void          SH2RemoveInterrupt(SH2_struct *context, u8 vector, u8 level);
void          SH2NMI(SH2_struct *context);

void SH2GetRegisters(SH2_struct *context, sh2regs_struct *r);
void SH2SetRegisters(SH2_struct *context, sh2regs_struct *r);
void SH2WriteNotify(SH2_struct *context, u32 start, u32 length);

void SH2DumpHistory(SH2_struct *context);

int BackupHandled(SH2_struct *sh, u32 addr);
int isBackupHandled(u32 addr);

u8   CacheReadByte(SH2_struct *context, u8 *mem, u32 addr);
u16  CacheReadWord(SH2_struct *context, u8 *mem, u32 addr);
u32  CacheReadLong(SH2_struct *context, u8 *mem, u32 addr);
void CacheWriteByte(SH2_struct *context, u8 *mem, u32 addr, u8 val);
void CacheWriteShort(SH2_struct *context, u8 *mem, u32 addr, u16 val);
void CacheWriteLong(SH2_struct *context, u8 *mem, u32 addr, u32 val);

void CacheInvalidate(SH2_struct *context, u32 addr);

void DMAExec(SH2_struct *context);
void DMATransfer(SH2_struct *context, u32 *CHCR, u32 *SAR, u32 *DAR, u32 *TCR, u32 *VCRDMA);

u8 FASTCALL   OnchipReadByte(SH2_struct *context, u32 addr);
u16 FASTCALL  OnchipReadWord(SH2_struct *context, u32 addr);
u32 FASTCALL  OnchipReadLong(SH2_struct *context, u32 addr);
void FASTCALL OnchipWriteByte(SH2_struct *context, u32 addr, u8 val);
void FASTCALL OnchipWriteWord(SH2_struct *context, u32 addr, u16 val);
void FASTCALL OnchipWriteLong(SH2_struct *context, u32 addr, u32 val);

u32 FASTCALL  AddressArrayReadLong(SH2_struct *context, u32 addr);
void FASTCALL AddressArrayWriteLong(SH2_struct *context, u32 addr, u32 val);

u8 FASTCALL   DataArrayReadByte(SH2_struct *context, u32 addr);
u16 FASTCALL  DataArrayReadWord(SH2_struct *context, u32 addr);
u32 FASTCALL  DataArrayReadLong(SH2_struct *context, u32 addr);
void FASTCALL DataArrayWriteByte(SH2_struct *context, u32 addr, u8 val);
void FASTCALL DataArrayWriteWord(SH2_struct *context, u32 addr, u16 val);
void FASTCALL DataArrayWriteLong(SH2_struct *context, u32 addr, u32 val);

void FASTCALL MSH2InputCaptureWriteWord(SH2_struct *context, UNUSED u8 *mem, u32 addr, u16 data);
void FASTCALL SSH2InputCaptureWriteWord(SH2_struct *context, UNUSED u8 *mem, u32 addr, u16 data);

int SH2SaveState(SH2_struct *context, void **stream);
int SH2LoadState(SH2_struct *context, const void *stream, UNUSED int version, int size);

extern SH2Interface_struct SH2Dyn;
extern SH2Interface_struct SH2DynDebug;

#if DYNAREC_KRONOS
extern SH2Interface_struct SH2KronosInterpreter;
#endif

#ifdef __cplusplus
}
#endif

#endif
