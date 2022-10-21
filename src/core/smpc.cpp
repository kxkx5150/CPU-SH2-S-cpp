

#include <stdlib.h>
#include <time.h>
#include "smpc.h"
#include "cs2.h"
#include "debug.h"
#include "peripheral.h"
#include "scsp.h"
#include "scu.h"
#include "sh2core.h"
#include "vdp1.h"
#include "vdp2.h"
#include "yabause.h"
#include "movie.h"
#include "eeprom.h"

#ifdef _arch_dreamcast
#include "dreamcast/localtime.h"
#endif
#ifdef PSP
#include "psp/localtime.h"
#endif

Smpc         *SmpcRegs;
SmpcInternal *SmpcInternalVars;

static u8         *SmpcRegsT;
static int         intback_wait_for_line = 0;
static u8          bustmp                = 0;
static const char *smpcfilename          = NULL;

int SmpcInit(u8 regionid, int clocksync, u32 basetime, const char *smpcpath, u8 languageid)
{
    if ((SmpcRegsT = (u8 *)calloc(1, sizeof(Smpc))) == NULL)
        return -1;

    SmpcRegs = (Smpc *)SmpcRegsT;

    if ((SmpcInternalVars = (SmpcInternal *)calloc(1, sizeof(SmpcInternal))) == NULL)
        return -1;

    SmpcInternalVars->regionsetting = regionid;
    SmpcInternalVars->regionid      = regionid;
    SmpcInternalVars->clocksync     = clocksync;
    SmpcInternalVars->basetime      = basetime ? basetime : time(NULL);
    SmpcInternalVars->languageid    = languageid;

    smpcfilename = smpcpath;

    return 0;
}

void SmpcDeInit(void)
{
    if (SmpcRegsT)
        free(SmpcRegsT);
    SmpcRegsT = NULL;

    if (SmpcInternalVars)
        free(SmpcInternalVars);
    SmpcInternalVars = NULL;
}

void SmpcRecheckRegion(void)
{
    if (SmpcInternalVars == NULL)
        return;

    if (SmpcInternalVars->regionsetting == REGION_AUTODETECT) {
        SmpcInternalVars->regionid = Cs2GetRegionID();

        if (SmpcInternalVars->regionid == 0)
            SmpcInternalVars->regionid = 1;
    } else
        Cs2GetIP(0);
}

static int SmpcSaveBiosSettings(void)
{
    FILE *fp;
    if (smpcfilename == NULL)
        return -1;
    if ((fp = fopen(smpcfilename, "wb")) == NULL)
        return -1;
    fwrite(SmpcInternalVars->SMEM, 1, sizeof(SmpcInternalVars->SMEM), fp);
    fclose(fp);
    return 0;
}

static int SmpcLoadBiosSettings(void)
{
    FILE  *fp;
    size_t nbRead = 0;
    if (smpcfilename == NULL)
        return -1;
    if ((fp = fopen(smpcfilename, "rb")) == NULL)
        return -1;
    nbRead                       = fread(SmpcInternalVars->SMEM, 1, sizeof(SmpcInternalVars->SMEM), fp);
    SmpcInternalVars->languageid = SmpcInternalVars->SMEM[3] & 0xF;
    fclose(fp);
    return (nbRead == sizeof(SmpcInternalVars->SMEM)) ? 0 : -1;
}

static void SmpcSetLanguage(void)
{
    SmpcInternalVars->SMEM[3] = (SmpcInternalVars->SMEM[3] & 0xF0) | SmpcInternalVars->languageid;
}

int SmpcGetLanguage(void)
{
    return SmpcInternalVars->languageid;
}

