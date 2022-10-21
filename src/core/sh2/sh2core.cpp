#include <stdlib.h>
#include "sh2core.h"
#include "debug.h"
#include "memory.h"
#include "yabause.h"

SH2_struct                 *SSH2    = NULL;
SH2_struct                 *MSH2    = NULL;
SH2Interface_struct        *SH2Core = NULL;
extern SH2Interface_struct *SH2CoreList[];

void        OnchipReset(SH2_struct *context);
static void FRTExec(SH2_struct *context, u32 cycles);
static void WDTExec(SH2_struct *context, u32 cycles);
u8          SCIReceiveByte(void);
void        SCITransmitByte(u8);

void enableCache(SH2_struct *ctx);
void disableCache(SH2_struct *ctx);
void InvalidateCache(SH2_struct *ctx);

#define CACHE_LOG

int SH2Init(int coreid)
{
    int i;

    if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
        return -1;

    MSH2->onchip.BCR1 = 0x0000;
    MSH2->isslave     = 0;
    MSH2->trace       = 0;

    if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
        return -1;

    SSH2->trace       = 0;
    SSH2->onchip.BCR1 = 0x8000;
    SSH2->isslave     = 1;

    MSH2->cacheOn = 0;
    SSH2->cacheOn = 0;

    memset(MSH2->tagWay, 0x4, 64 * 0x80000);
    memset(MSH2->cacheTagArray, 0x0, 64 * 4 * sizeof(u32));
    memset(SSH2->tagWay, 0x4, 64 * 0x80000);
    memset(SSH2->cacheTagArray, 0x0, 64 * 4 * sizeof(u32));
    if (coreid == SH2CORE_DEFAULT)
        coreid = 0;

    for (i = 0; SH2CoreList[i] != NULL; i++) {
        if (SH2CoreList[i]) {
            SH2Core = SH2CoreList[i];
            break;
        }
    }

    if ((SH2Core == NULL) || (SH2Core->Init() != 0)) {
        free(MSH2);
        free(SSH2);
        MSH2 = SSH2 = NULL;
        return -1;
    }

    SH2Reset(MSH2);
    SH2Reset(SSH2);

    return 0;
}

void SH2DeInit()
{
    if (SH2Core)
        SH2Core->DeInit();
    SH2Core = NULL;

    if (MSH2) {
        free(MSH2);
    }
    MSH2 = NULL;

    if (SSH2) {
        free(SSH2);
    }
    SSH2 = NULL;
}

void SH2Reset(SH2_struct *context)
{
    int i;
    SH2Core->Reset(context);

    for (i = 0; i < 15; i++)
        SH2Core->SetGPR(context, i, 0x00000000);

    SH2Core->SetSR(context, 0x000000F0);
    SH2Core->SetGBR(context, 0x00000000);
    SH2Core->SetVBR(context, 0x00000000);
    SH2Core->SetMACH(context, 0x00000000);
    SH2Core->SetMACL(context, 0x00000000);
    SH2Core->SetPR(context, 0x00000000);

    context->delay  = 0x00000000;
    context->cycles = 0;

    context->frc.leftover = 0;
    context->frc.shift    = 3;

    context->wdt.isenable   = 0;
    context->wdt.isinterval = 1;
    context->wdt.shift      = 1;
    context->wdt.leftover   = 0;

    context->cycleFrac = 0;

    memset((void *)context->interrupts, 0, sizeof(interrupt_struct) * MAX_INTERRUPTS);
    SH2Core->SetInterrupts(context, 0, context->interrupts);

    OnchipReset(context);
    InvalidateCache(context);

#ifdef DMPHISTORY
    memset(context->pchistory, 0, sizeof(context->pchistory));
    context->pchistory_index = 0;
#endif
}

void SH2PowerOn(SH2_struct *context)
{
    u32 VBR = SH2Core->GetVBR(context);
    SH2Core->SetPC(context, SH2MappedMemoryReadLong(context, VBR));
    SH2Core->SetGPR(context, 15, SH2MappedMemoryReadLong(context, VBR + 4));
    CACHE_LOG("%s start\n", (context == SSH2) ? "SSH2" : "MSH2");
}

void FASTCALL SH2TestExec(SH2_struct *context, u32 cycles)
{
    SH2Core->TestExec(context, cycles);
}

void FASTCALL SH2Exec(SH2_struct *context, u32 cycles)
{
    SH2Core->Exec(context, cycles);
    FRTExec(context, cycles);
    WDTExec(context, cycles);
}

void FASTCALL SH2OnFrame(SH2_struct *context)
{
    SH2Core->OnFrame(context);
}

void SH2SendInterrupt(SH2_struct *context, u8 vector, u8 level)
{
    SH2Core->SendInterrupt(context, vector, level);
}

void SH2RemoveInterrupt(SH2_struct *context, u8 vector, u8 level)
{
    SH2Core->RemoveInterrupt(context, vector, level);
}

void SH2NMI(SH2_struct *context)
{
    context->onchip.ICR |= 0x8000;
    SH2SendInterrupt(context, 0xB, 0x10);
}

void SH2GetRegisters(SH2_struct *context, sh2regs_struct *r)
{
    if (r != NULL) {
        SH2Core->GetRegisters(context, r);
    }
}

void SH2SetRegisters(SH2_struct *context, sh2regs_struct *r)
{
    if (r != NULL) {
        SH2Core->SetRegisters(context, r);
    }
}

void SH2WriteNotify(SH2_struct *context, u32 start, u32 length)
{
    if (SH2Core->WriteNotify)
        SH2Core->WriteNotify(start, length);
}

