/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file vdp1.c
    \brief VDP1 emulation functions.
*/


#include <stdlib.h>
#include <math.h>
#include "yabause.h"
#include "vdp1.h"
#include "debug.h"
#include "scu.h"
#include "vdp2.h"

#include "threads.h"
#include "sh2core.h"
#include "ygl.h"
#include "yui.h"


// #define DEBUG_CMD_LIST
#define VIDCORE_SOFT 3

u8 *Vdp1Ram;
int vdp1Ram_update_start;
int vdp1Ram_update_end;
int VDP1_MASK = 0xFFFF;

VideoInterface_struct        *VIDCore = NULL;
extern VideoInterface_struct *VIDCoreList[];

Vdp1               *Vdp1Regs;
Vdp1External_struct Vdp1External;

vdp1cmdctrl_struct cmdBufferBeingProcessed[CMD_QUEUE_SIZE];

int vdp1_clock = 0;

static int nbCmdToProcess = 0;
static int CmdListInLoop  = 0;
static int CmdListLimit   = 0x80000;


static int           needVdp1draw = 0;
static void          Vdp1NoDraw(void);
static int           Vdp1Draw(void);
static void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr, u8 *ram);

extern void addVdp1Framecount();

#define DEBUG_BAD_COORD    // YuiMsg

#define CONVERTCMD(A)                                                                                                  \
    {                                                                                                                  \
        s32 toto = (A);                                                                                                \
        if (((A)&0x7000) != 0)                                                                                         \
            (A) |= 0xF000;                                                                                             \
        else                                                                                                           \
            (A) &= ~0xF800;                                                                                            \
        ((A) = (s32)(s16)(A));                                                                                         \
        if (((A)) < -1024) {                                                                                           \
            DEBUG_BAD_COORD("Bad(-1024) %x (%d, 0x%x)\n", (A), (A), toto);                                             \
        }                                                                                                              \
        if (((A)) > 1023) {                                                                                            \
            DEBUG_BAD_COORD("Bad(1023) %x (%d, 0x%x)\n", (A), (A), toto);                                              \
        }                                                                                                              \
    }

static void RequestVdp1ToDraw()
{
    needVdp1draw = 1;
}