void SmpcReset(void)
{
    memset((void *)SmpcRegs, 0, sizeof(Smpc));
    memset((void *)SmpcInternalVars->SMEM, 0, 4);

    SmpcRecheckRegion();
    SmpcLoadBiosSettings();
    SmpcSetLanguage();

    SmpcInternalVars->dotsel = 0;
    SmpcInternalVars->mshnmi = 0;
    SmpcInternalVars->sysres = 0;
    SmpcInternalVars->sndres = 0;
    SmpcInternalVars->cdres  = 0;
    SmpcInternalVars->resd   = 1;
    SmpcInternalVars->ste    = 0;
    SmpcInternalVars->resb   = 0;

    SmpcInternalVars->firstPeri = 0;

    SmpcInternalVars->timing = 0;

    memset((void *)&SmpcInternalVars->port1, 0, sizeof(PortData_struct));
    memset((void *)&SmpcInternalVars->port2, 0, sizeof(PortData_struct));
    SmpcRegs->OREG[31] = 0xD;
}

static void SmpcSSHON(void)
{
    YabauseStartSlave();
}

static void SmpcSSHOFF(void)
{
    YabauseStopSlave();
}

static void SmpcSNDON(void)
{
    if (!yabsys.isSTV)
        M68KStart();
    SmpcRegs->OREG[31] = 0x6;
}

static void SmpcSNDOFF(void)
{
    if (!yabsys.isSTV)
        M68KStop();
    SmpcRegs->OREG[31] = 0x7;
}

static void SmpcSYSRES(void)
{
    SmpcRegs->OREG[31] = 0xD;
}

void SmpcCKCHG352(void)
{
    SmpcInternalVars->dotsel = 1;

    SH2NMI(MSH2);
    Vdp1Reset();
    Vdp2Reset();
    ScuReset();
    ScspReset();

    YabauseStopSlave();

    YabauseChangeTiming(CLKTYPE_28MHZ);
}

void SmpcCKCHG320(void)
{

    SmpcInternalVars->dotsel = 0;

    SH2NMI(MSH2);

    Vdp1Reset();
    Vdp2Reset();
    ScuReset();
    ScspReset();

    YabauseStopSlave();

    YabauseChangeTiming(CLKTYPE_26MHZ);
}

struct movietime
{

    int tm_year;
    int tm_wday;
    int tm_mon;
    int tm_mday;
    int tm_hour;
    int tm_min;
    int tm_sec;
};

static struct movietime movietime;
int                     totalseconds;
int                     noon = 43200;