void OnchipReset(SH2_struct *context)
{
    context->onchip.SMR       = 0x00;
    context->onchip.BRR       = 0xFF;
    context->onchip.SCR       = 0x00;
    context->onchip.TDR       = 0xFF;
    context->onchip.SSR       = 0x84;
    context->onchip.RDR       = 0x00;
    context->onchip.TIER      = 0x01;
    context->onchip.FTCSR     = 0x00;
    context->onchip.FRC.all   = 0x0000;
    context->onchip.OCRA      = 0xFFFF;
    context->onchip.OCRB      = 0xFFFF;
    context->onchip.TCR       = 0x00;
    context->onchip.TOCR      = 0xE0;
    context->onchip.FICR      = 0x0000;
    context->onchip.IPRB      = 0x0000;
    context->onchip.VCRA      = 0x0000;
    context->onchip.VCRB      = 0x0000;
    context->onchip.VCRC      = 0x0000;
    context->onchip.VCRD      = 0x0000;
    context->onchip.DRCR0     = 0x00;
    context->onchip.DRCR1     = 0x00;
    context->onchip.WTCSR     = 0x18;
    context->onchip.WTCNT     = 0x00;
    context->onchip.RSTCSR    = 0x1F;
    context->onchip.SBYCR     = 0x60;
    context->onchip.CCR       = 0x00;
    context->onchip.ICR       = 0x0000;
    context->onchip.IPRA      = 0x0000;
    context->onchip.VCRWDT    = 0x0000;
    context->onchip.DVCR      = 0x00000000;
    context->onchip.VCRDIV    = 0x00000000;
    context->onchip.BARA.all  = 0x00000000;
    context->onchip.BAMRA.all = 0x00000000;
    context->onchip.BBRA      = 0x0000;
    context->onchip.BARB.all  = 0x00000000;
    context->onchip.BAMRB.all = 0x00000000;
    context->onchip.BDRB.all  = 0x00000000;
    context->onchip.BDMRB.all = 0x00000000;
    context->onchip.BBRB      = 0x0000;
    context->onchip.BRCR      = 0x0000;
    context->onchip.CHCR0     = 0x00000000;
    context->onchip.CHCR1     = 0x00000000;
    context->onchip.DMAOR     = 0x00000000;
    context->onchip.BCR1 &= 0x8000;
    context->onchip.BCR1 |= 0x03F0;
    context->onchip.BCR2  = 0x00FC;
    context->onchip.WCR   = 0xAAFF;
    context->onchip.MCR   = 0x0000;
    context->onchip.RTCSR = 0x0000;
    context->onchip.RTCNT = 0x0000;
    context->onchip.RTCOR = 0x0000;
}

u8 FASTCALL OnchipReadByte(SH2_struct *context, u32 addr)
{
    switch (addr) {
        case 0x000:
            return context->onchip.SMR;
        case 0x001:
            return context->onchip.BRR;
        case 0x002:
            return context->onchip.SCR;
        case 0x003:
            return context->onchip.TDR;
        case 0x004:

            return context->onchip.SSR;
        case 0x005:
            return context->onchip.RDR;
        case 0x010:
            return context->onchip.TIER;
        case 0x011:
            return context->onchip.FTCSR;
        case 0x012:
            return context->onchip.FRC.part.H;
        case 0x013:
            return context->onchip.FRC.part.L;
        case 0x014:
            if (!(context->onchip.TOCR & 0x10))
                return context->onchip.OCRA >> 8;
            else
                return context->onchip.OCRB >> 8;
        case 0x015:
            if (!(context->onchip.TOCR & 0x10))
                return context->onchip.OCRA & 0xFF;
            else
                return context->onchip.OCRB & 0xFF;
        case 0x016:
            return context->onchip.TCR;
        case 0x017:
            return context->onchip.TOCR;
        case 0x018:
            return context->onchip.FICR >> 8;
        case 0x019:
            return context->onchip.FICR & 0xFF;
        case 0x060:
            return context->onchip.IPRB >> 8;
        case 0x062:
            return context->onchip.VCRA >> 8;
        case 0x063:
            return context->onchip.VCRA & 0xFF;
        case 0x064:
            return context->onchip.VCRB >> 8;
        case 0x065:
            return context->onchip.VCRB & 0xFF;
        case 0x066:
            return context->onchip.VCRC >> 8;
        case 0x067:
            return context->onchip.VCRC & 0xFF;
        case 0x068:
            return context->onchip.VCRD >> 8;
        case 0x080:
            return context->onchip.WTCSR;
        case 0x081:
            return context->onchip.WTCNT;
        case 0x092:
            return context->onchip.CCR;
        case 0x0E0:
            return context->onchip.ICR >> 8;
        case 0x0E1:
            return context->onchip.ICR & 0xFF;
        case 0x0E2:
            return context->onchip.IPRA >> 8;
        case 0x0E3:
            return context->onchip.IPRA & 0xFF;
        case 0x0E4:
            return context->onchip.VCRWDT >> 8;
        case 0x0E5:
            return context->onchip.VCRWDT & 0xFF;
        default:
            LOG("Unhandled Onchip byte read %08X\n", (int)addr);
            break;
    }

    return 0;
}

u16 FASTCALL OnchipReadWord(SH2_struct *context, u32 addr)
{
    switch (addr) {
        case 0x060:
            return context->onchip.IPRB;
        case 0x062:
            return context->onchip.VCRA;
        case 0x064:
            return context->onchip.VCRB;
        case 0x066:
            return context->onchip.VCRC;
        case 0x068:
            return context->onchip.VCRD;
        case 0x0E0:
            return context->onchip.ICR;
        case 0x0E2:
            return context->onchip.IPRA;
        case 0x0E4:
            return context->onchip.VCRWDT;
        case 0x1E2:
            return context->onchip.BCR1;
        case 0x1E6:
            return context->onchip.BCR2;
        case 0x1EA:
            return context->onchip.WCR;
        case 0x1EE:
            return context->onchip.MCR;
        case 0x1F2:
            return context->onchip.RTCSR;
        case 0x1F6:
            return context->onchip.RTCNT;
        case 0x1FA:
            return context->onchip.RTCOR;
        default:
            LOG("Unhandled Onchip word read %08X\n", (int)addr);
            return 0;
    }

    return 0;
}

u32 FASTCALL OnchipReadLong(SH2_struct *context, u32 addr)
{
    switch (addr) {
        case 0x100:
        case 0x120:
            return context->onchip.DVSR;
        case 0x104:
        case 0x124:
            return context->onchip.DVDNTL;
        case 0x108:
        case 0x128:
            return context->onchip.DVCR;
        case 0x10C:
        case 0x12C:
            return context->onchip.VCRDIV;
        case 0x110:
        case 0x130:
            return context->onchip.DVDNTH;
        case 0x114:
        case 0x134:
            return context->onchip.DVDNTL;
        case 0x118:
        case 0x138:
            return context->onchip.DVDNTUH;
        case 0x11C:
        case 0x13C:
            return context->onchip.DVDNTUL;
        case 0x180:
            return context->onchip.SAR0;
        case 0x184:
            return context->onchip.DAR0;
        case 0x188:
            return context->onchip.TCR0;
        case 0x18C:
            return context->onchip.CHCR0;
        case 0x190:
            return context->onchip.SAR1;
        case 0x194:
            return context->onchip.DAR1;
        case 0x198:
            return context->onchip.TCR1;
        case 0x19C:
            context->onchip.CHCR1M = 0;
            return context->onchip.CHCR1;
        case 0x1A0:
            return context->onchip.VCRDMA0;
        case 0x1A8:
            return context->onchip.VCRDMA1;
        case 0x1B0:
            return context->onchip.DMAOR;
        case 0x1E0:
            return context->onchip.BCR1;
        case 0x1E4:
            return context->onchip.BCR2;
        case 0x1E8:
            return context->onchip.WCR;
        case 0x1EC:
            return context->onchip.MCR;
        case 0x1F0:
            return context->onchip.RTCSR;
        case 0x1F4:
            return context->onchip.RTCNT;
        case 0x1F8:
            return context->onchip.RTCOR;
        default:
            LOG("Unhandled Onchip long read %08X\n", (int)addr);
            return 0;
    }

    return 0;
}

