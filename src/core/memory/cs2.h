
#ifndef CS2_H
#define CS2_H

#include "memory.h"
#include "cdbase.h"
#include "cs0.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLOCKS    200
#define MAX_SELECTORS 24
#define MAX_FILES     256

typedef struct
{
    s32 size;
    u32 FAD;
    u8  cn;
    u8  fn;
    u8  sm;
    u8  ci;
    u8  data[2352];
} block_struct;

typedef struct
{
    u32 FAD;
    u32 range;
    u8  mode;
    u8  chan;
    u8  smmask;
    u8  cimask;
    u8  fid;
    u8  smval;
    u8  cival;
    u8  condtrue;
    u8  condfalse;
} filter_struct;

typedef struct
{
    s32           size;
    block_struct *block[MAX_BLOCKS];
    u8            blocknum[MAX_BLOCKS];
    u8            numblocks;
} partition_struct;

typedef struct
{
    u16 groupid;
    u16 userid;
    u16 attributes;
    u16 signature;
    u8  filenumber;
    u8  reserved[5];
} xarec_struct;

typedef struct
{
    u8           recordsize;
    u8           xarecordsize;
    u32          lba;
    u32          size;
    u8           dateyear;
    u8           datemonth;
    u8           dateday;
    u8           datehour;
    u8           dateminute;
    u8           datesecond;
    u8           gmtoffset;
    u8           flags;
    u8           fileunitsize;
    u8           interleavegapsize;
    u16          volumesequencenumber;
    u8           namelength;
    char         name[32];
    xarec_struct xarecord;
} dirrec_struct;

typedef struct
{
    u8 vidplaymode;
    u8 dectimingmode;
    u8 outmode;
    u8 slmode;
} mpegmode_struct;

typedef struct
{
    u8 audcon;
    u8 audlay;
    u8 audbufnum;
    u8 vidcon;
    u8 vidlay;
    u8 vidbufnum;
} mpegcon_struct;

typedef struct
{
    u8 audstm;
    u8 audstmid;
    u8 audchannum;
    u8 vidstm;
    u8 vidstmid;
    u8 vidchannum;
} mpegstm_struct;

typedef struct
{
    u32 DTR;
    u16 UNKNOWN;
    u16 HIRQ;
    u16 HIRQMASK;
    u16 CR1;
    u16 CR2;
    u16 CR3;
    u16 CR4;
    u16 MPEGRGB;
} blockregs_struct;

typedef struct
{
    blockregs_struct reg;
    u32              FAD;
    u8               status;

    u8 options;
    u8 repcnt;
    u8 ctrladdr;
    u8 track;
    u8 index;

    u16 satauth;
    u16 mpgauth;

    u32          transfercount;
    u32          cdwnum;
    u32          TOC[102];
    u32          playFAD;
    u32          playendFAD;
    unsigned int maxrepeat;
    u32          getsectsize;
    u32          putsectsize;
    u32          calcsize;
    s32          infotranstype;
    s32          datatranstype;
    int          isonesectorstored;
    int          isdiskchanged;
    int          isbufferfull;
    int          speed1x;
    int          isaudio;
    u8           transfileinfo[12];
    u8           lastbuffer;
    u8           transscodeq[5 * 2];
    u8           transscoderw[12 * 2];

    filter_struct  filter[MAX_SELECTORS];
    filter_struct *outconcddev;
    filter_struct *outconmpegfb;
    filter_struct *outconmpegbuf;
    filter_struct *outconmpegrom;
    filter_struct *outconhost;
    u8             outconcddevnum;
    u8             outconmpegfbnum;
    u8             outconmpegbufnum;
    u8             outconmpegromnum;
    u8             outconhostnum;

    partition_struct partition[MAX_SELECTORS];

    partition_struct *datatranspartition;
    u8                datatranspartitionnum;
    s32               datatransoffset;
    u32               datanumsecttrans;
    u16               datatranssectpos;
    u16               datasectstotrans;

    u32          blockfreespace;
    block_struct block[MAX_BLOCKS];
    struct
    {
        s32 size;
        u32 FAD;
        u8  cn;
        u8  fn;
        u8  sm;
        u8  ci;
        u8  data[2448];
    } workblock;

    u32           curdirsect;
    u32           curdirsize;
    u32           curdirfidoffset;
    dirrec_struct fileinfo[MAX_FILES];
    u32           numfiles;

    const char *mpegpath;

    u32 mpegintmask;

    mpegmode_struct mpegmode;
    mpegcon_struct  mpegcon[2];
    mpegstm_struct  mpegstm[2];

    int          _command;
    u32          _statuscycles;
    u32          _statustiming;
    u32          _periodiccycles;
    u32          _periodictiming;
    u32          _commandtiming;
    CDInterface *cdi;

    int carttype;
    int playtype;

    u8  actionstatus;
    u8  pictureinfo;
    u8  mpegaudiostatus;
    u16 mpegvideostatus;
    u16 vcounter;

} Cs2;

typedef struct
{
    char system[17];
    char company[17];
    char itemnum[11];
    char version[7];
    char date[11];
    char cdinfo[9];
    char region[11];
    char peripheral[17];
    char gamename[113];
    u32  ipsize;
    u32  msh2stack;
    u32  ssh2stack;
    u32  firstprogaddr;
    u32  firstprogsize;
    u64  gameid;
} ip_struct;