static void SmpcINTBACKStatus(void)
{
    int       i;
    struct tm times;
    u8        year[4];
    time_t    tmp;

    SmpcRegs->OREG[0] = 0x80 | (SmpcInternalVars->resd << 6);

    if (SmpcInternalVars->clocksync) {
        tmp = SmpcInternalVars->basetime + ((u64)framecounter * 1001 / 60000);
    } else {
        tmp = time(NULL);
    }
#ifdef WIN32
    memcpy(&times, localtime(&tmp), sizeof(times));
#elif defined(_arch_dreamcast) || defined(PSP)
    internal_localtime_r(&tmp, &times);
#else
    localtime_r(&tmp, &times);
#endif
    year[0]           = (1900 + times.tm_year) / 1000;
    year[1]           = ((1900 + times.tm_year) % 1000) / 100;
    year[2]           = (((1900 + times.tm_year) % 1000) % 100) / 10;
    year[3]           = (((1900 + times.tm_year) % 1000) % 100) % 10;
    SmpcRegs->OREG[1] = (year[0] << 4) | year[1];
    SmpcRegs->OREG[2] = (year[2] << 4) | year[3];
    SmpcRegs->OREG[3] = (times.tm_wday << 4) | (times.tm_mon + 1);
    SmpcRegs->OREG[4] = ((times.tm_mday / 10) << 4) | (times.tm_mday % 10);
    SmpcRegs->OREG[5] = ((times.tm_hour / 10) << 4) | (times.tm_hour % 10);
    SmpcRegs->OREG[6] = ((times.tm_min / 10) << 4) | (times.tm_min % 10);
    SmpcRegs->OREG[7] = ((times.tm_sec / 10) << 4) | (times.tm_sec % 10);

    if (Movie.Status == Recording || Movie.Status == Playback) {
        movietime.tm_year = 0x62;
        movietime.tm_wday = 0x04;
        movietime.tm_mday = 0x01;
        movietime.tm_mon  = 0;
        totalseconds      = ((framecounter / 60) + noon);

        movietime.tm_sec  = totalseconds % 60;
        movietime.tm_min  = totalseconds / 60;
        movietime.tm_hour = movietime.tm_min / 60;

        movietime.tm_min  = movietime.tm_min % 60;
        movietime.tm_hour = movietime.tm_hour % 24;

        year[0]           = (1900 + movietime.tm_year) / 1000;
        year[1]           = ((1900 + movietime.tm_year) % 1000) / 100;
        year[2]           = (((1900 + movietime.tm_year) % 1000) % 100) / 10;
        year[3]           = (((1900 + movietime.tm_year) % 1000) % 100) % 10;
        SmpcRegs->OREG[1] = (year[0] << 4) | year[1];
        SmpcRegs->OREG[2] = (year[2] << 4) | year[3];
        SmpcRegs->OREG[3] = (movietime.tm_wday << 4) | (movietime.tm_mon + 1);
        SmpcRegs->OREG[4] = ((movietime.tm_mday / 10) << 4) | (movietime.tm_mday % 10);
        SmpcRegs->OREG[5] = ((movietime.tm_hour / 10) << 4) | (movietime.tm_hour % 10);
        SmpcRegs->OREG[6] = ((movietime.tm_min / 10) << 4) | (movietime.tm_min % 10);
        SmpcRegs->OREG[7] = ((movietime.tm_sec / 10) << 4) | (movietime.tm_sec % 10);
    }

    SmpcRegs->OREG[8] = 0;

    SmpcRegs->OREG[9] = SmpcInternalVars->regionid;

    SmpcRegs->OREG[10] = 0x34 | (SmpcInternalVars->dotsel << 6) | (SmpcInternalVars->mshnmi << 3) |
                         (SmpcInternalVars->sysres << 1) | SmpcInternalVars->sndres;

    SmpcRegs->OREG[11] = SmpcInternalVars->cdres << 6;

    for (i = 0; i < 4; i++)
        SmpcRegs->OREG[12 + i] = SmpcInternalVars->SMEM[i];

    SmpcRegs->OREG[31] = 0x10;
}

static u16 m_pmode = 0;

static void SmpcINTBACKPeripheral(void)
{
    int              oregoffset;
    PortData_struct *port1, *port2;
    if (PERCore)
        PERCore->HandleEvents();
    if (SmpcInternalVars->firstPeri == 2) {
        SmpcRegs->SR                = 0x80 | m_pmode;
        SmpcInternalVars->firstPeri = 0;
    } else {
        SmpcRegs->SR = 0xC0 | m_pmode;
        SmpcInternalVars->firstPeri++;
    }

    oregoffset = 0;

    if (SmpcInternalVars->port1.size == 0 && SmpcInternalVars->port2.size == 0) {
        port1 = &PORTDATA1;
        port2 = &PORTDATA2;
        memcpy(&SmpcInternalVars->port1, port1, sizeof(PortData_struct));
        memcpy(&SmpcInternalVars->port2, port2, sizeof(PortData_struct));
        PerFlush(&PORTDATA1);
        PerFlush(&PORTDATA2);
        SmpcInternalVars->port1.offset = 0;
        SmpcInternalVars->port2.offset = 0;
        LagFrameFlag                   = 0;
    }

    if (SmpcInternalVars->port1.size > 0) {
        if ((SmpcInternalVars->port1.size - SmpcInternalVars->port1.offset) < 32) {
            memcpy(SmpcRegs->OREG, SmpcInternalVars->port1.data + SmpcInternalVars->port1.offset,
                   SmpcInternalVars->port1.size - SmpcInternalVars->port1.offset);
            oregoffset += SmpcInternalVars->port1.size - SmpcInternalVars->port1.offset;
            SmpcInternalVars->port1.size = 0;
        } else {
            memcpy(SmpcRegs->OREG, SmpcInternalVars->port1.data, 32);
            oregoffset += 32;
            SmpcInternalVars->port1.offset += 32;
        }
    }
    if (SmpcInternalVars->port2.size > 0 && oregoffset < 32) {
        if ((SmpcInternalVars->port2.size - SmpcInternalVars->port2.offset) < (32 - oregoffset)) {
            memcpy(SmpcRegs->OREG + oregoffset, SmpcInternalVars->port2.data + SmpcInternalVars->port2.offset,
                   SmpcInternalVars->port2.size - SmpcInternalVars->port2.offset);
            SmpcInternalVars->port2.size = 0;
        } else {
            memcpy(SmpcRegs->OREG + oregoffset, SmpcInternalVars->port2.data, 32 - oregoffset);
            SmpcInternalVars->port2.offset += 32 - oregoffset;
        }
    }
}