void FASTCALL OnchipWriteByte(SH2_struct *context, u32 addr, u8 val)
{
    switch (addr) {
        case 0x000:
            context->onchip.SMR = val;
            return;
        case 0x001:
            context->onchip.BRR = val;
            return;
        case 0x002:

            if (!(val & 0x20))
                context->onchip.SSR |= 0x80;

            context->onchip.SCR = val;
            return;
        case 0x003:
            context->onchip.TDR = val;
            return;
        case 0x004:

            if (context->onchip.SCR & 0x20) {

                if (!(val & 0x80))
                    SCITransmitByte(context->onchip.TDR);
            }
            return;
        case 0x010:

            context->onchip.TIER = (val & 0x8E) | 0x1;
            if ((val & 0x80) && (context->onchip.FTCSR & 0x80)) {
                SH2SendInterrupt(context, (context->onchip.VCRC >> 8) & 0x7F, (context->onchip.IPRB >> 8) & 0xF);
            }
            return;
        case 0x011:
            context->onchip.FTCSR = (context->onchip.FTCSR & (val & 0xFE)) | (val & 0x1);
            return;
        case 0x012:
            context->onchip.FRC.part.H = val;
            return;
        case 0x013:
            context->onchip.FRC.part.L = val;
            return;
        case 0x014:
            if (!(context->onchip.TOCR & 0x10))
                context->onchip.OCRA = (val << 8) | (context->onchip.OCRA & 0xFF);
            else
                context->onchip.OCRB = (val << 8) | (context->onchip.OCRB & 0xFF);
            return;
        case 0x015:
            if (!(context->onchip.TOCR & 0x10))
                context->onchip.OCRA = (context->onchip.OCRA & 0xFF00) | val;
            else
                context->onchip.OCRB = (context->onchip.OCRB & 0xFF00) | val;
            return;
        case 0x016:
            context->onchip.TCR = val & 0x83;
            switch (val & 3) {
                case 0:
                    context->frc.shift = 3;
                    break;
                case 1:
                    context->frc.shift = 5;
                    break;
                case 2:
                    context->frc.shift = 7;
                    break;
                case 3:
                    LOG("FRT external input clock not implemented.\n");
                    break;
            }
            return;
        case 0x017:
            context->onchip.TOCR = 0xE0 | (val & 0x13);
            return;
        case 0x060:
            context->onchip.IPRB = (val << 8);
            return;
        case 0x061:
            return;
        case 0x062:
            context->onchip.VCRA = ((val & 0x7F) << 8) | (context->onchip.VCRA & 0x00FF);
            return;
        case 0x063:
            context->onchip.VCRA = (context->onchip.VCRA & 0xFF00) | (val & 0x7F);
            return;
        case 0x064:
            context->onchip.VCRB = ((val & 0x7F) << 8) | (context->onchip.VCRB & 0x00FF);
            return;
        case 0x065:
            context->onchip.VCRB = (context->onchip.VCRB & 0xFF00) | (val & 0x7F);
            return;
        case 0x066:
            context->onchip.VCRC = ((val & 0x7F) << 8) | (context->onchip.VCRC & 0x00FF);
            return;
        case 0x067:
            context->onchip.VCRC = (context->onchip.VCRC & 0xFF00) | (val & 0x7F);
            return;
        case 0x068:
            context->onchip.VCRD = (val & 0x7F) << 8;
            return;
        case 0x069:
            return;
        case 0x071:
            context->onchip.DRCR0 = val & 0x3;
            return;
        case 0x072:
            context->onchip.DRCR1 = val & 0x3;
            return;
        case 0x091:
            context->onchip.SBYCR = val & 0xDF;
            return;
        case 0x092:
            context->onchip.CCR = val & 0xCF;
            if (val & 0x10) {
                InvalidateCache(context);
            }
            if ((context->onchip.CCR & 0x01)) {
                enableCache(context);
            } else {
                disableCache(context);
            }
            return;
        case 0x0E0:
            context->onchip.ICR = ((val & 0x1) << 8) | (context->onchip.ICR & 0xFEFF);
            return;
        case 0x0E1:
            context->onchip.ICR = (context->onchip.ICR & 0xFFFE) | (val & 0x1);
            return;
        case 0x0E2:
            context->onchip.IPRA = (val << 8) | (context->onchip.IPRA & 0x00FF);
            return;
        case 0x0E3:
            context->onchip.IPRA = (context->onchip.IPRA & 0xFF00) | (val & 0xF0);
            return;
        case 0x0E4:
            context->onchip.VCRWDT = ((val & 0x7F) << 8) | (context->onchip.VCRWDT & 0x00FF);
            return;
        case 0x0E5:
            context->onchip.VCRWDT = (context->onchip.VCRWDT & 0xFF00) | (val & 0x7F);
            return;
        default:
            LOG("Unhandled Onchip byte write %08X\n", (int)addr);
    }
}

void FASTCALL OnchipWriteWord(SH2_struct *context, u32 addr, u16 val)
{
    switch (addr) {
        case 0x060:
            context->onchip.IPRB = val & 0xFF00;
            return;
        case 0x062:
            context->onchip.VCRA = val & 0x7F7F;
            return;
        case 0x064:
            context->onchip.VCRB = val & 0x7F7F;
            return;
        case 0x066:
            context->onchip.VCRC = val & 0x7F7F;
            return;
        case 0x068:
            context->onchip.VCRD = val & 0x7F7F;
            return;
        case 0x080:

            if (val >> 8 == 0xA5) {
                switch (val & 7) {
                    case 0:
                        context->wdt.shift = 1;
                        break;
                    case 1:
                        context->wdt.shift = 6;
                        break;
                    case 2:
                        context->wdt.shift = 7;
                        break;
                    case 3:
                        context->wdt.shift = 8;
                        break;
                    case 4:
                        context->wdt.shift = 9;
                        break;
                    case 5:
                        context->wdt.shift = 10;
                        break;
                    case 6:
                        context->wdt.shift = 12;
                        break;
                    case 7:
                        context->wdt.shift = 13;
                        break;
                }

                context->wdt.isenable   = (val & 0x20);
                context->wdt.isinterval = (~val & 0x40);

                context->onchip.WTCSR = (u8)val | 0x18;
            } else if (val >> 8 == 0x5A) {
                context->onchip.WTCNT = (u8)val;
            }
            return;
        case 0x082:
            if (val == 0xA500)
                context->onchip.RSTCSR &= 0x7F;
            else if (val >> 8 == 0x5A)
                context->onchip.RSTCSR = (context->onchip.RSTCSR & 0x80) | (val & 0x60) | 0x1F;
            return;
        case 0x092:
            context->onchip.CCR = val & 0xCF;
            if (val & 0x10) {
                InvalidateCache(context);
            }
            if ((context->onchip.CCR & 0x01)) {
                enableCache(context);
            } else {
                disableCache(context);
            }
            return;
        case 0x0E0:
            context->onchip.ICR = val & 0x0101;
            return;
        case 0x0E2:
            context->onchip.IPRA = val & 0xFFF0;
            return;
        case 0x0E4:
            context->onchip.VCRWDT = val & 0x7F7F;
            return;
        case 0x108:
        case 0x128:
            context->onchip.DVCR = val & 0x3;
            return;
        case 0x148:
            context->onchip.BBRA = val & 0xFF;
            return;
        case 0x178:
            context->onchip.BRCR = val & 0xF4DC;
            return;
        default:
            LOG("Unhandled Onchip word write %08X(%04X)\n", (int)addr, val);
            return;
    }
}