static void abortVdp1()
{
    if (Vdp1External.status == VDP1_STATUS_RUNNING) {
        // The vdp1 is still running and a new draw command request has been received
        // Abort the current command list
        Vdp1External.status = VDP1_STATUS_IDLE;
        CmdListInLoop       = 0;
        vdp1_clock          = 0;
        nbCmdToProcess      = 0;
        needVdp1draw        = 0;
    }
}
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1RamReadByte(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x7FFFF;
    return T1ReadByte(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1RamReadWord(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x07FFFF;
    return T1ReadWord(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1RamReadLong(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x7FFFF;
    return T1ReadLong(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteByte(SH2_struct *context, u8 *mem, u32 addr, u8 val)
{
    addr &= 0x7FFFF;
    Vdp1External.updateVdp1Ram = 1;
    if (Vdp1External.status == VDP1_STATUS_RUNNING)
        vdp1_clock -= 1;
    if (vdp1Ram_update_start > addr)
        vdp1Ram_update_start = addr;
    if (vdp1Ram_update_end < addr + 1)
        vdp1Ram_update_end = addr + 1;
    T1WriteByte(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteWord(SH2_struct *context, u8 *mem, u32 addr, u16 val)
{
    addr &= 0x7FFFF;
    Vdp1External.updateVdp1Ram = 1;
    if (Vdp1External.status == VDP1_STATUS_RUNNING)
        vdp1_clock -= 2;
    if (vdp1Ram_update_start > addr)
        vdp1Ram_update_start = addr;
    if (vdp1Ram_update_end < addr + 2)
        vdp1Ram_update_end = addr + 2;
    T1WriteWord(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteLong(SH2_struct *context, u8 *mem, u32 addr, u32 val)
{
    addr &= 0x7FFFF;
    Vdp1External.updateVdp1Ram = 1;
    if (Vdp1External.status == VDP1_STATUS_RUNNING)
        vdp1_clock -= 4;
    if (vdp1Ram_update_start > addr)
        vdp1Ram_update_start = addr;
    if (vdp1Ram_update_end < addr + 4)
        vdp1Ram_update_end = addr + 4;
    T1WriteLong(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1FrameBufferReadByte(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x3FFFF;
    if (VIDCore->Vdp1ReadFrameBuffer) {
        u8 val;
        VIDCore->Vdp1ReadFrameBuffer(0, addr, &val);
        return val;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1FrameBufferReadWord(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x3FFFF;
    if (VIDCore->Vdp1ReadFrameBuffer) {
        u16 val;
        VIDCore->Vdp1ReadFrameBuffer(1, addr, &val);
        return val;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1FrameBufferReadLong(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x3FFFF;
    if (VIDCore->Vdp1ReadFrameBuffer) {
        u32 val;
        VIDCore->Vdp1ReadFrameBuffer(2, addr, &val);
        return val;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteByte(SH2_struct *context, u8 *mem, u32 addr, u8 val)
{
    addr &= 0x7FFFF;

    if (VIDCore->Vdp1WriteFrameBuffer) {
        if (addr < 0x40000)
            VIDCore->Vdp1WriteFrameBuffer(0, addr, val);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteWord(SH2_struct *context, u8 *mem, u32 addr, u16 val)
{
    addr &= 0x7FFFF;

    if (VIDCore->Vdp1WriteFrameBuffer) {
        if (addr < 0x40000)
            VIDCore->Vdp1WriteFrameBuffer(1, addr, val);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteLong(SH2_struct *context, u8 *mem, u32 addr, u32 val)
{
    addr &= 0x7FFFF;

    if (VIDCore->Vdp1WriteFrameBuffer) {
        if (addr < 0x40000)
            VIDCore->Vdp1WriteFrameBuffer(2, addr, val);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

int Vdp1Init(void)
{
    if ((Vdp1Regs = (Vdp1 *)malloc(sizeof(Vdp1))) == NULL)
        return -1;

    if ((Vdp1Ram = T1MemoryInit(0x80000)) == NULL)
        return -1;

    Vdp1External.disptoggle = 1;

    Vdp1Regs->TVMR = 0;
    Vdp1Regs->FBCR = 0;
    Vdp1Regs->PTMR = 0;

    Vdp1Regs->userclipX1 = 0;
    Vdp1Regs->userclipY1 = 0;
    Vdp1Regs->userclipX2 = 1024;
    Vdp1Regs->userclipY2 = 512;

    Vdp1Regs->localX = 0;
    Vdp1Regs->localY = 0;

    VDP1_MASK = 0xFFFF;

    vdp1Ram_update_start = 0x80000;
    vdp1Ram_update_end   = 0x0;

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1DeInit(void)
{
    if (Vdp1Regs)
        free(Vdp1Regs);
    Vdp1Regs = NULL;

    if (Vdp1Ram)
        T1MemoryDeInit(Vdp1Ram);
    Vdp1Ram = NULL;
}

//////////////////////////////////////////////////////////////////////////////

int VideoInit(int coreid)
{
    return VideoChangeCore(coreid);
}

//////////////////////////////////////////////////////////////////////////////

int VideoChangeCore(int coreid)
{
    int i;

    // Make sure the old core is freed
    VideoDeInit();

    // So which core do we want?
    if (coreid == VIDCORE_DEFAULT)
        coreid = 0;    // Assume we want the first one

    // Go through core list and find the id
    for (i = 0; VIDCoreList[i] != NULL; i++) {
        if (VIDCoreList[i]->id == coreid) {
            // Set to current core
            VIDCore = VIDCoreList[i];
            break;
        }
    }

    if (VIDCore == NULL)
        return -1;

    if (VIDCore->Init() != 0)
        return -1;

    // Reset resolution/priority variables
    if (Vdp2Regs) {
        VIDCore->Vdp1Reset();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VideoDeInit(void)
{
    if (VIDCore)
        VIDCore->DeInit();
    VIDCore = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Reset(void)
{
    Vdp1Regs->PTMR = 0;
    Vdp1Regs->MODR = 0x1000;    // VDP1 Version 1
    Vdp1Regs->TVMR = 0;
    Vdp1Regs->EWDR = 0;
    Vdp1Regs->EWLR = 0;
    Vdp1Regs->EWRR = 0;
    Vdp1Regs->ENDR = 0;
    VDP1_MASK      = 0xFFFF;
    VIDCore->Vdp1Reset();
    vdp1_clock = 0;
}

int VideoSetSetting(int type, int value)
{
    if (VIDCore)
        VIDCore->SetSettingValue(type, value);
    return 0;
}


//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1ReadByte(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0xFF;
    LOG("trying to byte-read a Vdp1 register\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
u16 FASTCALL Vdp1ReadWord(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0xFF;
    switch (addr) {
        case 0x10:
            FRAMELOG("Read EDSR %X line = %d\n", Vdp1Regs->EDSR, yabsys.LineCount);
            if (Vdp1External.checkEDSR == 0) {
                if (VIDCore != NULL)
                    if (VIDCore->FinsihDraw != NULL)
                        VIDCore->FinsihDraw();
            }
            Vdp1External.checkEDSR = 1;
            return Vdp1Regs->EDSR;
        case 0x12:
            FRAMELOG("Read LOPR %X line = %d\n", Vdp1Regs->LOPR, yabsys.LineCount);
            return Vdp1Regs->LOPR;
        case 0x14:
            FRAMELOG("Read COPR %X line = %d\n", Vdp1Regs->COPR, yabsys.LineCount);
            return Vdp1Regs->COPR;
        case 0x16:
            return 0x1000 | ((Vdp1Regs->PTMR & 2) << 7) | ((Vdp1Regs->FBCR & 0x1E) << 3) | (Vdp1Regs->TVMR & 0xF);
        default:
            LOG("trying to read a Vdp1 write-only register\n");
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1ReadLong(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0xFF;
    LOG("trying to long-read a Vdp1 register - %08X\n", addr);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteByte(SH2_struct *context, u8 *mem, u32 addr, UNUSED u8 val)
{
    addr &= 0xFF;
    LOG("trying to byte-write a Vdp1 register - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////
static int needVBlankErase()
{
    return (Vdp1External.useVBlankErase != 0);
}
static void updateTVMRMode()
{
    Vdp1External.useVBlankErase = 0;
    if (((Vdp1Regs->FBCR & 3) == 3) && (((Vdp1Regs->TVMR >> 3) & 0x01) == 1)) {
        Vdp1External.useVBlankErase = 1;
    } else {
        if ((((Vdp1Regs->TVMR >> 3) & 0x01) == 1)) {
            // VBE can be one only when FCM and FCT are 1
            LOG("Prohibited FBCR/TVMR values\n");
            // Assume prohibited modes behave like if VBE/FCT/FCM were all 1
            Vdp1External.manualchange   = 1;
            Vdp1External.useVBlankErase = 1;
        }
    }
}

static void updateFBCRMode()
{
    Vdp1External.manualchange   = 0;
    Vdp1External.onecyclemode   = 0;
    Vdp1External.useVBlankErase = 0;
    if (((Vdp1Regs->TVMR >> 3) & 0x01) == 1) {    // VBE is set
        if ((Vdp1Regs->FBCR & 3) == 3) {
            Vdp1External.manualchange   = 1;
            Vdp1External.useVBlankErase = 1;
        } else {
            // VBE can be one only when FCM and FCT are 1
            LOG("Prohibited FBCR/TVMR values\n");
            // Assume prohibited modes behave like if VBE/FCT/FCM were all 1
            Vdp1External.manualchange   = 1;
            Vdp1External.useVBlankErase = 1;
        }
    } else {
        // Manual erase shall not be reseted but need to save its current value
        //  Only at frame change the order is executed.
        // This allows to have both a manual clear and a manual change at the same frame without continuously clearing
        // the VDP1 The mechanism is used by the official bios animation
        Vdp1External.onecyclemode = ((Vdp1Regs->FBCR & 3) == 0) || ((Vdp1Regs->FBCR & 3) == 1);
        Vdp1External.manualerase |= ((Vdp1Regs->FBCR & 3) == 2);
        Vdp1External.manualchange = ((Vdp1Regs->FBCR & 3) == 3);
    }
}

static void Vdp1TryDraw(void)
{
    if ((needVdp1draw == 1)) {
        needVdp1draw = Vdp1Draw();
    }
}

void FASTCALL Vdp1WriteWord(SH2_struct *context, u8 *mem, u32 addr, u16 val)
{
    u16 oldPTMR = 0;
    addr &= 0xFF;
    switch (addr) {
        case 0x0:
            if ((Vdp1Regs->FBCR & 3) != 3)
                val = (val & (~0x4));
            Vdp1Regs->TVMR = val;
            updateTVMRMode();
            FRAMELOG("TVMR => Write VBE=%d FCM=%d FCT=%d line = %d\n", (Vdp1Regs->TVMR >> 3) & 0x01,
                     (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01), yabsys.LineCount);
            break;
        case 0x2:
            Vdp1Regs->FBCR = val;
            FRAMELOG("FBCR => Write VBE=%d FCM=%d FCT=%d line = %d\n", (Vdp1Regs->TVMR >> 3) & 0x01,
                     (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01), yabsys.LineCount);
            updateFBCRMode();
            break;
        case 0x4:
            FRAMELOG("Write PTMR %X line = %d %d\n", val, yabsys.LineCount, yabsys.VBlankLineCount);
            if ((val & 0x3) == 0x3) {
                // Skeleton warriors is writing 0xFFF to PTMR. It looks like the behavior is 0x2
                val = 0x2;
            }
            oldPTMR                        = Vdp1Regs->PTMR;
            Vdp1Regs->PTMR                 = val;
            Vdp1External.plot_trigger_line = -1;
            Vdp1External.plot_trigger_done = 0;
            if (val == 1) {
                FRAMELOG("VDP1: VDPEV_DIRECT_DRAW\n");
                Vdp1External.plot_trigger_line = yabsys.LineCount;
                abortVdp1();
                RequestVdp1ToDraw();
                Vdp1TryDraw();
                Vdp1External.plot_trigger_done = 1;
            }
            if ((val == 0x2) && (oldPTMR == 0x0)) {
                FRAMELOG("[VDP1] PTMR == 0x2 start drawing immidiatly\n");
                abortVdp1();
                RequestVdp1ToDraw();
                Vdp1TryDraw();
            }
            break;
        case 0x6:
            Vdp1Regs->EWDR = val;
            break;
        case 0x8:
            Vdp1Regs->EWLR = val;
            break;
        case 0xA:
            Vdp1Regs->EWRR = val;
            break;
        case 0xC:
            Vdp1Regs->ENDR = val;
            abortVdp1();
            break;
        default:
            LOG("trying to write a Vdp1 read-only register - %08X\n", addr);
    }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteLong(SH2_struct *context, u8 *mem, u32 addr, UNUSED u32 val)
{
    addr &= 0xFF;
    LOG("trying to long-write a Vdp1 register - %08X\n", addr);
}

static void printCommand(vdp1cmd_struct *cmd)
{
    printf("===== CMD =====\n");
    printf("CMDCTRL = 0x%x\n", cmd->CMDCTRL);
    printf("CMDLINK = 0x%x\n", cmd->CMDLINK);
    printf("CMDPMOD = 0x%x\n", cmd->CMDPMOD);
    printf("CMDCOLR = 0x%x\n", cmd->CMDCOLR);
    printf("CMDSRCA = 0x%x\n", cmd->CMDSRCA);
    printf("CMDSIZE = 0x%x\n", cmd->CMDSIZE);
    printf("CMDXA = 0x%x\n", cmd->CMDXA);
    printf("CMDYA = 0x%x\n", cmd->CMDYA);
    printf("CMDXB = 0x%x\n", cmd->CMDXB);
    printf("CMDYB = 0x%x\n", cmd->CMDYB);
    printf("CMDXC = 0x%x\n", cmd->CMDXC);
    printf("CMDYC = 0x%x\n", cmd->CMDYC);
    printf("CMDXD = 0x%x\n", cmd->CMDXD);
    printf("CMDYD = 0x%x\n", cmd->CMDYD);
    printf("CMDGRDA = 0x%x\n", cmd->CMDGRDA);
}

static int emptyCmd(vdp1cmd_struct *cmd)
{
    return ((cmd->CMDCTRL == 0) && (cmd->CMDLINK == 0) && (cmd->CMDPMOD == 0) && (cmd->CMDCOLR == 0) &&
            (cmd->CMDSRCA == 0) && (cmd->CMDSIZE == 0) && (cmd->CMDXA == 0) && (cmd->CMDYA == 0) && (cmd->CMDXB == 0) &&
            (cmd->CMDYB == 0) && (cmd->CMDXC == 0) && (cmd->CMDYC == 0) && (cmd->CMDXD == 0) && (cmd->CMDYD == 0) &&
            (cmd->CMDGRDA == 0));
}

//////////////////////////////////////////////////////////////////////////////

static void checkClipCmd(vdp1cmd_struct **sysClipCmd, vdp1cmd_struct **usrClipCmd, vdp1cmd_struct **localCoordCmd,
                         u8 *ram, Vdp1 *regs)
{
    if (sysClipCmd != NULL) {
        if (*sysClipCmd != NULL) {
            VIDCore->Vdp1SystemClipping(*sysClipCmd, ram, regs);
            free(*sysClipCmd);
            *sysClipCmd = NULL;
        }
    }
    if (usrClipCmd != NULL) {
        if (*usrClipCmd != NULL) {
            VIDCore->Vdp1UserClipping(*usrClipCmd, ram, regs);
            free(*usrClipCmd);
            *usrClipCmd = NULL;
        }
    }
    if (localCoordCmd != NULL) {
        if (*localCoordCmd != NULL) {
            VIDCore->Vdp1LocalCoordinate(*localCoordCmd, ram, regs);
            free(*localCoordCmd);
            *localCoordCmd = NULL;
        }
    }
}

static int Vdp1NormalSpriteDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    Vdp2 *varVdp2Regs = &Vdp2Lines[0];
    int   ret         = 1;
    if (emptyCmd(cmd)) {
        // damaged data
        yabsys.vdp1cycles += 70;
        return -1;
    }

    if ((cmd->CMDSIZE & 0x8000)) {
        yabsys.vdp1cycles += 70;
        regs->EDSR |= 2;
        return -1;    // BAD Command
    }
    if (((cmd->CMDPMOD >> 3) & 0x7) > 5) {
        // damaged data
        yabsys.vdp1cycles += 70;
        return -1;
    }
    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip     = (cmd->CMDCTRL & 0x30) >> 4;
    cmd->priority = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;

    cmd->CMDXB = cmd->CMDXA + MAX(1, cmd->w);
    cmd->CMDYB = cmd->CMDYA;
    cmd->CMDXC = cmd->CMDXA + MAX(1, cmd->w);
    cmd->CMDYC = cmd->CMDYA + MAX(1, cmd->h);
    cmd->CMDXD = cmd->CMDXA;
    cmd->CMDYD = cmd->CMDYA + MAX(1, cmd->h);

    int area =
        abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) +
            (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) /
        2;
    switch ((cmd->CMDPMOD >> 3) & 0x7) {
        case 0:
        case 1:
            // 4 pixels per 16 bits
            area = area >> 2;
            break;
        case 2:
        case 3:
        case 4:
            // 2 pixels per 16 bits
            area = area >> 1;
            break;
        default:
            break;
    }
    yabsys.vdp1cycles += MIN(1000, 70 + (area));

    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 = Vdp1RamReadWord(NULL, ram, (Vdp1RamReadWord(NULL, ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }

    VIDCore->Vdp1NormalSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

static int Vdp1ScaledSpriteDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    Vdp2 *varVdp2Regs = &Vdp2Lines[0];
    s16   rw = 0, rh = 0;
    s16   x, y;
    int   ret = 1;

    if (emptyCmd(cmd)) {
        // damaged data
        yabsys.vdp1cycles += 70;
        return -1;
    }

    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip     = (cmd->CMDCTRL & 0x30) >> 4;
    cmd->priority = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);
    CONVERTCMD(cmd->CMDXC);
    CONVERTCMD(cmd->CMDYC);

    x = cmd->CMDXA;
    y = cmd->CMDYA;
    // Setup Zoom Point
    switch ((cmd->CMDCTRL & 0xF00) >> 8) {
        case 0x0:    // Only two coordinates
            rw = cmd->CMDXC - cmd->CMDXA;
            rh = cmd->CMDYC - cmd->CMDYA;
            break;
        case 0x5:    // Upper-left
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            break;
        case 0x6:    // Upper-Center
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw / 2;
            break;
        case 0x7:    // Upper-Right
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw;
            break;
        case 0x9:    // Center-left
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            y  = y - rh / 2;
            break;
        case 0xA:    // Center-center
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw / 2;
            y  = y - rh / 2;
            break;
        case 0xB:    // Center-right
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw;
            y  = y - rh / 2;
            break;
        case 0xD:    // Lower-left
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            y  = y - rh;
            break;
        case 0xE:    // Lower-center
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw / 2;
            y  = y - rh;
            break;
        case 0xF:    // Lower-right
            rw = cmd->CMDXB;
            rh = cmd->CMDYB;
            x  = x - rw;
            y  = y - rh;
            break;
        default:
            break;
    }

    cmd->CMDXA = x + regs->localX;
    cmd->CMDYA = y + regs->localY;
    cmd->CMDXB = x + rw + regs->localX;
    cmd->CMDYB = y + regs->localY;
    cmd->CMDXC = x + rw + regs->localX;
    cmd->CMDYC = y + rh + regs->localY;
    cmd->CMDXD = x + regs->localX;
    cmd->CMDYD = y + rh + regs->localY;

    // Setup Zoom Point
    switch ((cmd->CMDCTRL & 0xF00) >> 8) {
        case 0x0:    // Only two coordinates
            if ((s16)cmd->CMDXC > (s16)cmd->CMDXA) {
                cmd->CMDXB += 1;
                cmd->CMDXC += 1;
            } else {
                cmd->CMDXA += 1;
                cmd->CMDXB += 1;
                cmd->CMDXC += 1;
                cmd->CMDXD += 1;
            }
            if ((s16)cmd->CMDYC > (s16)cmd->CMDYA) {
                cmd->CMDYC += 1;
                cmd->CMDYD += 1;
            } else {
                cmd->CMDYA += 1;
                cmd->CMDYB += 1;
                cmd->CMDYC += 1;
                cmd->CMDYD += 1;
            }
            break;
        case 0x5:    // Upper-left
        case 0x6:    // Upper-Center
        case 0x7:    // Upper-Right
        case 0x9:    // Center-left
        case 0xA:    // Center-center
        case 0xB:    // Center-right
        case 0xD:    // Lower-left
        case 0xE:    // Lower-center
        case 0xF:    // Lower-right
            cmd->CMDXB += 1;
            cmd->CMDXC += 1;
            cmd->CMDYC += 1;
            cmd->CMDYD += 1;
            break;
        default:
            break;
    }



    int area =
        abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) +
            (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) /
        2;
    switch ((cmd->CMDPMOD >> 3) & 0x7) {
        case 0:
        case 1:
            // 4 pixels per 16 bits
            area = area >> 2;
            break;
        case 2:
        case 3:
        case 4:
            // 2 pixels per 16 bits
            area = area >> 1;
            break;
        default:
            break;
    }
    yabsys.vdp1cycles += MIN(1000, 70 + area);

    // gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 =
                Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }

    VIDCore->Vdp1ScaledSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

static int Vdp1DistortedSpriteDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    Vdp2 *varVdp2Regs = &Vdp2Lines[0];
    int   ret         = 1;

    if (emptyCmd(cmd)) {
        // damaged data
        yabsys.vdp1cycles += 70;
        return 0;
    }

    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip     = (cmd->CMDCTRL & 0x30) >> 4;
    cmd->priority = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);
    CONVERTCMD(cmd->CMDXC);
    CONVERTCMD(cmd->CMDYC);
    CONVERTCMD(cmd->CMDXD);
    CONVERTCMD(cmd->CMDYD);

    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;
    cmd->CMDXB += regs->localX;
    cmd->CMDYB += regs->localY;
    cmd->CMDXC += regs->localX;
    cmd->CMDYC += regs->localY;
    cmd->CMDXD += regs->localX;
    cmd->CMDYD += regs->localY;

    int area =
        abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) +
            (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) /
        2;
    switch ((cmd->CMDPMOD >> 3) & 0x7) {
        case 0:
        case 1:
            // 4 pixels per 16 bits
            area = area >> 2;
            break;
        case 2:
        case 3:
        case 4:
            // 2 pixels per 16 bits
            area = area >> 1;
            break;
        default:
            break;
    }
    yabsys.vdp1cycles += MIN(1000, 70 + (area * 3));

    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 =
                Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }

    VIDCore->Vdp1DistortedSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

static int Vdp1PolygonDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    Vdp2 *varVdp2Regs = &Vdp2Lines[0];

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);
    CONVERTCMD(cmd->CMDXC);
    CONVERTCMD(cmd->CMDYC);
    CONVERTCMD(cmd->CMDXD);
    CONVERTCMD(cmd->CMDYD);

    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;
    cmd->CMDXB += regs->localX;
    cmd->CMDYB += regs->localY;
    cmd->CMDXC += regs->localX;
    cmd->CMDYC += regs->localY;
    cmd->CMDXD += regs->localX;
    cmd->CMDYD += regs->localY;

    int w = (sqrt((cmd->CMDXA - cmd->CMDXB) * (cmd->CMDXA - cmd->CMDXB)) +
             sqrt((cmd->CMDXD - cmd->CMDXC) * (cmd->CMDXD - cmd->CMDXC))) /
            2;
    int h = (sqrt((cmd->CMDYA - cmd->CMDYD) * (cmd->CMDYA - cmd->CMDYD)) +
             sqrt((cmd->CMDYB - cmd->CMDYC) * (cmd->CMDYB - cmd->CMDYC))) /
            2;
    yabsys.vdp1cycles += MIN(1000, 16 + (w * h) + (w * 2));

    // gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 =
                Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }
    cmd->priority = 0;
    cmd->w        = 1;
    cmd->h        = 1;
    cmd->flip     = 0;

    VIDCore->Vdp1PolygonDraw(cmd, ram, regs, back_framebuffer);
    return 1;
}

static int Vdp1PolylineDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{

    Vdp2 *varVdp2Regs = &Vdp2Lines[0];

    cmd->priority = 0;
    cmd->w        = 1;
    cmd->h        = 1;
    cmd->flip     = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);
    CONVERTCMD(cmd->CMDXC);
    CONVERTCMD(cmd->CMDYC);
    CONVERTCMD(cmd->CMDXD);
    CONVERTCMD(cmd->CMDYD);

    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;
    cmd->CMDXB += regs->localX;
    cmd->CMDYB += regs->localY;
    cmd->CMDXC += regs->localX;
    cmd->CMDYC += regs->localY;
    cmd->CMDXD += regs->localX;
    cmd->CMDYD += regs->localY;

    // gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        for (int i = 0; i < 4; i++) {
            u16 color2 =
                Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }
    VIDCore->Vdp1PolylineDraw(cmd, ram, regs, back_framebuffer);

    return 1;
}

static int Vdp1LineDraw(vdp1cmd_struct *cmd, u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    Vdp2 *varVdp2Regs = &Vdp2Lines[0];


    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);

    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;
    cmd->CMDXB += regs->localX;
    cmd->CMDYB += regs->localY;
    cmd->CMDXC = cmd->CMDXB;
    cmd->CMDYC = cmd->CMDYB;
    cmd->CMDXD = cmd->CMDXA;
    cmd->CMDYD = cmd->CMDYA;

    // gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4)) {
        for (int i = 0; i < 4; i++) {
            u16 color2 =
                Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
        }
    }
    cmd->priority = 0;
    cmd->w        = 1;
    cmd->h        = 1;
    cmd->flip     = 0;

    VIDCore->Vdp1LineDraw(cmd, ram, regs, back_framebuffer);

    return 1;
}

static void setupSpriteLimit(vdp1cmdctrl_struct *ctrl)
{
    vdp1cmd_struct *cmd = &ctrl->cmd;
    u32             dot;
    switch ((cmd->CMDPMOD >> 3) & 0x7) {
        case 0: {
            // 4 bpp Bank mode
            ctrl->start_addr = cmd->CMDSRCA * 8;
            ctrl->end_addr   = ctrl->start_addr + MAX(1, cmd->h) * MAX(1, cmd->w) / 2;
            break;
        }
        case 1: {
            // 4 bpp LUT mode
            u32 colorLut     = cmd->CMDCOLR * 8;
            u32 charAddr     = cmd->CMDSRCA * 8;
            ctrl->start_addr = cmd->CMDSRCA * 8;
            ctrl->end_addr   = ctrl->start_addr + MAX(1, cmd->h) * MAX(1, cmd->w) / 2;

            for (int i = 0; i < MAX(1, cmd->h); i++) {
                u16 j;
                j = 0;
                while (j < MAX(1, cmd->w) / 2) {
                    dot              = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
                    int lutaddr      = (dot >> 4) * 2 + colorLut;
                    ctrl->start_addr = (ctrl->start_addr > lutaddr) ? lutaddr : ctrl->start_addr;
                    ctrl->end_addr   = (ctrl->end_addr < lutaddr) ? lutaddr : ctrl->end_addr;
                    charAddr += 1;
                    j += 1;
                }
            }
            break;
        }
        case 2:
        case 3:
        case 4: {
            // 8 bpp(64 color) Bank mode
            ctrl->start_addr = cmd->CMDSRCA * 8;
            ctrl->end_addr   = ctrl->start_addr + MAX(1, cmd->h) * MAX(1, cmd->w);
            break;
        }
        case 5: {
            // 16 bpp Bank mode
            // 8 bpp(64 color) Bank mode
            ctrl->start_addr = cmd->CMDSRCA * 8;
            ctrl->end_addr   = ctrl->start_addr + MAX(1, cmd->h) * MAX(1, cmd->w) * 2;
            break;
        }
        default:
            VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
            break;
    }
}

static int getVdp1CyclesPerLine(void)
{
    int clock = 26842600;
    int fps   = 60;
    // Using p37, Table 4.2 of vdp1 official doc
    if (yabsys.IsPal) {
        fps = 50;
        // Horizontal Resolution
        switch (Vdp2Lines[0].TVMD & 0x7) {
            case 0:
            case 2:
            case 4:
            case 6:
                // W is 320 or 640
                clock = 26656400;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                // W is 352 or 704
                clock = 28437500;
                break;
        }
    } else {
        // Horizontal Resolution
        switch (Vdp2Lines[0].TVMD & 0x7) {
            case 0:
            case 2:
            case 4:
            case 6:
                // W is 320 or 640
                clock = 26842600;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                // W is 352 or 704
                clock = 28636400;
                break;
        }
    }
    return clock / (fps * yabsys.MaxLineCount);
}

static u32             returnAddr    = 0xffffffff;
static vdp1cmd_struct *usrClipCmd    = NULL;
static vdp1cmd_struct *sysClipCmd    = NULL;
static vdp1cmd_struct *localCoordCmd = NULL;

#ifdef DEBUG_CMD_LIST
void debugCmdList()
{
    YuiMsg("Draw %d (%d)\n", yabsys.LineCount, _Ygl->drawframe);
    for (int i = 0;; i++) {
        char *string;

        if ((string = Vdp1DebugGetCommandNumberName(i)) == NULL)
            break;

        YuiMsg("\t%s\n", string);
    }
}
#endif
static u32 Vdp1DebugGetCommandNumberAddr(u32 number);

int EvaluateCmdListHash(Vdp1 *regs)
{
    int hash           = 0;
    u32 addr           = 0;
    u32 returnAddr     = 0xFFFFFFFF;
    u32 commandCounter = 0;
    u16 command;

    command = T1ReadWord(Vdp1Ram, addr);

    while (!(command & 0x8000) && (commandCounter < 2000)) {
        vdp1cmd_struct cmd;
        // Make sure we're still dealing with a valid command
        if ((command & 0x000C) == 0x000C)
            // Invalid, abort
            return hash;
        Vdp1ReadCommand(&cmd, addr, Vdp1Ram);
        hash ^= (cmd.CMDCTRL << 16) | cmd.CMDLINK;
        hash ^= (cmd.CMDPMOD << 16) | cmd.CMDCOLR;
        hash ^= (cmd.CMDSRCA << 16) | cmd.CMDSIZE;
        hash ^= (cmd.CMDXA << 16) | cmd.CMDYA;
        hash ^= (cmd.CMDXB << 16) | cmd.CMDYB;
        hash ^= (cmd.CMDXC << 16) | cmd.CMDYC;
        hash ^= (cmd.CMDXD << 16) | cmd.CMDYD;
        hash ^= (cmd.CMDGRDA << 16) | _Ygl->drawframe;

        // Determine where to go next
        switch ((command & 0x3000) >> 12) {
            case 0:    // NEXT, jump to following table
                addr += 0x20;
                break;
            case 1:    // ASSIGN, jump to CMDLINK
                addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
                break;
            case 2:    // CALL, call a subroutine
                if (returnAddr == 0xFFFFFFFF)
                    returnAddr = addr + 0x20;

                addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
                break;
            case 3:    // RETURN, return from subroutine
                if (returnAddr != 0xFFFFFFFF) {
                    addr       = returnAddr;
                    returnAddr = 0xFFFFFFFF;
                } else
                    addr += 0x20;
                break;
        }

        if (addr > 0x7FFE0)
            return hash;
        command = T1ReadWord(Vdp1Ram, addr);
        commandCounter++;
    }
    return hash;
}

static int sameCmd(vdp1cmd_struct *a, vdp1cmd_struct *b)
{
    if (a == NULL)
        return 0;
    if (b == NULL)
        return 0;
    if (emptyCmd(a))
        return 0;
    int cmp = memcmp(a, b, 15 * sizeof(int));
    if (cmp == 0) {
        return 1;
    }
    return 0;
}

static int lastHash = -1;
void       Vdp1DrawCommands(u8 *ram, Vdp1 *regs, u8 *back_framebuffer)
{
    int cylesPerLine = getVdp1CyclesPerLine();

    if (Vdp1External.status == VDP1_STATUS_IDLE) {
#if 0
    int newHash = EvaluateCmdListHash(regs);
    // Breaks megamanX4
    if (newHash == lastHash) {
#ifdef DEBUG_CMD_LIST
      YuiMsg("Abort same command %x %x (%d) (%d)\n", newHash, lastHash, _Ygl->drawframe, yabsys.LineCount);
#endif
      return;
    }
    lastHash = newHash;
    YuiMsg("The last list is 0x%x (%d) (%d)\n", newHash, _Ygl->drawframe, yabsys.LineCount);
#endif
#ifdef DEBUG_CMD_LIST
        debugCmdList();
#endif

        returnAddr = 0xffffffff;
        if (usrClipCmd != NULL)
            free(usrClipCmd);
        if (sysClipCmd != NULL)
            free(sysClipCmd);
        if (localCoordCmd != NULL)
            free(localCoordCmd);
        usrClipCmd     = NULL;
        sysClipCmd     = NULL;
        localCoordCmd  = NULL;
        nbCmdToProcess = 0;
    }
    CmdListLimit = 0;

    Vdp1External.status = VDP1_STATUS_RUNNING;
    if (regs->addr > 0x7FFFF) {
        Vdp1External.status = VDP1_STATUS_IDLE;
        return;    // address error
    }

    u16 command        = Vdp1RamReadWord(NULL, ram, regs->addr);
    u32 commandCounter = 0;

    Vdp1External.updateVdp1Ram = 0;
    vdp1Ram_update_start       = 0x80000;
    vdp1Ram_update_end         = 0x0;
    Vdp1External.checkEDSR     = 0;

    vdp1cmd_struct oldCmd;

    yabsys.vdp1cycles = 0;
    while (!(command & 0x8000) && nbCmdToProcess < CMD_QUEUE_SIZE) {    // fix me
        int ret;
        regs->COPR = (regs->addr & 0x7FFFF) >> 3;
        // First, process the command
        if (!(command & 0x4000)) {    // if (!skip)
            vdp1cmdctrl_struct *ctrl = NULL;
            int                 ret;
            if (vdp1_clock <= 0) {
                // No more clock cycle, wait next line
                return;
            }
            switch (command & 0x000F) {
                case 0:    // normal sprite draw
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        ret = Vdp1NormalSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        if (ret == 1)
                            nbCmdToProcess++;
                        else
                            vdp1_clock = 0;    // Incorrect command, wait next line to continue
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 1:    // scaled sprite draw
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        ret = Vdp1ScaledSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        if (ret == 1)
                            nbCmdToProcess++;
                        else
                            vdp1_clock = 0;    // Incorrect command, wait next line to continue
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 2:    // distorted sprite draw
                case 3:    /* this one should be invalid, but some games
                           (Hardcore 4x4 for instance) use it instead of 2 */
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        ret = Vdp1DistortedSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        if (ret == 1)
                            nbCmdToProcess++;
                        else
                            vdp1_clock = 0;    // Incorrect command, wait next line to continue
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 4:    // polygon draw
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        nbCmdToProcess += Vdp1PolygonDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 5:    // polyline draw
                case 7:    // undocumented mirror
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        nbCmdToProcess += Vdp1PolylineDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 6:    // line draw
                    ctrl        = &cmdBufferBeingProcessed[nbCmdToProcess];
                    ctrl->dirty = 0;
                    Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
                    if (!sameCmd(&ctrl->cmd, &oldCmd)) {
                        oldCmd = ctrl->cmd;
                        ctrl->ignitionLine =
                            MIN(yabsys.LineCount + yabsys.vdp1cycles / cylesPerLine, yabsys.MaxLineCount - 1);
                        checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                        nbCmdToProcess += Vdp1LineDraw(&ctrl->cmd, ram, regs, back_framebuffer);
                        setupSpriteLimit(ctrl);
                    }
                    break;
                case 8:     // user clipping coordinates
                case 11:    // undocumented mirror
                    checkClipCmd(&sysClipCmd, NULL, &localCoordCmd, ram, regs);
                    yabsys.vdp1cycles += 16;
                    usrClipCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
                    Vdp1ReadCommand(usrClipCmd, regs->addr, ram);
                    oldCmd = *usrClipCmd;
                    break;
                case 9:    // system clipping coordinates
                    checkClipCmd(NULL, &usrClipCmd, &localCoordCmd, ram, regs);
                    yabsys.vdp1cycles += 16;
                    sysClipCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
                    Vdp1ReadCommand(sysClipCmd, regs->addr, ram);
                    oldCmd = *sysClipCmd;
                    break;
                case 10:    // local coordinate
                    checkClipCmd(&sysClipCmd, &usrClipCmd, NULL, ram, regs);
                    yabsys.vdp1cycles += 16;
                    localCoordCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
                    Vdp1ReadCommand(localCoordCmd, regs->addr, ram);
                    oldCmd = *localCoordCmd;
                    break;
                default:    // Abort
                    VDP1LOG("vdp1\t: Bad command: %x\n", command);
                    checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                    Vdp1External.status = VDP1_STATUS_IDLE;
                    regs->EDSR |= 2;
                    regs->COPR   = (regs->addr & 0x7FFFF) >> 3;
                    CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
                    return;
            }
        } else {
            yabsys.vdp1cycles += 16;
        }
        vdp1_clock -= yabsys.vdp1cycles;
        yabsys.vdp1cycles = 0;

        // Force to quit internal command error( This technic(?) is used by BATSUGUN )
        if (regs->EDSR & 0x02) {
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            Vdp1External.status = VDP1_STATUS_IDLE;
            regs->COPR          = (regs->addr & 0x7FFFF) >> 3;
            CmdListLimit        = MAX((regs->addr & 0x7FFFF), regs->addr);
            return;
        }

        // Next, determine where to go next
        switch ((command & 0x3000) >> 12) {
            case 0:    // NEXT, jump to following table
                regs->addr += 0x20;
                break;
            case 1:    // ASSIGN, jump to CMDLINK
            {
                u32 oldAddr = regs->addr;
                regs->addr  = T1ReadWord(ram, regs->addr + 2) * 8;
                if (((regs->addr == oldAddr) && (command & 0x4000)) || (regs->addr == 0)) {
                    // The next adress is the same as the old adress and the command is skipped => Exit
                    // The next adress is the start of the command list. It means the list has an infinte loop => Exit
                    // (used by Burning Rangers)
                    regs->lCOPR   = (regs->addr & 0x7FFFF) >> 3;
                    vdp1_clock    = 0;
                    CmdListInLoop = 1;
                    CmdListLimit  = MAX((regs->addr & 0x7FFFF), regs->addr);
                    checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
                    return;
                }
            } break;
            case 2:    // CALL, call a subroutine
                if (returnAddr == 0xFFFFFFFF)
                    returnAddr = regs->addr + 0x20;

                regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
                break;
            case 3:    // RETURN, return from subroutine
                if (returnAddr != 0xFFFFFFFF) {
                    regs->addr = returnAddr;
                    returnAddr = 0xFFFFFFFF;
                } else
                    regs->addr += 0x20;
                break;
        }

        command      = Vdp1RamReadWord(NULL, ram, regs->addr);
        CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
        // If we change directly CPR to last value, scorcher will not boot.
        // If we do not change it, Noon will not start
        // So store the value and update COPR with last value at VBlank In
        regs->lCOPR = (regs->addr & 0x7FFFF) >> 3;
    }
    if (command & 0x8000) {
        LOG("VDP1: Command Finished! count = %d @ %08X", commandCounter, regs->addr);
        Vdp1External.status = VDP1_STATUS_IDLE;
        regs->COPR          = (regs->addr & 0x7FFFF) >> 3;
        regs->lCOPR         = (regs->addr & 0x7FFFF) >> 3;
    }
    CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
    checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
}

// ensure that registers are set correctly
void Vdp1FakeDrawCommands(u8 *ram, Vdp1 *regs)
{
    u16            command        = T1ReadWord(ram, regs->addr);
    u32            commandCounter = 0;
    u32            returnAddr     = 0xffffffff;
    vdp1cmd_struct cmd;

    while (!(command & 0x8000) && commandCounter < 2000) {    // fix me
        // First, process the command
        if (!(command & 0x4000)) {    // if (!skip)
            switch (command & 0x000F) {
                case 0:    // normal sprite draw
                case 1:    // scaled sprite draw
                case 2:    // distorted sprite draw
                case 3:    /* this one should be invalid, but some games
                           (Hardcore 4x4 for instance) use it instead of 2 */
                case 4:    // polygon draw
                case 5:    // polyline draw
                case 6:    // line draw
                case 7:    // undocumented polyline draw mirror
                    break;
                case 8:     // user clipping coordinates
                case 11:    // undocumented mirror
                    Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
                    VIDCore->Vdp1UserClipping(&cmd, ram, regs);
                    break;
                case 9:    // system clipping coordinates
                    Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
                    VIDCore->Vdp1SystemClipping(&cmd, ram, regs);
                    break;
                case 10:    // local coordinate
                    Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
                    VIDCore->Vdp1LocalCoordinate(&cmd, ram, regs);
                    break;
                default:    // Abort
                    VDP1LOG("vdp1\t: Bad command: %x\n", command);
                    regs->EDSR |= 2;
                    regs->COPR = regs->addr >> 3;
                    return;
            }
        }

        // Next, determine where to go next
        switch ((command & 0x3000) >> 12) {
            case 0:    // NEXT, jump to following table
                regs->addr += 0x20;
                break;
            case 1:    // ASSIGN, jump to CMDLINK
                regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
                break;
            case 2:    // CALL, call a subroutine
                if (returnAddr == 0xFFFFFFFF)
                    returnAddr = regs->addr + 0x20;

                regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
                break;
            case 3:    // RETURN, return from subroutine
                if (returnAddr != 0xFFFFFFFF) {
                    regs->addr = returnAddr;
                    returnAddr = 0xFFFFFFFF;
                } else
                    regs->addr += 0x20;
                break;
        }

        command = T1ReadWord(ram, regs->addr);
        commandCounter++;
    }
}

static int Vdp1Draw(void)
{
    FRAMELOG("Vdp1Draw\n");
    if (!Vdp1External.disptoggle) {
        Vdp1Regs->EDSR >>= 1;
        Vdp1NoDraw();
    } else {
        if (Vdp1External.status == VDP1_STATUS_IDLE) {
            Vdp1Regs->EDSR >>= 1;
            Vdp1Regs->addr = 0;

            // beginning of a frame
            // BEF <- CEF
            // CEF <- 0
            // Vdp1Regs->EDSR >>= 1;
            /* this should be done after a frame change or a plot trigger */
            Vdp1Regs->COPR  = 0;
            Vdp1Regs->lCOPR = 0;
        }
        VIDCore->Vdp1Draw();
    }
    if (Vdp1External.status == VDP1_STATUS_IDLE) {
        FRAMELOG("Vdp1Draw end at %d line\n", yabsys.LineCount);
        Vdp1Regs->EDSR |= 2;
        ScuSendDrawEnd();
    }
    if (Vdp1External.status == VDP1_STATUS_IDLE)
        return 0;
    else
        return 1;
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp1NoDraw(void)
{
    // beginning of a frame (ST-013-R3-061694 page 53)
    // BEF <- CEF
    // CEF <- 0
    // Vdp1Regs->EDSR >>= 1;
    /* this should be done after a frame change or a plot trigger */
    Vdp1Regs->COPR                = 0;
    Vdp1Regs->lCOPR               = 0;
    _Ygl->vdp1On[_Ygl->drawframe] = 0;
    Vdp1FakeDrawCommands(Vdp1Ram, Vdp1Regs);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr, u8 *ram)
{
    cmd->CMDCTRL = T1ReadWord(ram, addr);
    cmd->CMDLINK = T1ReadWord(ram, addr + 0x2);
    cmd->CMDPMOD = T1ReadWord(ram, addr + 0x4);
    cmd->CMDCOLR = T1ReadWord(ram, addr + 0x6);
    cmd->CMDSRCA = T1ReadWord(ram, addr + 0x8);
    cmd->CMDSIZE = T1ReadWord(ram, addr + 0xA);
    cmd->CMDXA   = T1ReadWord(ram, addr + 0xC);
    cmd->CMDYA   = T1ReadWord(ram, addr + 0xE);
    cmd->CMDXB   = T1ReadWord(ram, addr + 0x10);
    cmd->CMDYB   = T1ReadWord(ram, addr + 0x12);
    cmd->CMDXC   = T1ReadWord(ram, addr + 0x14);
    cmd->CMDYC   = T1ReadWord(ram, addr + 0x16);
    cmd->CMDXD   = T1ReadWord(ram, addr + 0x18);
    cmd->CMDYD   = T1ReadWord(ram, addr + 0x1A);
    cmd->CMDGRDA = T1ReadWord(ram, addr + 0x1C);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp1SaveState(void **stream)
{
    int offset;
#ifdef IMPROVED_SAVESTATES
    int i                         = 0;
    u8  back_framebuffer[0x40000] = {0};
#endif

    offset = MemStateWriteHeader(stream, "VDP1", 2);

    // Write registers
    MemStateWrite((void *)Vdp1Regs, sizeof(Vdp1), 1, stream);

    // Write VDP1 ram
    MemStateWrite((void *)Vdp1Ram, 0x80000, 1, stream);

#ifdef IMPROVED_SAVESTATES
    for (i = 0; i < 0x40000; i++)
        back_framebuffer[i] = Vdp1FrameBufferReadByte(NULL, NULL, i);

    MemStateWrite((void *)back_framebuffer, 0x40000, 1, stream);
#endif

    // VDP1 status
    int size = sizeof(Vdp1External_struct);
    MemStateWrite((void *)(&size), sizeof(int), 1, stream);
    MemStateWrite((void *)(&Vdp1External), sizeof(Vdp1External_struct), 1, stream);
    return MemStateFinishHeader(stream, offset);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp1LoadState(const void *stream, UNUSED int version, int size)
{
#ifdef IMPROVED_SAVESTATES
    int i                         = 0;
    u8  back_framebuffer[0x40000] = {0};
#endif

    // Read registers
    MemStateRead((void *)Vdp1Regs, sizeof(Vdp1), 1, stream);

    // Read VDP1 ram
    MemStateRead((void *)Vdp1Ram, 0x80000, 1, stream);
    vdp1Ram_update_start = 0x80000;
    vdp1Ram_update_end   = 0x0;
#ifdef IMPROVED_SAVESTATES
    MemStateRead((void *)back_framebuffer, 0x40000, 1, stream);

    for (i = 0; i < 0x40000; i++)
        Vdp1FrameBufferWriteByte(NULL, NULL, i, back_framebuffer[i]);
#endif
    if (version > 1) {
        int size = 0;
        MemStateRead((void *)(&size), sizeof(int), 1, stream);
        if (size == sizeof(Vdp1External_struct)) {
            MemStateRead((void *)(&Vdp1External), sizeof(Vdp1External_struct), 1, stream);
        } else {
            YuiMsg("Too old savestate, can not restore Vdp1External\n");
            memset((void *)(&Vdp1External), 0, sizeof(Vdp1External_struct));
            Vdp1External.disptoggle = 1;
        }
    } else {
        YuiMsg("Too old savestate, can not restore Vdp1External\n");
        memset((void *)(&Vdp1External), 0, sizeof(Vdp1External_struct));
        Vdp1External.disptoggle = 1;
    }
    Vdp1External.updateVdp1Ram = 1;

    return size;
}

//////////////////////////////////////////////////////////////////////////////

static u32 Vdp1DebugGetCommandNumberAddr(u32 number)
{
    u32 addr           = 0;
    u32 returnAddr     = 0xFFFFFFFF;
    u32 commandCounter = 0;
    u16 command;

    command = T1ReadWord(Vdp1Ram, addr);

    while (!(command & 0x8000) && commandCounter != number) {
        // Make sure we're still dealing with a valid command
        if ((command & 0x000C) == 0x000C)
            // Invalid, abort
            return 0xFFFFFFFF;

        // Determine where to go next
        switch ((command & 0x3000) >> 12) {
            case 0:    // NEXT, jump to following table
                addr += 0x20;
                break;
            case 1:    // ASSIGN, jump to CMDLINK
                addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
                break;
            case 2:    // CALL, call a subroutine
                if (returnAddr == 0xFFFFFFFF)
                    returnAddr = addr + 0x20;

                addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
                break;
            case 3:    // RETURN, return from subroutine
                if (returnAddr != 0xFFFFFFFF) {
                    addr       = returnAddr;
                    returnAddr = 0xFFFFFFFF;
                } else
                    addr += 0x20;
                break;
        }

        if (addr > 0x7FFE0)
            return 0xFFFFFFFF;
        command = T1ReadWord(Vdp1Ram, addr);
        commandCounter++;
    }

    if (commandCounter == number)
        return addr;
    else
        return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

Vdp1CommandType Vdp1DebugGetCommandType(u32 number)
{
    u32 addr;
    if ((addr = Vdp1DebugGetCommandNumberAddr(number)) != 0xFFFFFFFF) {
        const u16 command = T1ReadWord(Vdp1Ram, addr);
        if (command & 0x8000)
            return VDPCT_DRAW_END;
        else if ((command & 0x000F) < VDPCT_INVALID)
            return (Vdp1CommandType)(command & 0x000F);
    }

    return VDPCT_INVALID;
}


char *Vdp1DebugGetCommandNumberName(u32 number)
{
    u32 addr;
    u16 command;

    if ((addr = Vdp1DebugGetCommandNumberAddr(number)) != 0xFFFFFFFF) {
        command = T1ReadWord(Vdp1Ram, addr);

        if (command & 0x8000)
            return "Draw End";

        // Figure out command name
        switch (command & 0x000F) {
            case 0:
                return "Normal Sprite";
            case 1:
                return "Scaled Sprite";
            case 2:
                return "Distorted Sprite";
            case 3:
                return "Distorted Sprite *";
            case 4:
                return "Polygon";
            case 5:
                return "Polyline";
            case 6:
                return "Line";
            case 7:
                return "Polyline *";
            case 8:
                return "User Clipping Coordinates";
            case 9:
                return "System Clipping Coordinates";
            case 10:
                return "Local Coordinates";
            case 11:
                return "User Clipping Coordinates *";
            default:
                return "Bad command";
        }
    } else
        return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1DebugCommand(u32 number, char *outstring)
{
    u16            command;
    vdp1cmd_struct cmd;
    u32            addr;

    if ((addr = Vdp1DebugGetCommandNumberAddr(number)) == 0xFFFFFFFF)
        return;

    command = T1ReadWord(Vdp1Ram, addr);

    if (command & 0x8000) {
        // Draw End
        outstring[0] = 0x00;
        return;
    }

    if (command & 0x4000) {
        AddString(outstring, "Command is skipped\r\n");
        return;
    }

    Vdp1ReadCommand(&cmd, addr, Vdp1Ram);

    if ((cmd.CMDCTRL & 0x000F) < 4) {
        int w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
        int h = cmd.CMDSIZE & 0xFF;
    }

    if ((cmd.CMDYA & 0x400))
        cmd.CMDYA |= 0xFC00;
    else
        cmd.CMDYA &= ~(0xFC00);
    if ((cmd.CMDYC & 0x400))
        cmd.CMDYC |= 0xFC00;
    else
        cmd.CMDYC &= ~(0xFC00);
    if ((cmd.CMDYB & 0x400))
        cmd.CMDYB |= 0xFC00;
    else
        cmd.CMDYB &= ~(0xFC00);
    if ((cmd.CMDYD & 0x400))
        cmd.CMDYD |= 0xFC00;
    else
        cmd.CMDYD &= ~(0xFC00);

    if ((cmd.CMDXA & 0x400))
        cmd.CMDXA |= 0xFC00;
    else
        cmd.CMDXA &= ~(0xFC00);
    if ((cmd.CMDXC & 0x400))
        cmd.CMDXC |= 0xFC00;
    else
        cmd.CMDXC &= ~(0xFC00);
    if ((cmd.CMDXB & 0x400))
        cmd.CMDXB |= 0xFC00;
    else
        cmd.CMDXB &= ~(0xFC00);
    if ((cmd.CMDXD & 0x400))
        cmd.CMDXD |= 0xFC00;
    else
        cmd.CMDXD &= ~(0xFC00);

    switch (cmd.CMDCTRL & 0x000F) {
        case 0:
            AddString(outstring, "Normal Sprite\r\n");
            AddString(outstring, "x = %d, y = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA);
            break;
        case 1:
            AddString(outstring, "Scaled Sprite\r\n");

            AddString(outstring, "Zoom Point: ");

            switch ((cmd.CMDCTRL >> 8) & 0xF) {
                case 0x0:
                    AddString(outstring, "Only two coordinates\r\n");
                    break;
                case 0x5:
                    AddString(outstring, "Upper-left\r\n");
                    break;
                case 0x6:
                    AddString(outstring, "Upper-center\r\n");
                    break;
                case 0x7:
                    AddString(outstring, "Upper-right\r\n");
                    break;
                case 0x9:
                    AddString(outstring, "Center-left\r\n");
                    break;
                case 0xA:
                    AddString(outstring, "Center-center\r\n");
                    break;
                case 0xB:
                    AddString(outstring, "Center-right\r\n");
                    break;
                case 0xC:
                    AddString(outstring, "Lower-left\r\n");
                    break;
                case 0xE:
                    AddString(outstring, "Lower-center\r\n");
                    break;
                case 0xF:
                    AddString(outstring, "Lower-right\r\n");
                    break;
                default:
                    break;
            }

            if (((cmd.CMDCTRL >> 8) & 0xF) == 0) {
                AddString(outstring, "xa = %d, ya = %d, xc = %d, yc = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                          (s16)cmd.CMDXC, (s16)cmd.CMDYC);
            } else {
                AddString(outstring, "xa = %d, ya = %d, xb = %d, yb = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                          (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            }

            break;
        case 2:
            AddString(outstring, "Distorted Sprite\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC,
                      (s16)cmd.CMDXD, (s16)cmd.CMDYD);
            break;
        case 3:
            AddString(outstring, "Distorted Sprite *\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC,
                      (s16)cmd.CMDXD, (s16)cmd.CMDYD);
            break;
        case 4:
            AddString(outstring, "Polygon\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC,
                      (s16)cmd.CMDXD, (s16)cmd.CMDYD);
            break;
        case 5:
            AddString(outstring, "Polyline\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC,
                      (s16)cmd.CMDXD, (s16)cmd.CMDYD);
            break;
        case 6:
            AddString(outstring, "Line\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            break;
        case 7:
            AddString(outstring, "Polyline *\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXB, (s16)cmd.CMDYB);
            AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC,
                      (s16)cmd.CMDXD, (s16)cmd.CMDYD);
            break;
        case 8:
            AddString(outstring, "User Clipping\r\n");
            AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA,
                      (s16)cmd.CMDXC, (s16)cmd.CMDYC);
            break;
        case 9:
            AddString(outstring, "System Clipping\r\n");
            AddString(outstring, "x1 = 0, y1 = 0, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC);
            break;
        case 10:
            AddString(outstring, "Local Coordinates\r\n");
            AddString(outstring, "x = %d, y = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA);
            break;
        default:
            AddString(outstring, "Invalid command\r\n");
            return;
    }

    // Only Sprite commands use CMDSRCA, CMDSIZE
    if (!(cmd.CMDCTRL & 0x000C)) {
        AddString(outstring, "Texture address = %08X\r\n", ((unsigned int)cmd.CMDSRCA) << 3);
        AddString(outstring, "Texture width = %d, height = %d\r\n", MAX(1, (cmd.CMDSIZE & 0x3F00) >> 5),
                  MAX(1, cmd.CMDSIZE & 0xFF));
        if ((((cmd.CMDSIZE & 0x3F00) >> 5) == 0) || ((cmd.CMDSIZE & 0xFF) == 0))
            AddString(outstring, "Texture malformed \r\n");
        AddString(outstring, "Texture read direction: ");

        switch ((cmd.CMDCTRL >> 4) & 0x3) {
            case 0:
                AddString(outstring, "Normal\r\n");
                break;
            case 1:
                AddString(outstring, "Reversed horizontal\r\n");
                break;
            case 2:
                AddString(outstring, "Reversed vertical\r\n");
                break;
            case 3:
                AddString(outstring, "Reversed horizontal and vertical\r\n");
                break;
            default:
                break;
        }
    }

    // Only draw commands use CMDPMOD
    if (!(cmd.CMDCTRL & 0x0008)) {
        if (cmd.CMDPMOD & 0x8000) {
            AddString(outstring, "MSB set\r\n");
        }

        if (cmd.CMDPMOD & 0x1000) {
            AddString(outstring, "High Speed Shrink Enabled\r\n");
        }

        if (!(cmd.CMDPMOD & 0x0800)) {
            AddString(outstring, "Pre-clipping Enabled\r\n");
        }

        if (cmd.CMDPMOD & 0x0400) {
            AddString(outstring, "User Clipping Enabled\r\n");
            AddString(outstring, "Clipping Mode = %d\r\n", (cmd.CMDPMOD >> 9) & 0x1);
        }

        if (cmd.CMDPMOD & 0x0100) {
            AddString(outstring, "Mesh Enabled\r\n");
        }

        if (!(cmd.CMDPMOD & 0x0080)) {
            AddString(outstring, "End Code Enabled\r\n");
        }

        if (!(cmd.CMDPMOD & 0x0040)) {
            AddString(outstring, "Transparent Pixel Enabled\r\n");
        }

        if (cmd.CMDCTRL & 0x0004) {
            AddString(outstring, "Non-textured color: %04X\r\n", cmd.CMDCOLR);
        } else {
            AddString(outstring, "Color mode: ");

            switch ((cmd.CMDPMOD >> 3) & 0x7) {
                case 0:
                    AddString(outstring, "4 BPP(16 color bank)\r\n");
                    AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                    break;
                case 1:
                    AddString(outstring, "4 BPP(16 color LUT)\r\n");
                    AddString(outstring, "Color lookup table: %08X\r\n", (cmd.CMDCOLR));
                    break;
                case 2:
                    AddString(outstring, "8 BPP(64 color bank)\r\n");
                    AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                    break;
                case 3:
                    AddString(outstring, "8 BPP(128 color bank)\r\n");
                    AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                    break;
                case 4:
                    AddString(outstring, "8 BPP(256 color bank)\r\n");
                    AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                    break;
                case 5:
                    AddString(outstring, "15 BPP(RGB)\r\n");
                    break;
                default:
                    break;
            }
        }

        AddString(outstring, "Color Calc. mode: ");

        switch (cmd.CMDPMOD & 0x7) {
            case 0:
                AddString(outstring, "Replace\r\n");
                break;
            case 1:
                AddString(outstring, "Cannot overwrite/Shadow\r\n");
                break;
            case 2:
                AddString(outstring, "Half-luminance\r\n");
                break;
            case 3:
                AddString(outstring, "Replace/Half-transparent\r\n");
                break;
            case 4:
                AddString(outstring, "Gouraud Shading\r\n");
                AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
                break;
            case 6:
                AddString(outstring, "Gouraud Shading + Half-luminance\r\n");
                AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
                break;
            case 7:
                AddString(outstring, "Gouraud Shading/Gouraud Shading + Half-transparent\r\n");
                AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
                break;
            default:
                break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

static u32 ColorRamGetColor(u32 colorindex)
{
    switch (Vdp2Internal.ColorMode) {
        case 0:
        case 1: {
            u32 tmp;
            colorindex <<= 1;
            tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
            return SAT2YAB1(0xFF, tmp);
        }
        case 2: {
            u32 tmp1, tmp2;
            colorindex <<= 2;
            colorindex &= 0xFFF;
            tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
            tmp2 = T2ReadWord(Vdp2ColorRam, colorindex + 2);
            return SAT2YAB2(0xFF, tmp1, tmp2);
        }
        default:
            break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int CheckEndcode(int dot, int endcode, int *code)
{
    if (dot == endcode) {
        code[0]++;
        if (code[0] == 2) {
            code[0] = 0;
            return 2;
        }
        return 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int DoEndcode(int count, u32 *charAddr, u32 **textdata, int width, int xoff, int oddpixel, int pixelsize)
{
    if (count > 1) {
        float divisor = (float)(8 / pixelsize);

        if (divisor != 0)
            charAddr[0] += (int)((float)(width - xoff + oddpixel) / divisor);
        memset(textdata[0], 0, sizeof(u32) * (width - xoff));
        textdata[0] += (width - xoff);
        return 1;
    } else
        *textdata[0]++ = 0;

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 *Vdp1DebugTexture(u32 number, int *w, int *h)
{
    u16            command;
    vdp1cmd_struct cmd;
    u32            addr;
    u32           *texture;
    u32            charAddr;
    u32            dot;
    u8             SPD;
    u32            alpha;
    u32           *textdata;
    int            isendcode = 0;
    int            code      = 0;
    int            ret;

    if ((addr = Vdp1DebugGetCommandNumberAddr(number)) == 0xFFFFFFFF)
        return NULL;

    command = T1ReadWord(Vdp1Ram, addr);

    if (command & 0x8000)
        // Draw End
        return NULL;

    if (command & 0x4000)
        // Command Skipped
        return NULL;

    Vdp1ReadCommand(&cmd, addr, Vdp1Ram);

    switch (cmd.CMDCTRL & 0x000F) {
        case 0:    // Normal Sprite
        case 1:    // Scaled Sprite
        case 2:    // Distorted Sprite
        case 3:    // Distorted Sprite *
            w[0] = MAX(1, (cmd.CMDSIZE & 0x3F00) >> 5);
            h[0] = MAX(1, cmd.CMDSIZE & 0xFF);

            if ((texture = (u32 *)malloc(sizeof(u32) * w[0] * h[0])) == NULL)
                return NULL;

            if (!(cmd.CMDPMOD & 0x80)) {
                isendcode = 1;
                code      = 0;
            } else
                isendcode = 0;
            break;
        case 4:    // Polygon
        case 5:    // Polyline
        case 6:    // Line
        case 7:    // Polyline *
            // Do 1x1 pixel
            w[0] = 1;
            h[0] = 1;
            if ((texture = (u32 *)malloc(sizeof(u32))) == NULL)
                return NULL;

            if (cmd.CMDCOLR & 0x8000)
                texture[0] = SAT2YAB1(0xFF, cmd.CMDCOLR);
            else
                texture[0] = ColorRamGetColor(cmd.CMDCOLR);

            return texture;
        case 8:     // User Clipping
        case 9:     // System Clipping
        case 10:    // Local Coordinates
        case 11:    // User Clipping *
            return NULL;
        default:    // Invalid command
            return NULL;
    }

    charAddr = cmd.CMDSRCA * 8;
    SPD      = ((cmd.CMDPMOD & 0x40) != 0);
    alpha    = 0xFF;
    textdata = texture;

    switch ((cmd.CMDPMOD >> 3) & 0x7) {
        case 0: {
            // 4 bpp Bank mode
            u32 colorBank   = cmd.CMDCOLR;
            u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
            u16 i;

            for (i = 0; i < h[0]; i++) {
                u16 j;
                j = 0;
                while (j < w[0]) {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

                    // Pixel 1
                    if (isendcode && (ret = CheckEndcode(dot >> 4, 0xF, &code)) > 0) {
                        if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 4))
                            break;
                    } else {
                        if (((dot >> 4) == 0) && !SPD)
                            *textdata++ = 0;
                        else
                            *textdata++ = ColorRamGetColor(((dot >> 4) | colorBank) + colorOffset);
                    }

                    j += 1;

                    // Pixel 2
                    if (isendcode && (ret = CheckEndcode(dot & 0xF, 0xF, &code)) > 0) {
                        if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 1, 4))
                            break;
                    } else {
                        if (((dot & 0xF) == 0) && !SPD)
                            *textdata++ = 0;
                        else
                            *textdata++ = ColorRamGetColor(((dot & 0xF) | colorBank) + colorOffset);
                    }

                    j += 1;
                    charAddr += 1;
                }
            }
            break;
        }
        case 1: {
            // 4 bpp LUT mode
            u32 temp;
            u32 colorLut = cmd.CMDCOLR * 8;
            u16 i;

            for (i = 0; i < h[0]; i++) {
                u16 j;
                j = 0;
                while (j < w[0]) {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

                    if (isendcode && (ret = CheckEndcode(dot >> 4, 0xF, &code)) > 0) {
                        if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 4))
                            break;
                    } else {
                        if (((dot >> 4) == 0) && !SPD)
                            *textdata++ = 0;
                        else {
                            temp = T1ReadWord(Vdp1Ram, ((dot >> 4) * 2 + colorLut) & 0x7FFFF);
                            if (temp & 0x8000)
                                *textdata++ = SAT2YAB1(0xFF, temp);
                            else
                                *textdata++ = ColorRamGetColor(temp);
                        }
                    }

                    j += 1;

                    if (isendcode && (ret = CheckEndcode(dot & 0xF, 0xF, &code)) > 0) {
                        if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 1, 4))
                            break;
                    } else {
                        if (((dot & 0xF) == 0) && !SPD)
                            *textdata++ = 0;
                        else {
                            temp = T1ReadWord(Vdp1Ram, ((dot & 0xF) * 2 + colorLut) & 0x7FFFF);
                            if (temp & 0x8000)
                                *textdata++ = SAT2YAB1(0xFF, temp);
                            else
                                *textdata++ = ColorRamGetColor(temp);
                        }
                    }

                    j += 1;

                    charAddr += 1;
                }
            }
            break;
        }
        case 2: {
            // 8 bpp(64 color) Bank mode
            u32 colorBank   = cmd.CMDCOLR;
            u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;

            u16 i, j;

            for (i = 0; i < h[0]; i++) {
                for (j = 0; j < w[0]; j++) {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x3F;
                    charAddr++;

                    if ((dot == 0) && !SPD)
                        *textdata++ = 0;
                    else
                        *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
                }
            }
            break;
        }
        case 3: {
            // 8 bpp(128 color) Bank mode
            u32 colorBank   = cmd.CMDCOLR;
            u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
            u16 i, j;

            for (i = 0; i < h[0]; i++) {
                for (j = 0; j < w[0]; j++) {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x7F;
                    charAddr++;

                    if ((dot == 0) && !SPD)
                        *textdata++ = 0;
                    else
                        *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
                }
            }
            break;
        }
        case 4: {
            // 8 bpp(256 color) Bank mode
            u32 colorBank   = cmd.CMDCOLR;
            u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
            u16 i, j;

            for (i = 0; i < h[0]; i++) {
                for (j = 0; j < w[0]; j++) {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
                    charAddr++;

                    if ((dot == 0) && !SPD)
                        *textdata++ = 0;
                    else
                        *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
                }
            }
            break;
        }
        case 5: {
            // 16 bpp Bank mode
            u16 i, j;

            for (i = 0; i < h[0]; i++) {
                for (j = 0; j < w[0]; j++) {
                    dot = T1ReadWord(Vdp1Ram, charAddr & 0x7FFFF);

                    if (isendcode && (ret = CheckEndcode(dot, 0x7FFF, &code)) > 0) {
                        if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 16))
                            break;
                    } else {
                        // if (!(dot & 0x8000) && (Vdp2Regs->SPCTL & 0x20)) printf("mixed mode\n");
                        if (!(dot & 0x8000) && !SPD)
                            *textdata++ = 0;
                        else
                            *textdata++ = SAT2YAB1(0xFF, dot);
                    }

                    charAddr += 2;
                }
            }
            break;
        }
        default:
            break;
    }

    return texture;
}

u8 *Vdp1DebugRawTexture(u32 cmdNumber, int *width, int *height, int *numBytes)
{
    u16            cmdRaw;
    vdp1cmd_struct cmd;
    u32            cmdAddress;
    u8            *texture = NULL;

    // Initial number of bytes written to texture
    *numBytes = 0;

    if ((cmdAddress = Vdp1DebugGetCommandNumberAddr(cmdNumber)) == 0xFFFFFFFF)
        return NULL;

    cmdRaw = T1ReadWord(Vdp1Ram, cmdAddress);

    if (cmdRaw & 0x8000)
        // Draw End
        return NULL;

    if (cmdRaw & 0x4000)
        // Command Skipped
        return NULL;

    Vdp1ReadCommand(&cmd, cmdAddress, Vdp1Ram);

    const int spriteCmdType = ((cmd.CMDPMOD >> 3) & 0x7);
    switch (cmd.CMDCTRL & 0x000F) {
        case 0:    // Normal Sprite
        case 1:    // Scaled Sprite
        case 2:    // Distorted Sprite
        case 3:    // Distorted Sprite *
            width[0]  = (cmd.CMDSIZE & 0x3F00) >> 5;
            height[0] = cmd.CMDSIZE & 0xFF;

            switch (spriteCmdType) {
                // 0: 4 bpp Bank mode
                // 1: 4 bpp LUT mode
                case 0:
                case 1:
                    numBytes[0] = 0.5 * width[0] * height[0];
                    texture     = (u8 *)malloc(numBytes[0]);
                    break;
                // 2: 8 bpp(64 color) Bank mode
                // 3: 8 bpp(128 color) Bank mode
                // 4: 8 bpp(256 color) Bank mode
                case 2:
                case 3:
                case 4:
                    numBytes[0] = width[0] * height[0];
                    texture     = (u8 *)malloc(numBytes[0]);
                    break;
                // 5: 16 bpp Bank mode
                case 5:
                    numBytes[0] = 2 * width[0] * height[0];
                    texture     = (u8 *)malloc(numBytes[0]);
                    break;
                default:
                    texture = NULL;
                    break;
            }

            if (texture == NULL)
                return NULL;

            break;
        case 4:    // Polygon
        case 5:    // Polyline
        case 6:    // Line
        case 7:    // Polyline *
            // Do 1x1 pixel
            width[0]  = 1;
            height[0] = 1;
            texture   = (u8 *)malloc(sizeof(u16));

            if (texture == NULL)
                return NULL;

            *numBytes = 2;
            memcpy(texture, &cmd.CMDCOLR, sizeof(u16));
            return texture;
        case 8:     // User Clipping
        case 9:     // System Clipping
        case 10:    // Local Coordinates
        case 11:    // User Clipping *
            return NULL;
        default:    // Invalid command
            return NULL;
    }

    // Read texture data directly from VRAM.
    for (u32 i = 0; i < *numBytes; ++i) {
        texture[i] = T1ReadByte(Vdp1Ram, ((cmd.CMDSRCA * 8) + i) & 0x7FFFF);
    }

    return texture;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleVDP1(void)
{
    Vdp1External.disptoggle ^= 1;
}
//////////////////////////////////////////////////////////////////////////////

static void Vdp1EraseWrite(int id)
{
    lastHash = -1;
    if ((VIDCore != NULL) && (VIDCore->Vdp1EraseWrite != NULL))
        VIDCore->Vdp1EraseWrite(id);
}
static void startField(void)
{
    int isrender           = 0;
    yabsys.wait_line_count = -1;
    FRAMELOG("StartField ***** VOUT(T) %d FCM=%d FCT=%d VBE=%d PTMR=%d (%d, %d, %d, %d)*****\n",
             Vdp1External.swap_frame_buffer, (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01),
             (Vdp1Regs->TVMR >> 3) & 0x01, Vdp1Regs->PTMR, Vdp1External.onecyclemode, Vdp1External.manualchange,
             Vdp1External.manualerase, needVBlankErase());

    // Manual Change
    Vdp1External.swap_frame_buffer |= (Vdp1External.manualchange == 1);
    Vdp1External.swap_frame_buffer |= (Vdp1External.onecyclemode == 1);

    // Frame Change
    if (Vdp1External.swap_frame_buffer == 1) {
        addVdp1Framecount();
        FRAMELOG("Swap Line %d\n", yabsys.LineCount);
        lastHash = -1;
        if ((Vdp1External.manualerase == 1) || (Vdp1External.onecyclemode == 1)) {
            int id = 0;
            if (_Ygl != NULL)
                id = _Ygl->readframe;
            Vdp1EraseWrite(id);
            Vdp1External.manualerase = 0;
        }

        VIDCore->Vdp1FrameChange();
        FRAMELOG("Change readframe %d to %d (%d)\n", _Ygl->drawframe, _Ygl->readframe, yabsys.LineCount);
        Vdp1External.current_frame = !Vdp1External.current_frame;
        Vdp1Regs->LOPR             = Vdp1Regs->COPR;
        Vdp1Regs->COPR             = 0;
        Vdp1Regs->lCOPR            = 0;
        Vdp1Regs->EDSR >>= 1;

        FRAMELOG("[VDP1] Displayed framebuffer changed. EDSR=%02X", Vdp1Regs->EDSR);

        Vdp1External.swap_frame_buffer = 0;

        // if Plot Trigger mode == 0x02 draw start
        if ((Vdp1Regs->PTMR == 0x2)) {
            FRAMELOG("[VDP1] PTMR == 0x2 start drawing immidiatly\n");
            abortVdp1();
            vdp1_clock = 0;
            RequestVdp1ToDraw();
        }
    } else {
        if (Vdp1External.status == VDP1_STATUS_RUNNING) {
            LOG("[VDP1] Start Drawing continue");
            RequestVdp1ToDraw();
        }
    }

    if (Vdp1Regs->PTMR == 0x1)
        Vdp1External.plot_trigger_done = 0;

    FRAMELOG("End StartField\n");

    Vdp1External.manualchange = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankIN(void)
{
    int needToCompose = 0;
    if (nbCmdToProcess > 0) {
        for (int i = 0; i < nbCmdToProcess; i++) {
            if (cmdBufferBeingProcessed[i].ignitionLine == (yabsys.LineCount + 1)) {
                if (!((cmdBufferBeingProcessed[i].start_addr >= vdp1Ram_update_end) ||
                      (cmdBufferBeingProcessed[i].end_addr <= vdp1Ram_update_start))) {
                    needToCompose = 1;
                    if (Vdp1External.checkEDSR == 0) {
                        if (VIDCore->Vdp1RegenerateCmd != NULL) {
                            VIDCore->Vdp1RegenerateCmd(&cmdBufferBeingProcessed[i].cmd);
                        }
                    }
                }
                cmdBufferBeingProcessed[i].ignitionLine = -1;
            }
        }
        nbCmdToProcess = 0;
        if (needToCompose == 1) {
            // We need to evaluate end line and not ignition line? It is improving doom if we better take care of the
            // concurrency betwwen vdp1 update and command list"
            vdp1Ram_update_start = 0x80000;
            vdp1Ram_update_end   = 0x0;
            if (VIDCore != NULL) {
                if (VIDCore->composeVDP1 != NULL)
                    VIDCore->composeVDP1();
            }
            Vdp1Regs->COPR = Vdp1Regs->lCOPR;
        }
    }
    if (yabsys.LineCount == 0) {
        startField();
    }
    if (Vdp1Regs->PTMR == 0x1) {
        if (Vdp1External.plot_trigger_line == yabsys.LineCount) {
            if (Vdp1External.plot_trigger_done == 0) {
                vdp1_clock = 0;
                RequestVdp1ToDraw();
                Vdp1External.plot_trigger_done = 1;
            }
        }
    }
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
    if (VIDCore != NULL && VIDCore->id != VIDCORE_SOFT)
        YglTMCheck();
#endif
}
//////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankOUT(void)
{
    vdp1_clock += getVdp1CyclesPerLine();
    Vdp1TryDraw();
}

//////////////////////////////////////////////////////////////////////////////
extern void vdp1_compute();
void        Vdp1VBlankIN(void)
{
    // if (VIDCore != NULL) {
    //   if (VIDCore->composeVDP1 != NULL) VIDCore->composeVDP1();
    // }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1VBlankOUT(void)
{
    // Out of VBlankOut : Break Batman
    if (needVBlankErase()) {
        int id = 0;
        if (_Ygl != NULL)
            id = _Ygl->readframe;
        Vdp1EraseWrite(id);
    }
}