static void SmpcINTBACK(void)
{
    if (SmpcInternalVars->firstPeri == 1) {
        SmpcINTBACKPeripheral();
        SmpcRegs->SF       = 0;
        SmpcRegs->OREG[31] = 0x10;
        ScuSendSystemManager();
        return;
    }
    if (SmpcRegs->IREG[0] != 0x0) {
        SmpcInternalVars->firstPeri = ((SmpcRegs->IREG[1] & 0x8) >> 3);
        for (int i = 0; i < 31; i++)
            SmpcRegs->OREG[i] = 0xff;
        m_pmode = (SmpcRegs->IREG[0] >> 4);
        SmpcINTBACKStatus();
        SmpcRegs->SR = 0x40 | (SmpcInternalVars->firstPeri << 5);
        SmpcRegs->SF = 0;
        ScuSendSystemManager();
        return;
    }
    if (SmpcRegs->IREG[1] & 0x8) {
        SmpcInternalVars->firstPeri = ((SmpcRegs->IREG[1] & 0x8) >> 3);
        SmpcINTBACKPeripheral();
        SmpcRegs->OREG[31] = 0x10;
        ScuSendSystemManager();
        SmpcRegs->SF = 0;
    } else {
        SMPCLOG("Nothing to do\n");
        SmpcRegs->OREG[31] = 0x10;
        SmpcRegs->SF       = 0;
    }
}

static void SmpcSETSMEM(void)
{
    int i;

    for (i = 0; i < 4; i++)
        SmpcInternalVars->SMEM[i] = SmpcRegs->IREG[i];

    SmpcInternalVars->languageid = SmpcInternalVars->SMEM[3] & 0xF;
    SmpcSaveBiosSettings();

    SmpcRegs->OREG[31] = 0x17;
}

static void SmpcNMIREQ(void)
{
    SH2SendInterrupt(MSH2, 0xB, 16);
    SmpcRegs->OREG[31] = 0x18;
}

void SmpcResetButton(void)
{
    if (SmpcInternalVars->resd)
        return;

    SH2SendInterrupt(MSH2, 0xB, 16);
}

static void SmpcRESENAB(void)
{
    SmpcInternalVars->resd = 0;
    SmpcRegs->OREG[31]     = 0x19;
}

static void SmpcRESDISA(void)
{
    SmpcInternalVars->resd = 1;
    SmpcRegs->OREG[31]     = 0x1A;
}