void FASTCALL OnchipWriteLong(SH2_struct *context, u32 addr, u32 val)
{
    switch (addr) {
        case 0x010:
            context->onchip.TIER = (val & 0x8E) | 0x1;
            break;
        case 0x060:
            context->onchip.IPRB = val & 0xFF00;
            break;
        case 0x100:
        case 0x120:
            context->onchip.DVSR = val;
            return;
        case 0x104:
        case 0x124: {
            s32 divisor = (s32)context->onchip.DVSR;
            if (divisor == 0) {
                if (val & 0x80000000) {
                    context->onchip.DVDNTL = 0x80000000;
                    context->onchip.DVDNTH = 0xFFFFFFFC | ((val >> 29) & 0x3);
                } else {
                    context->onchip.DVDNTL = 0x7FFFFFFF;
                    context->onchip.DVDNTH = 0 | (val >> 29);
                }
                context->onchip.DVDNTUL = context->onchip.DVDNTL;
                context->onchip.DVDNTUH = context->onchip.DVDNTH;
                context->onchip.DVCR |= 1;

                if (context->onchip.DVCR & 0x2)
                    SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
            } else {
                s32 quotient  = ((s32)val) / divisor;
                s32 remainder = ((s32)val) % divisor;

                if (quotient > 0x7FFFFFFF) {
                    context->onchip.DVCR |= 1;
                    context->onchip.DVDNTL = 0x7FFFFFFF;
                    context->onchip.DVDNTH = 0xFFFFFFFE;

                    if (context->onchip.DVCR & 0x2) {
                        SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
                    }
                } else if ((s32)((s64)quotient >> 32) < -1) {
                    context->onchip.DVCR |= 1;
                    context->onchip.DVDNTL = 0x80000000;
                    context->onchip.DVDNTH = 0xFFFFFFFE;

                    if (context->onchip.DVCR & 0x2) {
                        SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
                    }
                } else {
                    context->onchip.DVDNTL = quotient;
                    context->onchip.DVDNTH = remainder;
                }
                context->onchip.DVDNT   = context->onchip.DVDNTL;
                context->onchip.DVDNTUL = context->onchip.DVDNTL;
                context->onchip.DVDNTUH = context->onchip.DVDNTH;
            }
            return;
        }
        case 0x108:
        case 0x128:
            context->onchip.DVCR = val & 0x3;
            return;
        case 0x10C:
        case 0x12C:
            context->onchip.VCRDIV = val & 0xFFFF;
            return;
        case 0x110:
        case 0x130:
            context->onchip.DVDNTH = val;
            return;
        case 0x114:
        case 0x134: {
            s32 divisor  = (s32)context->onchip.DVSR;
            s64 dividend = context->onchip.DVDNTH;
            dividend     = (s64)(((u64)dividend) << 32);
            dividend |= val;

            if (divisor == 0) {
                if (context->onchip.DVDNTH & 0x80000000) {
                    context->onchip.DVDNTL = 0x80000000;
                    context->onchip.DVDNTH = context->onchip.DVDNTH << 3;
                } else {
                    context->onchip.DVDNTL = 0x7FFFFFFF;
                    context->onchip.DVDNTH = context->onchip.DVDNTH << 3;
                }

                context->onchip.DVDNTUL = context->onchip.DVDNTL;
                context->onchip.DVDNTUH = context->onchip.DVDNTH;
                context->onchip.DVCR |= 1;

                if (context->onchip.DVCR & 0x2)
                    SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
            } else {
                s64 quotient  = dividend / divisor;
                s32 remainder = dividend % divisor;

                if (quotient > 0x7FFFFFFF) {
                    context->onchip.DVCR |= 1;
                    context->onchip.DVDNTL = 0x7FFFFFFF;
                    context->onchip.DVDNTH = 0xFFFFFFFE;

                    if (context->onchip.DVCR & 0x2)
                        SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
                } else if ((s32)(quotient >> 32) < -1) {
                    context->onchip.DVCR |= 1;
                    context->onchip.DVDNTL = 0x80000000;
                    context->onchip.DVDNTH = 0xFFFFFFFE;

                    if (context->onchip.DVCR & 0x2)
                        SH2SendInterrupt(context, context->onchip.VCRDIV & 0x7F, (context->onchip.IPRA >> 12) & 0xF);
                } else {
                    context->onchip.DVDNTL = quotient;
                    context->onchip.DVDNTH = remainder;
                }
                context->onchip.DVDNT   = context->onchip.DVDNTL;
                context->onchip.DVDNTUL = context->onchip.DVDNTL;
                context->onchip.DVDNTUH = context->onchip.DVDNTH;
            }
            return;
        }
        case 0x118:
        case 0x138:
            context->onchip.DVDNTUH = val;
            return;
        case 0x11C:
        case 0x13C:
            context->onchip.DVDNTUL = val;
            return;
        case 0x140:
            context->onchip.BARA.all = val;
            return;
        case 0x144:
            context->onchip.BAMRA.all = val;
            return;
        case 0x180:
            context->onchip.SAR0 = val;
            return;
        case 0x184:
            context->onchip.DAR0 = val;
            return;
        case 0x188:
            context->onchip.TCR0 = val & 0xFFFFFF;
            return;
        case 0x18C:
            context->onchip.CHCR0 = val & 0xFFFF;

            if ((context->onchip.DMAOR & 7) == 1 && (val & 0x3) == 1)
                DMAExec(context);
            return;
        case 0x190:
            context->onchip.SAR1 = val;
            return;
        case 0x194:
            context->onchip.DAR1 = val;
            return;
        case 0x198:
            context->onchip.TCR1 = val & 0xFFFFFF;
            return;
        case 0x19C:
            context->onchip.CHCR1 = val & 0xFFFF;

            context->onchip.CHCR1 = (val & ~2) | (context->onchip.CHCR1 & (val | context->onchip.CHCR1M) & 2);

            if ((context->onchip.DMAOR & 7) == 1 && (context->onchip.CHCR1 & 0x3) == 1)
                DMAExec(context);
            return;
        case 0x1A0:
            context->onchip.VCRDMA0 = val & 0xFFFF;
            return;
        case 0x1A8:
            context->onchip.VCRDMA1 = val & 0xFFFF;
            return;
        case 0x1B0:
            context->onchip.DMAOR = val & 0xF;

            if ((val & 7) == 1)
                DMAExec(context);
            return;
        case 0x1E0:
            context->onchip.BCR1 &= 0x8000;
            context->onchip.BCR1 |= val & 0x1FF7;
            return;
        case 0x1E4:
            context->onchip.BCR2 = val & 0xFC;
            return;
        case 0x1E8:
            context->onchip.WCR = val;
            return;
        case 0x1EC:
            context->onchip.MCR = val & 0xFEFC;
            return;
        case 0x1F0:
            context->onchip.RTCSR = val & 0xF8;
            return;
        case 0x1F8:
            context->onchip.RTCOR = val & 0xFF;
            return;
        default:
            LOG("Unhandled Onchip long write %08X,%08X\n", (int)addr, val);
            break;
    }
}
static void UpdateLRU(SH2_struct *context, u8 line, u8 way)
{
    switch (way) {
        case 0:
            context->cacheLRU[line] &= 0x7;
            break;
        case 1:
            context->cacheLRU[line] &= 0x19;
            context->cacheLRU[line] |= 0x20;
            break;
        case 2:
            context->cacheLRU[line] &= 0x2A;
            context->cacheLRU[line] |= 0x14;
            break;
        case 3:
            context->cacheLRU[line] |= 0x0B;
            break;
        default:
            break;
    }
}