extern Cs2       *Cs2Area;
extern ip_struct *cdip;

int  Cs2Init(int coreid, const char *cdpath, const char *mpegpath);
int  Cs2ChangeCDCore(int coreid, const char *cdpath);
void Cs2DeInit(void);

u8 FASTCALL   Cs2ReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Cs2ReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Cs2ReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Cs2WriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Cs2WriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Cs2WriteLong(SH2_struct *context, u8 *, u32, u32);

void Cs2Exec(u32);
int  Cs2GetTimeToNextSector(void);
void Cs2Execute(void);
void Cs2Reset(void);
void Cs2SetTiming(int);
void Cs2Command(void);
void Cs2SetCommandTiming(u8 cmd);

void Cs2GetStatus(void);
void Cs2GetHardwareInfo(void);
void Cs2GetToc(void);
void Cs2GetSessionInfo(void);
void Cs2InitializeCDSystem(void);
void Cs2OpenTray(void);
void Cs2EndDataTransfer(void);
void Cs2PlayDisc(void);
void Cs2SeekDisc(void);
void Cs2ScanDisc(void);
void Cs2GetSubcodeQRW(void);
void Cs2SetCDDeviceConnection(void);
void Cs2GetCDDeviceConnection(void);
void Cs2GetLastBufferDestination(void);
void Cs2SetFilterRange(void);
void Cs2GetFilterRange(void);
void Cs2SetFilterSubheaderConditions(void);
void Cs2GetFilterSubheaderConditions(void);
void Cs2SetFilterMode(void);
void Cs2GetFilterMode(void);
void Cs2SetFilterConnection(void);
void Cs2GetFilterConnection(void);
void Cs2ResetSelector(void);
void Cs2GetBufferSize(void);
void Cs2GetSectorNumber(void);
void Cs2CalculateActualSize(void);
void Cs2GetActualSize(void);
void Cs2GetSectorInfo(void);
void Cs2ExecFadSearch(void);
void Cs2GetFadSearchResults(void);
void Cs2SetSectorLength(void);
void Cs2GetSectorData(void);
void Cs2DeleteSectorData(void);
void Cs2GetThenDeleteSectorData(void);
void Cs2PutSectorData(void);
void Cs2CopySectorData(void);
void Cs2MoveSectorData(void);
void Cs2GetCopyError(void);
void Cs2ChangeDirectory(void);
void Cs2ReadDirectory(void);
void Cs2GetFileSystemScope(void);
void Cs2GetFileInfo(void);
void Cs2ReadFile(void);
void Cs2AbortFile(void);
void Cs2MpegGetStatus(void);
void Cs2MpegGetInterrupt(void);
void Cs2MpegSetInterruptMask(void);
void Cs2MpegInit(void);
void Cs2MpegSetMode(void);
void Cs2MpegPlay(void);
void Cs2MpegSetDecodingMethod(void);
void Cs2MpegSetConnection(void);
void Cs2MpegGetConnection(void);
void Cs2MpegSetStream(void);
void Cs2MpegGetStream(void);
void Cs2MpegDisplay(void);
void Cs2MpegSetWindow(void);
void Cs2MpegSetBorderColor(void);
void Cs2MpegSetFade(void);
void Cs2MpegSetVideoEffects(void);
void Cs2MpegSetLSI(void);
void Cs2AuthenticateDevice(void);
void Cs2IsDeviceAuthenticated(void);
void Cs2GetMPEGRom(void);

u8                Cs2FADToTrack(u32 val);
u32               Cs2TrackToFAD(u16 trackandindex);
void              Cs2FADToMSF(u32 val, u8 *m, u8 *s, u8 *f);
void              Cs2SetupDefaultPlayStats(u8 track_number, int writeFAD);
block_struct     *Cs2AllocateBlock(u8 *blocknum, s32 sectsize);
void              Cs2FreeBlock(block_struct *blk);
void              Cs2SortBlocks(partition_struct *part);
partition_struct *Cs2GetPartition(filter_struct *curfilter);
partition_struct *Cs2FilterData(filter_struct *curfilter, int isaudio);
int               Cs2CopyDirRecord(u8 *buffer, dirrec_struct *dirrec);
int               Cs2ReadFileSystem(filter_struct *curfilter, u32 fid, int isoffset);
void              Cs2SetupFileInfoTransfer(u32 fid);
partition_struct *Cs2ReadUnFilteredSector(u32 rufsFAD);
int               Cs2ReadFilteredSector(u32 rfsFAD, partition_struct **partition);
u8                Cs2GetIP(int autoregion);
u8                Cs2GetRegionID(void);
int               Cs2SaveState(void **stream);
int               Cs2LoadState(const void *stream, int version, int size);
u32               Cs2GetMasterStackAdress(void);
u32               Cs2GetSlaveStackAdress(void);
u64               Cs2GetGameId();
char             *Cs2GetCurrentGmaecode();

u32 Cs2GetMasterExecutionAdress();

void Cs2ForceOpenTray();
int  Cs2ForceCloseTray(int coreid, const char *cdpath);

#ifdef __cplusplus
}
#endif

#endif