static void processCommand(void)
{
    intback_wait_for_line = 0;
    switch (SmpcRegs->COMREG) {
        case 0x0:
            SMPCLOG("smpc\t: MSHON not implemented\n");
            SmpcRegs->OREG[31] = 0x0;
            SmpcRegs->SF       = 0;
            break;
        case 0x2:
            SMPCLOG("smpc\t: SSHON\n");
            SmpcSSHON();
            SmpcRegs->SF = 0;
            break;
        case 0x3:
            SMPCLOG("smpc\t: SSHOFF\n");
            SmpcSSHOFF();
            SmpcRegs->SF = 0;
            break;
        case 0x6:
            SMPCLOG("smpc\t: SNDON\n");
            SmpcSNDON();
            SmpcRegs->SF = 0;
            break;
        case 0x7:
            SMPCLOG("smpc\t: SNDOFF\n");
            SmpcSNDOFF();
            SmpcRegs->SF = 0;
            break;
        case 0x8:
            SMPCLOG("smpc\t: CDON not implemented\n");
            SmpcRegs->SF = 0;
            break;
        case 0x9:
            SMPCLOG("smpc\t: CDOFF not implemented\n");
            SmpcRegs->SF = 0;
            break;
        case 0xD:
            SMPCLOG("smpc\t: SYSRES not implemented\n");
            SmpcSYSRES();
            SmpcRegs->SF = 0;
            break;
        case 0xE:
            SMPCLOG("smpc\t: CKCHG352\n");
            SmpcCKCHG352();
            SmpcRegs->SF = 0;
            break;
        case 0xF:
            SMPCLOG("smpc\t: CKCHG320\n");
            SmpcCKCHG320();
            SmpcRegs->SF = 0;
            break;
        case 0x10:
            SMPCLOG("smpc\t: INTBACK\n");
            SmpcINTBACK();
            break;
        case 0x17:
            SMPCLOG("smpc\t: SETSMEM\n");
            SmpcSETSMEM();
            SmpcRegs->SF = 0;
            break;
        case 0x18:
            SMPCLOG("smpc\t: NMIREQ\n");
            SmpcNMIREQ();
            SmpcRegs->SF = 0;
            break;
        case 0x19:
            SMPCLOG("smpc\t: RESENAB\n");
            SmpcRESENAB();
            SmpcRegs->SF = 0;
            break;
        case 0x1A:
            SMPCLOG("smpc\t: RESDISA\n");
            SmpcRESDISA();
            SmpcRegs->SF = 0;
            break;
        default:
            printf("smpc\t: Command %02X not implemented\n", SmpcRegs->COMREG);
            break;
    }
}

void SmpcExec(s32 t)
{
    if (SmpcInternalVars->timing > 0) {

        if (intback_wait_for_line) {
            if (yabsys.LineCount == 207) {
                SmpcInternalVars->timing = -1;
                intback_wait_for_line    = 0;
            }
        }
        SmpcInternalVars->timing -= t;
        if (SmpcInternalVars->timing <= 0) {
            processCommand();
        }
    }
}

void SmpcINTBACKEnd(void)
{
}

static u8 m_pdr2_readback = 0;
static u8 m_pdr1_readback = 0;

u8 FASTCALL SmpcReadByte(SH2_struct *context, u8 *mem, u32 addr)
{
    addr &= 0x7F;
    if (addr == 0x063) {
        bustmp = SmpcRegsT[addr >> 1] & 0xFE;
        bustmp |= SmpcRegs->SF;
        SMPCLOG("Read SMPC[0x63] 0x%x\n", bustmp);
        return bustmp;
    }

    if ((addr == 0x77) && ((SmpcRegs->DDR[1] & 0x7F) == 0x18)) {
        u8 val = (((0x67 & ~0x19) | 0x18 | (eeprom_do_read() << 0)) & ~SmpcRegs->DDR[1]) | m_pdr2_readback;
        return val;
    }
    if ((addr == 0x75) && ((SmpcRegs->DDR[0] & 0x7F) == 0x3f)) {
        u8 val = (((0x40 & 0x40) | 0x3f) & ~SmpcRegs->DDR[0]) | m_pdr1_readback;
        return val;
    }

    if ((addr >= 0x21) && (addr <= 0x5D)) {
        if ((SmpcRegs->SF == 1) && (SmpcRegs->COMREG == 0x10)) {
            processCommand();
        }
    }

    if ((addr == 0x5F) && (addr <= 0x5D)) {
        if (SmpcRegs->SF == 1) {
            processCommand();
        }
    }

    SMPCLOG("Read SMPC[0x%x] = 0x%x\n", addr, SmpcRegsT[addr >> 1]);
    return SmpcRegsT[addr >> 1];
}

u16 FASTCALL SmpcReadWord(SH2_struct *context, UNUSED u8 *mem, USED_IF_SMPC_DEBUG u32 addr)
{
    SMPCLOG("smpc\t: SMPC register read word - %08X\n", addr);
    return 0;
}