static u8 getLRU(SH2_struct *context, u32 tag, u8 line)
{
    u8 way = -1;
    if (context->onchip.CCR & (1 << 3)) {
        if ((context->cacheLRU[line] & 1) == 1)
            return 2;
        else
            return 3;
    } else {
        if ((context->cacheLRU[line] & 0x38) == 0x38)
            way = 0;
        else if ((context->cacheLRU[line] & 0x26) == 0x6)
            way = 1;
        else if ((context->cacheLRU[line] & 0x15) == 0x1)
            way = 2;
        else if ((context->cacheLRU[line] & 0x0B) == 0x0)
            way = 3;
        else if (context->cacheLRU[line] == 0xB)
            way = 2;
        else if (context->cacheLRU[line] == 0x1E)
            way = 1;
        else if (context->cacheLRU[line] == 0x38)
            way = 0;
    }
    return way;
}

static inline void CacheWriteThrough(SH2_struct *context, u8 *mem, u32 addr, u32 val, u8 size)
{
    switch (size) {
        case 1:
            WriteByteList[(addr >> 16) & 0xFFF](context, mem, addr, val);
            break;
        case 2:
            WriteWordList[(addr >> 16) & 0xFFF](context, mem, addr, val);
            break;
        case 4:
            WriteLongList[(addr >> 16) & 0xFFF](context, mem, addr, val);
            break;
    }
}

static inline void CacheWriteVal(SH2_struct *context, u32 addr, u32 val, u8 size)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    u8  byte = addr & 0xF;
    u8  way  = context->tagWay[line][tag];
    switch (size) {
        case 1:
            WriteByteList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte, val);
            break;
        case 2:
            WriteWordList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte, val);
            break;
        case 4:
            WriteLongList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte, val);
            break;
    }
}

void CacheWrite(SH2_struct *context, u8 *mem, u32 addr, u32 val, u8 size)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    u8  byte = addr & 0xF;
    u8  way  = context->tagWay[line][tag];
    u8  ret  = 0;
    if (byte + size > 16)
        CACHE_LOG("!!!!!!!!!!!!!!!! Warn out of line....\n");
    if ((way <= 0x3) && (context->cacheTagArray[line][way] == tag)) {
        UpdateLRU(context, line, way);
        CacheWriteVal(context, addr, val, size);
    }
    CacheWriteThrough(context, mem, addr, val, size);
}

void CacheWriteByte(SH2_struct *context, u8 *mem, u32 addr, u8 val)
{
    CacheWrite(context, mem, addr, val, 1);
}
void CacheWriteWord(SH2_struct *context, u8 *mem, u32 addr, u16 val)
{
    CacheWrite(context, mem, addr, val, 2);
}
void CacheWriteLong(SH2_struct *context, u8 *mem, u32 addr, u32 val)
{
    CacheWrite(context, mem, addr, val, 4);
}

void InvalidateCache(SH2_struct *ctx)
{
    if (yabsys.usecache == 0)
        return;
    memset(ctx->cacheLRU, 0, 64);
    memset(ctx->tagWay, 0x4, 64 * 0x80000);
    memset(ctx->cacheTagArray, 0x0, 64 * 4 * sizeof(u32));
}

void enableCache(SH2_struct *context)
{
    int i;
    if (yabsys.usecache == 0)
        return;
    if (context->cacheOn == 0) {
        context->cacheOn    = 1;
        context->nbCacheWay = 4;
        for (i = 0x10; i < 0x1000; i++) {
            CacheReadByteList[i]  = CacheReadByte;
            CacheReadWordList[i]  = CacheReadWord;
            CacheReadLongList[i]  = CacheReadLong;
            CacheWriteByteList[i] = CacheWriteByte;
            CacheWriteWordList[i] = CacheWriteWord;
            CacheWriteLongList[i] = CacheWriteLong;
        }
    }
}

void disableCache(SH2_struct *context)
{
    int i;
    if (context->cacheOn == 1) {
        context->cacheOn = 0;
        for (i = 0x10; i < 0x1000; i++) {
            CacheReadByteList[i]  = ReadByteList[i];
            CacheReadWordList[i]  = ReadWordList[i];
            CacheReadLongList[i]  = ReadLongList[i];
            CacheWriteByteList[i] = WriteByteList[i];
            CacheWriteWordList[i] = WriteWordList[i];
            CacheWriteLongList[i] = WriteLongList[i];
        }
        InvalidateCache(context);
    }
}
void CacheFetch(SH2_struct *context, u8 *memory, u32 addr, u8 way)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    UpdateLRU(context, line, way);
    context->tagWay[line][tag]        = way;
    context->cacheTagArray[line][way] = tag;
    for (int i = 0; i < 4; i++) {
        u32 ret = ReadLongList[(addr >> 16) & 0xFFF](context, memory, (addr & (~0xF)) | (i * 4));
        CacheWriteVal(context, (addr & (~0xF)) | (i * 4), ret, 4);
    }
}

