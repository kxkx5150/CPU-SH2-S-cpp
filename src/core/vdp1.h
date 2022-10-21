
#ifndef VDP1_H
#define VDP1_H

#include "memory.h"
#include "vdp2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VIDCORE_DEFAULT -1
#define CMD_QUEUE_SIZE  2048

typedef struct
{
    u16 TVMR;
    u16 FBCR;
    u16 PTMR;
    u16 EWDR;
    u16 EWLR;
    u16 EWRR;
    u16 ENDR;
    u16 EDSR;
    u16 LOPR;
    u16 COPR;
    u16 MODR;

    u16 lCOPR;

    u32 addr;

    s16 localX;
    s16 localY;

    u16 systemclipX1;
    u16 systemclipY1;
    u16 systemclipX2;
    u16 systemclipY2;

    u16 userclipX1;
    u16 userclipY1;
    u16 userclipX2;
    u16 userclipY2;

} Vdp1;

typedef struct
{
    int disptoggle;
    int manualerase;
    int manualchange;
    int onecyclemode;
    int useVBlankErase;
    int swap_frame_buffer;
    int plot_trigger_line;
    int plot_trigger_done;
    int current_frame;
    int updateVdp1Ram;
    int checkEDSR;
    int status;
} Vdp1External_struct;

extern Vdp1External_struct Vdp1External;

typedef enum
{
    VDPCT_NORMAL_SPRITE               = 0,
    VDPCT_SCALED_SPRITE               = 1,
    VDPCT_DISTORTED_SPRITE            = 2,
    VDPCT_DISTORTED_SPRITEN           = 3,
    VDPCT_POLYGON                     = 4,
    VDPCT_POLYLINE                    = 5,
    VDPCT_LINE                        = 6,
    VDPCT_POLYLINEN                   = 7,
    VDPCT_USER_CLIPPING_COORDINATES   = 8,
    VDPCT_SYSTEM_CLIPPING_COORDINATES = 9,
    VDPCT_LOCAL_COORDINATES           = 10,
    VDPCT_USER_CLIPPING_COORDINATESN  = 11,

    VDPCT_INVALID = 12,
    VDPCT_DRAW_END

} Vdp1CommandType;

typedef struct
{
    u32   CMDCTRL;
    u32   CMDLINK;
    u32   CMDPMOD;
    u32   CMDCOLR;
    u32   CMDSRCA;
    u32   CMDSIZE;
    s32   CMDXA;
    s32   CMDYA;
    s32   CMDXB;
    s32   CMDYB;
    s32   CMDXC;
    s32   CMDYC;
    s32   CMDXD;
    s32   CMDYD;
    u32   CMDGRDA;
    u32   COLOR[4];
    float G[16];
    u32   priority;
    u32   w;
    u32   h;
    u32   flip;
    u32   type;
    u32   SPCTL;
    s32   B[4];
    u32   nbStep;
    float uAstepx;
    float uAstepy;
    float uBstepx;
    float uBstepy;
    u32   pad[2];
} vdp1cmd_struct;

typedef struct
{
    vdp1cmd_struct cmd;
    int            ignitionLine;
    int            start_addr;
    int            end_addr;
    int            dirty;
} vdp1cmdctrl_struct;

typedef struct
{
    int         id;
    const char *Name;
    int (*Init)(void);
    void (*DeInit)(void);
    void (*Resize)(int, int, unsigned int, unsigned int, int);
    void (*getScale)(float *xRatio, float *yRatio);
    int (*IsFullscreen)(void);
    int (*Vdp1Reset)(void);
    void (*Vdp1Draw)();
    void (*Vdp1NormalSpriteDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1ScaledSpriteDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1DistortedSpriteDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1PolygonDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1PolylineDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1LineDraw)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
    void (*Vdp1UserClipping)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs);
    void (*Vdp1SystemClipping)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs);
    void (*Vdp1LocalCoordinate)(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs);
    void (*Vdp1ReadFrameBuffer)(u32 type, u32 addr, void *out);
    void (*Vdp1WriteFrameBuffer)(u32 type, u32 addr, u32 val);
    void (*Vdp1EraseWrite)(int id);
    void (*Vdp1FrameChange)(void);
    void (*Vdp1RegenerateCmd)(vdp1cmd_struct *cmd);
    int (*Vdp2Reset)(void);
    void (*Vdp2Draw)(void);
    void (*GetGlSize)(int *width, int *height);
    void (*SetSettingValue)(int type, int value);
    void (*Sync)();
    void (*GetNativeResolution)(int *width, int *height, int *interlace);
    void (*Vdp2DispOff)(void);
    void (*composeFB)(Vdp2 *regs);
    void (*composeVDP1)(void);
    int (*setupFrame)(int);
    void (*FinsihDraw)(void);
} VideoInterface_struct;

extern VideoInterface_struct *VIDCore;

extern vdp1cmdctrl_struct cmdBufferBeingProcessed[CMD_QUEUE_SIZE];

extern u8 *Vdp1Ram;
extern int vdp1Ram_update_start;
extern int vdp1Ram_update_end;

u8 FASTCALL   Vdp1RamReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp1RamReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp1RamReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp1RamWriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp1RamWriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp1RamWriteLong(SH2_struct *context, u8 *, u32, u32);
u8 FASTCALL   Vdp1FrameBufferReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp1FrameBufferReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp1FrameBufferReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp1FrameBufferWriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp1FrameBufferWriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp1FrameBufferWriteLong(SH2_struct *context, u8 *, u32, u32);

void Vdp1DrawCommands(u8 *ram, Vdp1 *regs, u8 *back_framebuffer);
void Vdp1FakeDrawCommands(u8 *ram, Vdp1 *regs);

extern Vdp1 *Vdp1Regs;

enum VDP1STATUS
{
    VDP1_STATUS_IDLE = 0,
    VDP1_STATUS_RUNNING
};

int  Vdp1Init(void);
void Vdp1DeInit(void);
int  VideoInit(int coreid);
int  VideoChangeCore(int coreid);
void VideoDeInit(void);
void Vdp1Reset(void);
int  VideoSetSetting(int type, int value);

u8 FASTCALL   Vdp1ReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Vdp1ReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Vdp1ReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Vdp1WriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Vdp1WriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Vdp1WriteLong(SH2_struct *context, u8 *, u32, u32);

int Vdp1SaveState(void **stream);
int Vdp1LoadState(const void *stream, int version, int size);

char           *Vdp1DebugGetCommandNumberName(u32 number);
Vdp1CommandType Vdp1DebugGetCommandType(u32 number);
void            Vdp1DebugCommand(u32 number, char *outstring);
u32            *Vdp1DebugTexture(u32 number, int *w, int *h);
u8             *Vdp1DebugRawTexture(u32 number, int *w, int *h, int *numBytes);
void            ToggleVDP1(void);

void Vdp1HBlankIN(void);
void Vdp1HBlankOUT(void);
void Vdp1VBlankIN(void);
void Vdp1VBlankOUT(void);

#ifdef __cplusplus
}
#endif

#endif