u32 FASTCALL SmpcReadLong(SH2_struct *context, u8 *mem, USED_IF_SMPC_DEBUG u32 addr)
{
    SMPCLOG("smpc\t: SMPC register read long - %08X\n", addr);
    return 0;
}

static void SmpcSetTiming(void)
{
    switch (SmpcRegs->COMREG) {
        case 0x0:
            SMPCLOG("smpc\t: MSHON not implemented\n");
            SmpcInternalVars->timing = 1;
            return;
        case 0x8:
            SMPCLOG("smpc\t: CDON not implemented\n");
            SmpcInternalVars->timing = 1;
            return;
        case 0x9:
            SMPCLOG("smpc\t: CDOFF not implemented\n");
            SmpcInternalVars->timing = 1;
            return;
        case 0xD:
        case 0xE:
        case 0xF:
            SmpcInternalVars->timing = 1;
            return;
        case 0x10:
            if (SmpcInternalVars->firstPeri == 1) {
                SmpcInternalVars->timing = 16000;
                intback_wait_for_line    = 1;
            } else {

                if ((SmpcRegs->IREG[0] == 0x01) && (SmpcRegs->IREG[1] & 0x8)) {
                    SmpcInternalVars->timing = 250;
                } else if ((SmpcRegs->IREG[0] == 0x01) && ((SmpcRegs->IREG[1] & 0x8) == 0)) {
                    SmpcInternalVars->timing = 250;
                } else if ((SmpcRegs->IREG[0] == 0) && (SmpcRegs->IREG[1] & 0x8)) {
                    SmpcInternalVars->timing = 16000;
                    intback_wait_for_line    = 1;
                } else
                    SmpcInternalVars->timing = 1;
            }
            return;
        case 0x17:
            SmpcInternalVars->timing = 1;
            return;
        case 0x2:
            SmpcInternalVars->timing = 1;
            return;
        case 0x3:
            SmpcInternalVars->timing = 1;
            return;
        case 0x6:
        case 0x7:
        case 0x18:
        case 0x19:
        case 0x1A:
            SmpcInternalVars->timing = 1;
            return;
        default:
            SMPCLOG("smpc\t: unimplemented command: %02X\n", SmpcRegs->COMREG);
            SmpcRegs->SF = 0;
            break;
    }
}

u8 do_th_mode(u8 val)
{
    switch (val & 0x40) {
        case 0x40:
            return 0x70 | ((PORTDATA1.data[3] & 0xF) & 0xc);
            break;
        case 0x00:
            return 0x30 | ((PORTDATA1.data[2] >> 4) & 0xf);
            break;
    }

    return 0;
}