u8 CacheReadByte(SH2_struct *context, u8 *memory, u32 addr)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    u8  byte = addr & 0xF;
    u8  way  = context->tagWay[line][tag];
    if (byte + 1 > 16)
        CACHE_LOG("!!!!!!!!!!!!!!!! Warn out of line....\n");
    if ((way <= 0x3) && (context->cacheTagArray[line][way] == tag)) {
        UpdateLRU(context, line, way);
        u8 ret = ReadByteList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
        if (ret != ReadByteList[(addr >> 16) & 0xFFF](context, memory, addr)) {
            YuiMsg("Read Byte addr %x from cache = %x (%x)\n", addr, ret,
                   ReadByteList[(addr >> 16) & 0xFFF](context, memory, addr));
            fflush(stdout);
            abort();
        }
#endif
        return ret;
    }
    way = getLRU(context, tag, line);
    CacheFetch(context, memory, addr, way);
    u8 ret = ReadByteList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
    if (ret != ReadByteList[(addr >> 16) & 0xFFF](context, memory, addr)) {
        YuiMsg("Read Byte addr %x out of cache = %x (%x)\n", addr, ret,
               ReadByteList[(addr >> 16) & 0xFFF](context, memory, addr));
        fflush(stdout);
        abort();
    }
#endif
    return ret;
}

u16 CacheReadWord(SH2_struct *context, u8 *memory, u32 addr)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    u8  byte = (addr & 0xF);
    u8  way  = context->tagWay[line][tag];
    if (byte + 2 > 16)
        CACHE_LOG("!!!!!!!!!!!!!!!! Warn out of line....\n");
    if ((way <= 0x3) && (context->cacheTagArray[line][way] == tag)) {
        UpdateLRU(context, line, way);
        u16 ret = ReadWordList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
        if (ret != ReadWordList[(addr >> 16) & 0xFFF](context, memory, addr)) {
            YuiMsg("Read Word addr %x (%x) from of cache = %x (%x)\n", addr, (addr >> 16) & 0xFFF, ret,
                   ReadWordList[(addr >> 16) & 0xFFF](context, memory, addr));
            fflush(stdout);
            abort();
        }
#endif
        return ret;
    }
    way = getLRU(context, tag, line);
    CacheFetch(context, memory, addr, way);
    u16 ret = ReadWordList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
    if (ret != ReadWordList[(addr >> 16) & 0xFFF](context, memory, addr)) {
        YuiMsg("Read Word addr %x (%x) out of cache = %x (%x)\n", addr, (addr >> 16) & 0xFFF, ret,
               ReadWordList[(addr >> 16) & 0xFFF](context, memory, addr));
        fflush(stdout);
        abort();
    }
#endif
    return ret;
}

u32 CacheReadLong(SH2_struct *context, u8 *memory, u32 addr)
{
    u8  line = (addr >> 4) & 0x3F;
    u32 tag  = (addr >> 10) & 0x7FFFF;
    u8  byte = (addr & 0xF);
    u8  way  = context->tagWay[line][tag];
    if (byte + 4 > 16)
        CACHE_LOG("!!!!!!!!!!!!!!!! Warn out of line....\n");
    if ((way <= 0x3) && (context->cacheTagArray[line][way] == tag)) {
        UpdateLRU(context, line, way);
        u32 ret = ReadLongList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
        if (ret != ReadLongList[(addr >> 16) & 0xFFF](context, memory, addr)) {
            YuiMsg("Read Long addr %x from cache = %x (%x)\n", addr, ret,
                   ReadLongList[(addr >> 16) & 0xFFF](context, memory, addr));
            fflush(stdout);
            abort();
        }
#endif
        return ret;
    }
    way = getLRU(context, tag, line);
    CacheFetch(context, memory, addr, way);
    u32 ret = ReadLongList[(addr >> 16) & 0xFFF](context, context->cacheData[line][way], byte);
#ifdef CACHE_DEBUG
    if (ret != ReadLongList[(addr >> 16) & 0xFFF](context, memory, addr)) {
        YuiMsg("Read Long addr %x out of cache = %x (%x)\n", addr, ret,
               ReadLongList[(addr >> 16) & 0xFFF](context, memory, addr));
        fflush(stdout);
        abort();
    }
#endif
    return ret;
}
void CacheInvalidate(SH2_struct *context, u32 addr)
{
    if (yabsys.usecache == 0)
        return;
    u8  line                   = (addr >> 4) & 0x3F;
    u32 tag                    = (addr >> 10) & 0x7FFFF;
    u8  way                    = context->tagWay[line][tag];
    context->tagWay[line][tag] = 0x4;
    if (way <= 0x3)
        context->cacheTagArray[line][way] = 0x0;
    context->cacheLRU[line] = 0;
}

u32 FASTCALL AddressArrayReadLong(SH2_struct *context, u32 addr)
{
    if (yabsys.usecache == 0)
        return 0;
    u8 line = (addr >> 4) & 0x3F;
    u8 way  = (context->onchip.CCR >> 6) & 0x3;
    return ((context->cacheLRU[line] & 0x3F) << 4) | ((context->cacheTagArray[line][way] & 0x7FFFF) << 10) |
           ((context->cacheTagArray[line][way] != 0x0) << 1);
    return 0;
}

void FASTCALL AddressArrayWriteLong(SH2_struct *context, u32 addr, u32 val)
{
    if (yabsys.usecache == 0)
        return;
    u8  line                = (addr >> 4) & 0x3F;
    u32 tag                 = (addr >> 10) & 0x7FFFF;
    u8  valid               = (addr >> 2) & 0x1;
    u8  way                 = (context->onchip.CCR >> 6) & 0x3;
    context->cacheLRU[line] = (val >> 4) & 0x3F;
    if (valid) {
        context->tagWay[line][tag]        = way;
        context->cacheTagArray[line][way] = tag;
    } else {
        context->tagWay[line][tag]        = 0x4;
        context->cacheTagArray[line][way] = 0x0;
    }
}

u8 FASTCALL DataArrayReadByte(SH2_struct *context, u32 addr)
{
    return T2ReadByte(context->DataArray, addr & 0xFFF);
}