void FASTCALL SmpcWriteByte(SH2_struct *context, u8 *mem, u32 addr, u8 val)
{
    u8 oldVal;
    if (!(addr & 0x1))
        return;
    addr &= 0x7F;
    oldVal = SmpcRegsT[addr >> 1];
    bustmp = val;
    if (addr == 0x1F) {
        SmpcRegsT[0xF] = val & 0x1F;
    } else
        SmpcRegsT[addr >> 1] = val;

    SMPCLOG("Write SMPC[0x%x] = 0x%x\n", addr, SmpcRegsT[addr >> 1]);

    switch (addr) {
        case 0x01:
            if (SmpcInternalVars->firstPeri != 0) {
                if (SmpcRegs->IREG[0] & 0x40) {
                    SMPCLOG("INTBACK Break\n");
                    SmpcInternalVars->firstPeri = 0;
                    SmpcRegs->SR &= 0x0F;
                    SmpcRegs->SF = 0;
                    break;
                } else if (SmpcRegs->IREG[0] & 0x80) {
                    SMPCLOG("INTBACK Continue\n");
                    SmpcSetTiming();
                    SmpcRegs->SF = 1;
                }
            }
            return;
        case 0x1F:
            SmpcSetTiming();
            return;
        case 0x63:
            SmpcRegs->SF &= val;
            return;
        case 0x75:
            switch (SmpcRegs->DDR[0] & 0x7F) {
                case 0x00:
                    if (PORTDATA1.data[1] == PERGUN && (val & 0x7F) == 0x7F)
                        SmpcRegs->PDR[0] = PORTDATA1.data[2];
                    break;
                case 0x40:
                    SmpcRegs->PDR[0] = do_th_mode(val);
                    break;
                case 0x60:
                    switch (val & 0x60) {
                        case 0x60:
                            val = (val & 0x80) | 0x14 | (PORTDATA1.data[3] & 0x8);
                            break;
                        case 0x20:
                            val = (val & 0x80) | 0x10 | ((PORTDATA1.data[2] >> 4) & 0xF);
                            break;
                        case 0x40:
                            val = (val & 0x80) | 0x10 | (PORTDATA1.data[2] & 0xF);
                            break;
                        case 0x00:
                            val = (val & 0x80) | 0x10 | ((PORTDATA1.data[3] >> 4) & 0xF);
                            break;
                        default:
                            break;
                    }

                    SmpcRegs->PDR[0] = val;
                    break;
                case 0x3f:
                    m_pdr1_readback = (val & SmpcRegs->DDR[0]) & 0x7f;
                    eeprom_set_clk((val & 0x08) ? 1 : 0);
                    eeprom_set_di((val >> 4) & 1);
                    eeprom_set_cs((val & 0x04) ? 1 : 0);
                    SmpcRegs->PDR[0] = m_pdr1_readback;
                    m_pdr1_readback |= (val & 0x80);
                    break;
                default:
                    SMPCLOG("smpc\t: PDR1 Peripheral Unknown Control Method not implemented 0x%x\n",
                            SmpcRegs->DDR[0] & 0x7F);
                    break;
            }
            break;
        case 0x77:
            switch (SmpcRegs->DDR[1] & 0x7F) {
                case 0x00:
                    if (PORTDATA2.data[1] == PERGUN && (val & 0x7F) == 0x7F)
                        SmpcRegs->PDR[1] = PORTDATA2.data[2];
                    break;
                case 0x60:
                    switch (val & 0x60) {
                        case 0x60:
                            val = (val & 0x80) | 0x14 | (PORTDATA2.data[3] & 0x8);
                            break;
                        case 0x20:
                            val = (val & 0x80) | 0x10 | ((PORTDATA2.data[2] >> 4) & 0xF);
                            break;
                        case 0x40:
                            val = (val & 0x80) | 0x10 | (PORTDATA2.data[2] & 0xF);
                            break;
                        case 0x00:
                            val = (val & 0x80) | 0x10 | ((PORTDATA2.data[3] >> 4) & 0xF);
                            break;
                        default:
                            break;
                    }

                    SmpcRegs->PDR[1] = val;
                    break;
                case 0x18:
                    m_pdr2_readback = ((val & SmpcRegs->DDR[1]) & 0x7F);
                    if (m_pdr2_readback & 0x10) {
                        M68KStop();
                    } else {
                        M68KStart();
                    }
                    SmpcRegs->PDR[1] = m_pdr2_readback;
                    m_pdr2_readback |= val & 0x80;
                    break;
                default:
                    SMPCLOG("smpc\t: PDR2 Peripheral Unknown Control Method not implemented 0x%x\n",
                            SmpcRegs->DDR[1] & 0x7F);
                    break;
            }
            break;
        case 0x79:
            switch (SmpcRegs->DDR[0] & 0x7F) {
                case 0x00:
                case 0x40:
                    switch (PORTDATA1.data[0]) {
                        case 0xA0: {
                            if (PORTDATA1.data[1] == PERGUN)
                                SmpcRegs->PDR[0] = 0x7C;
                            break;
                        }
                        case 0xF0:
                            SmpcRegs->PDR[0] = 0x7F;
                            break;
                        case 0xF1: {
                            switch (PORTDATA1.data[1]) {
                                case PERPAD:
                                    SmpcRegs->PDR[0] = 0x7C;
                                    break;
                                case PER3DPAD:
                                case PERKEYBOARD:
                                    SmpcRegs->PDR[0] = 0x71;
                                    break;
                                case PERMOUSE:
                                    SmpcRegs->PDR[0] = 0x70;
                                    break;
                                case PERWHEEL:
                                case PERMISSIONSTICK:
                                case PERTWINSTICKS:
                                default:
                                    SMPCLOG(
                                        "smpc\t: Peripheral TH Control Method not supported for peripherl id %02X\n",
                                        PORTDATA1.data[1]);
                                    break;
                            }
                            break;
                        }
                        default:
                            SmpcRegs->PDR[0] = 0x71;
                            break;
                    }

                    break;
                default:
                    break;
            }
            break;
        case 0x7B:
            SmpcRegs->DDR[1] = (val & 0x7F);
            break;
        case 0x7D:
            SmpcRegs->IOSEL = val;
            break;
        case 0x7F:
            SmpcRegs->EXLE = val;
            break;
        default:
            return;
    }
}

void FASTCALL SmpcWriteWord(SH2_struct *context, UNUSED u8 *mem, USED_IF_SMPC_DEBUG u32 addr, UNUSED u16 val)
{
    SMPCLOG("smpc\t: SMPC register write word - %08X\n", addr);
}

void FASTCALL SmpcWriteLong(SH2_struct *context, UNUSED u8 *mem, USED_IF_SMPC_DEBUG u32 addr, UNUSED u32 val)
{
    SMPCLOG("smpc\t: SMPC register write long - %08X\n", addr);
}

int SmpcSaveState(void **stream)
{
    int offset;

    offset = MemStateWriteHeader(stream, "SMPC", 3);

    MemStateWrite((void *)SmpcRegs->IREG, sizeof(u8), 7, stream);
    MemStateWrite((void *)&SmpcRegs->COMREG, sizeof(u8), 1, stream);
    MemStateWrite((void *)SmpcRegs->OREG, sizeof(u8), 32, stream);
    MemStateWrite((void *)&SmpcRegs->SR, sizeof(u8), 1, stream);
    MemStateWrite((void *)&SmpcRegs->SF, sizeof(u8), 1, stream);
    MemStateWrite((void *)SmpcRegs->PDR, sizeof(u8), 2, stream);
    MemStateWrite((void *)SmpcRegs->DDR, sizeof(u8), 2, stream);
    MemStateWrite((void *)&SmpcRegs->IOSEL, sizeof(u8), 1, stream);
    MemStateWrite((void *)&SmpcRegs->EXLE, sizeof(u8), 1, stream);

    MemStateWrite((void *)SmpcInternalVars, sizeof(SmpcInternal), 1, stream);

    return MemStateFinishHeader(stream, offset);
}

int SmpcLoadState(const void *stream, int version, int size)
{
    int internalsizev2 = sizeof(SmpcInternal) - 8;

    MemStateRead((void *)SmpcRegs->IREG, sizeof(u8), 7, stream);
    MemStateRead((void *)&SmpcRegs->COMREG, sizeof(u8), 1, stream);
    MemStateRead((void *)SmpcRegs->OREG, sizeof(u8), 32, stream);
    MemStateRead((void *)&SmpcRegs->SR, sizeof(u8), 1, stream);
    MemStateRead((void *)&SmpcRegs->SF, sizeof(u8), 1, stream);
    MemStateRead((void *)SmpcRegs->PDR, sizeof(u8), 2, stream);
    MemStateRead((void *)SmpcRegs->DDR, sizeof(u8), 2, stream);
    MemStateRead((void *)&SmpcRegs->IOSEL, sizeof(u8), 1, stream);
    MemStateRead((void *)&SmpcRegs->EXLE, sizeof(u8), 1, stream);

    if (version == 1) {
        if ((size - 48) == internalsizev2)
            MemStateRead((void *)SmpcInternalVars, internalsizev2, 1, stream);
        else if ((size - 48) == 24)
            MemStateRead((void *)SmpcInternalVars, 24, 1, stream);
        else
            MemStateSetOffset(size - 48);
    } else if (version == 2)
        MemStateRead((void *)SmpcInternalVars, internalsizev2, 1, stream);
    else
        MemStateRead((void *)SmpcInternalVars, sizeof(SmpcInternal), 1, stream);

    return size;
}