u16 FASTCALL DataArrayReadWord(SH2_struct *context, u32 addr)
{
    return T2ReadWord(context->DataArray, addr & 0xFFF);
}

u32 FASTCALL DataArrayReadLong(SH2_struct *context, u32 addr)
{
    return T2ReadLong(context->DataArray, addr & 0xFFF);
}

void FASTCALL DataArrayWriteByte(SH2_struct *context, u32 addr, u8 val)
{
    T2WriteByte(context->DataArray, addr & 0xFFF, val);
}

void FASTCALL DataArrayWriteWord(SH2_struct *context, u32 addr, u16 val)
{
    T2WriteWord(context->DataArray, addr & 0xFFF, val);
}

void FASTCALL DataArrayWriteLong(SH2_struct *context, u32 addr, u32 val)
{
    T2WriteLong(context->DataArray, addr & 0xFFF, val);
}

void FRTExec(SH2_struct *context, u32 cycles)
{
    u32 frcold;
    u32 frctemp;
    u32 mask;

    frcold = frctemp = (u32)context->onchip.FRC.all;
    mask             = (1 << context->frc.shift) - 1;

    frctemp += ((cycles + context->frc.leftover) >> context->frc.shift);
    context->frc.leftover = (cycles + context->frc.leftover) & mask;

    if (frctemp >= context->onchip.OCRA && frcold < context->onchip.OCRA) {
        if (context->onchip.TIER & 0x8)
            SH2SendInterrupt(context, context->onchip.VCRC & 0x7F, (context->onchip.IPRB & 0xF00) >> 8);

        if (context->onchip.FTCSR & 0x1) {
            frctemp               = 0;
            context->frc.leftover = 0;
        }

        context->onchip.FTCSR |= 0x8;
    }

    if (frctemp >= context->onchip.OCRB && frcold < context->onchip.OCRB) {
        if (context->onchip.TIER & 0x4)
            SH2SendInterrupt(context, context->onchip.VCRC & 0x7F, (context->onchip.IPRB & 0xF00) >> 8);

        context->onchip.FTCSR |= 0x4;
    }

    if (frctemp > 0xFFFF) {
        if (context->onchip.TIER & 0x2)
            SH2SendInterrupt(context, (context->onchip.VCRD >> 8) & 0x7F, (context->onchip.IPRB & 0xF00) >> 8);

        context->onchip.FTCSR |= 2;
    }

    context->onchip.FRC.all = frctemp;
}

void WDTExec(SH2_struct *context, u32 cycles)
{
    u32 wdttemp;
    u32 mask;

    if (!context->wdt.isenable || context->onchip.WTCSR & 0x80 || context->onchip.RSTCSR & 0x80)
        return;

    wdttemp = (u32)context->onchip.WTCNT;
    mask    = (1 << context->wdt.shift) - 1;
    wdttemp += ((cycles + context->wdt.leftover) >> context->wdt.shift);
    context->wdt.leftover = (cycles + context->wdt.leftover) & mask;

    if (wdttemp > 0xFF) {

        if (context->wdt.isinterval) {

            context->onchip.WTCSR |= 0x80;

            SH2SendInterrupt(context, (context->onchip.VCRWDT >> 8) & 0x7F, (context->onchip.IPRA >> 4) & 0xF);
        } else {
            LOG("Watchdog timer(WDT mode) overflow not implemented\n");
        }
    }

    context->onchip.WTCNT = (u8)wdttemp;
}

void DMAExec(SH2_struct *context)
{
    if (context->onchip.DMAOR & 0x6)
        return;

    if (((context->onchip.CHCR0 & 0x3) == 0x01) && ((context->onchip.CHCR1 & 0x3) == 0x01)) {
        if (context->onchip.DMAOR & 0x8) {
            LOG("dma\t: FIXME: two channel dma - round robin priority not properly implemented\n");
            DMATransfer(context, &context->onchip.CHCR0, &context->onchip.SAR0, &context->onchip.DAR0,
                        &context->onchip.TCR0, &context->onchip.VCRDMA0);
            DMATransfer(context, &context->onchip.CHCR1, &context->onchip.SAR1, &context->onchip.DAR1,
                        &context->onchip.TCR1, &context->onchip.VCRDMA1);

            context->onchip.CHCR1M |= 2;
        } else {
            DMATransfer(context, &context->onchip.CHCR0, &context->onchip.SAR0, &context->onchip.DAR0,
                        &context->onchip.TCR0, &context->onchip.VCRDMA0);
            DMATransfer(context, &context->onchip.CHCR1, &context->onchip.SAR1, &context->onchip.DAR1,
                        &context->onchip.TCR1, &context->onchip.VCRDMA1);
            context->onchip.CHCR1M |= 2;
        }
    } else {
        if (((context->onchip.CHCR0 & 0x3) == 0x01)) {
            DMATransfer(context, &context->onchip.CHCR0, &context->onchip.SAR0, &context->onchip.DAR0,
                        &context->onchip.TCR0, &context->onchip.VCRDMA0);
            return;
        }
        if (((context->onchip.CHCR1 & 0x3) == 0x01)) {
            DMATransfer(context, &context->onchip.CHCR1, &context->onchip.SAR1, &context->onchip.DAR1,
                        &context->onchip.TCR1, &context->onchip.VCRDMA1);
            context->onchip.CHCR1M |= 2;
            return;
        }
    }
}

void DMATransfer(SH2_struct *context, u32 *CHCR, u32 *SAR, u32 *DAR, u32 *TCR, u32 *VCRDMA)
{
    int size;
    u32 i, i2;

    if (!(*CHCR & 0x2)) {
        int srcInc;
        int destInc;

        switch (*CHCR & 0x3000) {
            case 0x0000:
                srcInc = 0;
                break;
            case 0x1000:
                srcInc = 1;
                break;
            case 0x2000:
                srcInc = -1;
                break;
            default:
                srcInc = 0;
                break;
        }

        switch (*CHCR & 0xC000) {
            case 0x0000:
                destInc = 0;
                break;
            case 0x4000:
                destInc = 1;
                break;
            case 0x8000:
                destInc = -1;
                break;
            default:
                destInc = 0;
                break;
        }

        switch (size = ((*CHCR & 0x0C00) >> 10)) {
            case 0:
                for (i = 0; i < *TCR; i++) {
                    DMAMappedMemoryWriteByte(*DAR, DMAMappedMemoryReadByte(*SAR));
                    *SAR += srcInc;
                    *DAR += destInc;
                }

                *TCR = 0;
                break;
            case 1:
                destInc *= 2;
                srcInc *= 2;

                for (i = 0; i < *TCR; i++) {
                    DMAMappedMemoryWriteWord(*DAR, DMAMappedMemoryReadWord(*SAR));
                    *SAR += srcInc;
                    *DAR += destInc;
                }

                *TCR = 0;
                break;
            case 2:
                destInc *= 4;
                srcInc *= 4;

                for (i = 0; i < *TCR; i++) {
                    DMAMappedMemoryWriteLong(*DAR, DMAMappedMemoryReadLong(*SAR));
                    *DAR += destInc;
                    *SAR += srcInc;
                }

                *TCR = 0;
                break;
            case 3: {
                u32 buffer[4];
                u32 show = 0;
                destInc *= 4;
                srcInc *= 4;
                for (i = 0; i < *TCR; i += 4) {
                    for (i2 = 0; i2 < 4; i2++) {
                        buffer[i2] = MappedMemoryReadLong(context, ((*SAR + (i2 << 2)) & 0x07FFFFFC));
                    }
                    *SAR += 0x10;
                    for (i2 = 0; i2 < 4; i2++) {
                        MappedMemoryWriteLong(context, *DAR & 0x07FFFFFC, buffer[i2]);
                        *DAR += destInc;
                    }
                }
                *TCR = 0;
            } break;
        }
        SH2WriteNotify(context, destInc < 0 ? *DAR : *DAR - i * destInc, i * abs(destInc));
    }

    if (*CHCR & 0x4)
        SH2SendInterrupt(context, *VCRDMA, (context->onchip.IPRA & 0xF00) >> 8);

    *CHCR |= 0x2;
}

extern u8 execInterrupt;

void FASTCALL MSH2InputCaptureWriteWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u16 data)
{
    MSH2->onchip.FTCSR |= 0x80;

    MSH2->onchip.FICR = MSH2->onchip.FRC.all;

    LOG("MSH2InputCapture\n");

    if (MSH2->onchip.TIER & 0x80) {
        SH2SendInterrupt(MSH2, (MSH2->onchip.VCRC >> 8) & 0x7F, (MSH2->onchip.IPRB >> 8) & 0xF);
        execInterrupt = 1;
    }
}

void FASTCALL SSH2InputCaptureWriteWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u16 data)
{
    SSH2->onchip.FTCSR |= 0x80;

    SSH2->onchip.FICR = SSH2->onchip.FRC.all;

    LOG("SSH2InputCapture\n");

    if (SSH2->onchip.TIER & 0x80) {
        SH2SendInterrupt(SSH2, (SSH2->onchip.VCRC >> 8) & 0x7F, (SSH2->onchip.IPRB >> 8) & 0xF);
        execInterrupt = 1;
    }
}

u8 SCIReceiveByte(void)
{
    return 0;
}

void SCITransmitByte(UNUSED u8 val)
{
}

int SH2SaveState(SH2_struct *context, void **stream)
{
    int            offset;
    sh2regs_struct regs;

    if (context->isslave == 0)
        offset = MemStateWriteHeader(stream, "MSH2", 3);
    else {
        offset = MemStateWriteHeader(stream, "SSH2", 3);
        MemStateWrite((void *)&yabsys.IsSSH2Running, 1, 1, stream);
    }

    SH2GetRegisters(context, &regs);
    MemStateWrite((void *)&regs, sizeof(sh2regs_struct), 1, stream);

    MemStateWrite((void *)&context->onchip, sizeof(Onchip_struct), 1, stream);

    context->frc.shift = 1 << context->frc.shift;
    MemStateWrite((void *)&context->frc, sizeof(context->frc), 1, stream);
    {
        u32 div            = context->frc.shift;
        context->frc.shift = 0;
        while ((div >>= 1) != 0)
            context->frc.shift++;
    }
    context->NumberOfInterrupts = SH2Core->GetInterrupts(context, context->interrupts);
    MemStateWrite((void *)context->interrupts, sizeof(interrupt_struct), MAX_INTERRUPTS, stream);
    MemStateWrite((void *)&context->NumberOfInterrupts, sizeof(u32), 1, stream);
    MemStateWrite((void *)context->AddressArray, sizeof(u32), 0x100, stream);
    MemStateWrite((void *)context->DataArray, sizeof(u8), 0x1000, stream);
    MemStateWrite((void *)&context->delay, sizeof(u32), 1, stream);
    MemStateWrite((void *)&context->cycles, sizeof(u32), 1, stream);
    MemStateWrite((void *)&context->isslave, sizeof(u8), 1, stream);
    MemStateWrite((void *)&context->instruction, sizeof(u16), 1, stream);

    MemStateWrite((void *)&context->dma_ch0.copy_clock, sizeof(u32), 1, stream);
    MemStateWrite((void *)&context->dma_ch1.copy_clock, sizeof(u32), 1, stream);

    return MemStateFinishHeader(stream, offset);
}

int SH2LoadState(SH2_struct *context, const void *stream, UNUSED int version, int size)
{
    sh2regs_struct regs;

    SH2Reset(context);

    if (context->isslave == 1)
        MemStateRead((void *)&yabsys.IsSSH2Running, 1, 1, stream);

    MemStateRead((void *)&regs, sizeof(sh2regs_struct), 1, stream);
    SH2SetRegisters(context, &regs);

    if (version < 2) {
        MemStateRead((void *)&context->onchip, sizeof(Onchip_struct) - sizeof(u32), 1, stream);
    } else {
        MemStateRead((void *)&context->onchip, sizeof(Onchip_struct), 1, stream);
    }

    MemStateRead((void *)&context->frc, sizeof(context->frc), 1, stream);
    {
        u32 div            = context->frc.shift;
        context->frc.shift = 0;
        while ((div >>= 1) != 0)
            context->frc.shift++;
    }
    MemStateRead((void *)context->interrupts, sizeof(interrupt_struct), MAX_INTERRUPTS, stream);
    MemStateRead((void *)&context->NumberOfInterrupts, sizeof(u32), 1, stream);
    SH2Core->SetInterrupts(context, context->NumberOfInterrupts, context->interrupts);
    MemStateRead((void *)context->AddressArray, sizeof(u32), 0x100, stream);
    MemStateRead((void *)context->DataArray, sizeof(u8), 0x1000, stream);
    MemStateRead((void *)&context->delay, sizeof(u32), 1, stream);
    MemStateRead((void *)&context->cycles, sizeof(u32), 1, stream);
    MemStateRead((void *)&context->isslave, sizeof(u8), 1, stream);
    MemStateRead((void *)&context->instruction, sizeof(u16), 1, stream);

    if (version >= 3) {
        MemStateRead((void *)&context->dma_ch0.copy_clock, sizeof(u32), 1, stream);
        MemStateRead((void *)&context->dma_ch1.copy_clock, sizeof(u32), 1, stream);
    }

    return size;
}

void SH2DumpHistory(SH2_struct *context)
{
}
