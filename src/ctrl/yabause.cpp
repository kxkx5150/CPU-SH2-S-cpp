#include <cstdio>
#include <sys/types.h>
#include <sys/time.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <string.h>
#include "yabause.h"
#include "cs0.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "m68kcore.h"
#include "peripheral.h"
#include "scsp.h"
#include "scspdsp.h"
#include "scu.h"
#include "sh2core.h"
#include "smpc.h"
#include "ygl.h"
#include "vdp2.h"
#include "yui.h"
#include "bios.h"
#include "movie.h"
#include "stv.h"
#ifdef HAVE_LIBSDL
#if defined(__APPLE__) || defined(GEKKO)
#ifdef HAVE_LIBSDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif
#endif
#if defined(_MSC_VER) || !defined(HAVE_SYS_TIME_H)
#include <time.h>
#else
#include <sys/time.h>
#endif
#ifdef _arch_dreamcast
#include <arch/timer.h>
#endif
#ifdef GEKKO
#include <ogc/lwp_watchdog.h>
#endif
#ifdef PSP
#include "psp/common.h"
#endif
#ifdef SYS_
#include SYS_
#else
#endif
#if HAVE_GDBSTUB
#include "gdb/stub.h"
#endif
#ifdef _USE_PERFETTO_TRACE_
#include <fstream>
PERFETTO_TRACK_EVENT_STATIC_STORAGE();
#endif
#define THREAD_LOG
yabsys_struct yabsys;
u64           tickfreq;
ScspDsp       scsp_dsp          = {0};
u32           saved_scsp_cycles = 0;
volatile u64  saved_m68k_cycles = 0;
#ifndef NO_CLI
void print_usage(const char *program_name)
{
}
#endif
static unsigned long nextFrameTime     = 0;
static int           autoframeskipenab = 0;
#ifdef TIMING_SWAP
void YuiTimedSwapBuffers()
{
    unsigned long now = YabauseGetTicks();
    YuiSwapBuffers();
    YuiMsg("delay %ld\n", YabauseGetTicks() - now);
}
#endif
static void syncVideoMode(void)
{
    unsigned long sleep = 0;
    unsigned long now;
    unsigned long delay = 0;
    YuiEndOfFrame();
    now = YabauseGetTicks();
    if (nextFrameTime == 0)
        nextFrameTime = YabauseGetTicks();
    if (nextFrameTime > now) {
        sleep = ((nextFrameTime - now) * 1000000.0) / yabsys.tickfreq;
    } else {
        delay = nextFrameTime - now;
    }
    if (isAutoFrameSkip() == 0) {
        YabThreadUSleep(sleep);
        now = YabauseGetTicks();
    }
    nextFrameTime = now + yabsys.OneFrameTime + delay;
}
void resetSyncVideo(void)
{
    nextFrameTime = 0;
    resetFrameSkip();
}
void YuiEndOfFrame()
{
}
void YabauseChangeTiming(int freqtype)
{
    const double freq_base     = yabsys.IsPal ? 28437500.0 : (39375000.0 / 11.0) * 8.0;
    const double freq_mult     = (freqtype == CLKTYPE_26MHZ) ? 15.0 / 16.0 : 1.0;
    const double freq_shifted  = (freq_base * freq_mult) * (1 << YABSYS_TIMING_BITS);
    const double usec_shifted  = 1.0e6 * (1 << YABSYS_TIMING_BITS);
    const double line_time     = yabsys.IsPal ? 1.0 / 50 / 313 : 1.0 / (60 / 1.001) / 263;
    const double line_clk_cnt  = line_time * (freq_base * freq_mult);
    const double deciline_time = line_time / DECILINE_STEP;
    yabsys.DecilineCount       = 0;
    yabsys.LineCount           = 0;
    yabsys.CurSH2FreqType      = freqtype;
    for (int i = 0; i < DECILINE_STEP; i++) {
        yabsys.LineCycle[i] = (u32)(line_clk_cnt * (float)i / (float)(DECILINE_STEP - 1));
    }
    for (int i = DECILINE_STEP - 1; i > 0; i--) {
        yabsys.LineCycle[i] -= yabsys.LineCycle[i - 1];
    }
    yabsys.DecilineStop = (u32)(freq_shifted * deciline_time + 0.5);
    MSH2->cycleFrac     = 0;
    SSH2->cycleFrac     = 0;
    yabsys.DecilineUsec = (u32)(usec_shifted * deciline_time + 0.5);
    yabsys.UsecFrac     = 0;
}
extern int     tweak_backup_file_size;
extern YabSem *g_scsp_ready;
extern YabSem *g_cpu_ready;
static void    sh2ExecuteSync(SH2_struct *sh, int req)
{
    SH2Exec(sh, req);
}
int YabauseSh2Init(yabauseinit_struct *init)
{
    yabsys.UseThreads     = init->usethreads;
    yabsys.NumThreads     = init->numthreads;
    yabsys.usecache       = init->usecache;
    yabsys.vsyncon        = init->vsyncon;
    yabsys.wireframe_mode = init->wireframe_mode;
    yabsys.isRotated      = 0;
    nextFrameTime         = 0;
    if (SH2Init(init->sh2coretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("SH2"));
        return -1;
    }

    if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
        return -1;
    if ((HighWram = T2MemoryInit(0x100000)) == NULL)
        return -1;
    if ((LowWram = T2MemoryInit(0x100000)) == NULL)
        return -1;
    BackupInit(init->buppath, init->extend_backup);
    if (CartInit(init->cartpath, init->carttype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("Cartridge"));
        return -1;
    }
    if (STVSingleInit(init->stvgamepath, init->stvbiospath, init->eepromdir, init->stv_favorite_region) != 0) {
        if (STVInit(init->stvgame, init->cartpath, init->eepromdir, init->stv_favorite_region) != 0) {
            YabSetError(YAB_ERR_CANNOTINIT, _("STV emulation"));
            return -1;
        }
    }
    MappedMemoryInit();
    MSH2->cycleFrac = 0;
    MSH2->cycleLost = 0;
    MSH2->cdiff     = 0;
    SSH2->cycleFrac = 0;
    SSH2->cycleLost = 0;
    SSH2->cdiff     = 0;
    return 0;
}
static u64 fpsticks = 0;
#ifdef _USE_PERFETTO_TRACE_
std::unique_ptr<perfetto::TracingSession> myTracingSession;
void                                      InitializePerfetto()
{
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();
}
std::unique_ptr<perfetto::TracingSession> StartTracing()
{
    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024);
    auto *ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");
    auto tracing_session = perfetto::Tracing::NewTrace();
    tracing_session->Setup(cfg);
    tracing_session->StartBlocking();
    return tracing_session;
}
void StopTracing(std::unique_ptr<perfetto::TracingSession> tracing_session)
{
    perfetto::TrackEvent::Flush();
    tracing_session->StopBlocking();
    std::vector<char> trace_data(tracing_session->ReadTraceBlocking());
    std::ofstream     output;
    output.open("kronos.pftrace", std::ios::out | std::ios::binary);
    output.write(&trace_data[0], std::streamsize(trace_data.size()));
    output.close();
    PERFETTO_LOG("Trace written in kronos.pftrace file. To read this trace in "
                 "text form, run `./tools/traceconv text kronos.pftrace`");
}
#endif
int YabauseInit(yabauseinit_struct *init)
{
#ifdef _USE_PERFETTO_TRACE_
    InitializePerfetto();
    myTracingSession = StartTracing();
#endif
    yabsys.UseThreads     = init->usethreads;
    yabsys.NumThreads     = init->numthreads;
    yabsys.usecache       = init->usecache;
    yabsys.vsyncon        = init->vsyncon;
    yabsys.wireframe_mode = init->wireframe_mode;
    yabsys.skipframe      = init->skipframe;
    yabsys.isRotated      = 0;
    nextFrameTime         = 0;
    if (SH2Init(init->sh2coretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("SH2"));
        return -1;
    }
    if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
        return -1;
    if ((HighWram = T2MemoryInit(0x100000)) == NULL)
        return -1;
    if ((LowWram = T2MemoryInit(0x100000)) == NULL)
        return -1;
    if (BackupInit(init->buppath, init->extend_backup) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("Backup Ram"));
        return -1;
    }
    if (Cs2Init(init->cdcoretype, init->cdpath, init->mpegpath) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("CS2"));
        return -1;
    }
    if (CartInit(init->cartpath, init->carttype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("Cartridge"));
        return -1;
    }
    if (STVSingleInit(init->stvgamepath, init->stvbiospath, init->eepromdir, init->stv_favorite_region) != 0) {
        if (STVInit(init->stvgame, init->cartpath, init->eepromdir, init->stv_favorite_region) != 0) {
            YabSetError(YAB_ERR_CANNOTINIT, _("STV emulation"));
            return -1;
        }
    }
    MappedMemoryInit();
    MSH2->cycleFrac = 0;
    MSH2->cycleLost = 0;
    MSH2->cdiff     = 0;
    SSH2->cycleFrac = 0;
    SSH2->cycleLost = 0;
    SSH2->cdiff     = 0;
    if (VideoInit(init->vidcoretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("Video"));
        return -1;
    }

    if (PerInit(init->percoretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("Peripheral"));
        return -1;
    }
    if (ScuInit() != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("SCU"));
        return -1;
    }
    if (M68KInit(init->m68kcoretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("M68K"));
        return -1;
    }
    if (ScspInit(init->sndcoretype) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("SCSP/M68K"));
        return -1;
    }
    if (Vdp1Init() != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("VDP1"));
        return -1;
    }
    if (Vdp2Init() != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("VDP2"));
        return -1;
    }
    if (SmpcInit(init->regionid, init->clocksync, init->basetime, init->smpcpath, init->languageid) != 0) {
        YabSetError(YAB_ERR_CANNOTINIT, _("SMPC"));
        return -1;
    }
    YabauseChangeTiming(CLKTYPE_26MHZ);
    if (init->vsyncon == 0)
        EnableAutoFrameSkip();

    yabsys.emulatebios  = 1;
    yabsys.usequickload = 0;
    YabauseResetNoLoad();
    if (init->skip_load) {
        return 0;
    }

    if (yabsys.usequickload || yabsys.emulatebios) {
        if (YabauseQuickLoadGame() != 0) {
            if (yabsys.emulatebios) {
                YabSetError(YAB_ERR_CANNOTINIT, _("Game"));
                return -2;
            } else
                YabauseResetNoLoad();
        }
    }
    if (Cs2GetRegionID() >= 0xA)
        YabauseSetVideoFormat(VIDEOFORMATTYPE_PAL);
    else
        YabauseSetVideoFormat(VIDEOFORMATTYPE_NTSC);
#ifdef HAVE_GDBSTUB
    GdbStubInit(MSH2, 43434);
#endif
    fpsticks = YabauseGetTicks();
    return 0;
}
void YabFlushBackups(void)
{
    BackupFlush();
    CartFlush();
}
void YabauseDeInit(void)
{
    STVDeInit();
    Vdp2DeInit();
    Vdp1DeInit();
    SH2DeInit();
    if (BiosRom)
        T2MemoryDeInit(BiosRom);
    BiosRom = NULL;
    if (HighWram)
        T2MemoryDeInit(HighWram);
    HighWram = NULL;
    if (LowWram)
        T2MemoryDeInit(LowWram);
    LowWram = NULL;
    BackupDeinit();
    CartDeInit();
    Cs2DeInit();
    ScuDeInit();
    ScspDeInit();
    SmpcDeInit();
    PerDeInit();
    VideoDeInit();
#ifdef _USE_PERFETTO_TRACE_
    StopTracing(std::move(myTracingSession));
#endif
}
void YabauseResetNoLoad(void)
{
    SH2Reset(MSH2);
    YabauseStopSlave();
    memset(HighWram, 0, 0x100000);
    memset(LowWram, 0, 0x100000);
    Cs2Reset();
    ScuReset();
    ScspReset();
    Vdp1Reset();
    Vdp2Reset();
    SmpcReset();
    nextFrameTime = 0;
    SH2PowerOn(MSH2);
}
void YabauseReset(void)
{
    YabauseResetNoLoad();
    if (yabsys.usequickload || yabsys.emulatebios) {
        if (YabauseQuickLoadGame() != 0) {
            if (yabsys.emulatebios)
                YabSetError(YAB_ERR_CANNOTINIT, _("Game"));
            else
                YabauseResetNoLoad();
        }
    }
}
void YabauseResetButton(void)
{
    SmpcResetButton();
}
int YabauseExec(void)
{
#if 0
	if (FrameAdvanceVariable > 0 && LagFrameFlag == 1){
		FrameAdvanceVariable = NeedAdvance;
		YabauseEmulate();
		FrameAdvanceVariable = Paused;
		return(0);
	}

	if (FrameAdvanceVariable == Paused){
		ScspMuteAudio(SCSP_MUTE_SYSTEM);
		return(0);
	}

	if (FrameAdvanceVariable == NeedAdvance){
		FrameAdvanceVariable = Paused;
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
		YabauseEmulate();
	}

	if (FrameAdvanceVariable == RunNormal ) {
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
		YabauseEmulate();
	}
#else
    ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
    YabauseEmulate();
#endif
    return 0;
}
int saved_centicycles;
u32 get_cycles_per_line_division(u32 clock, int frames, int lines, int divisions_per_line)
{
    return ((u64)(clock / frames) << SCSP_FRACTIONAL_BITS) / (lines * divisions_per_line);
}
u32 YabauseGetCpuTime()
{
    return MSH2->cycles;
}
#define HBLANK_IN_STEP ((DECILINE_STEP * 8) / 10)
static int  fpsframecount     = 0;
static int  vdp1fpsframecount = 0;
static int  fps               = 0;
static int  vdp1fps           = 0;
static void FPSDisplay(void)
{
    fpsframecount++;
    u64 now = YabauseGetTicks();
    if (now >= fpsticks + yabsys.tickfreq) {
        u64 delta         = now - (fpsticks + yabsys.tickfreq);
        fps               = fpsframecount;
        vdp1fps           = vdp1fpsframecount;
        fpsframecount     = 0;
        vdp1fpsframecount = 0;
        fpsticks          = YabauseGetTicks() - delta;
    }
}
void addVdp1Framecount()
{
    vdp1fpsframecount++;
}
void dropFrameDisplay()
{
    fpsframecount--;
    if (fpsframecount < -1)
        fpsframecount = -1;
}
u32 YabauseGetFrameCount()
{
    return yabsys.frame_count;
}
void SyncCPUtoSCSP();
u64  g_m68K_dec_cycle = 0;
int  YabauseEmulate(void)
{
    int ret          = 0;
    int oneframeexec = 0;
    yabsys.frame_count++;
    const u32    usecinc = yabsys.DecilineUsec;
    unsigned int m68kcycles;
    unsigned int m68kcenticycles;
    u64          m68k_cycles_per_deciline = 0;
    u64          scsp_cycles_per_deciline = 0;
    int          lines                    = 0;
    int          frames                   = 0;
    if (yabsys.IsPal) {
        lines  = 313;
        frames = 50;
    } else {
        lines  = 263;
        frames = 60;
    }
    scsp_cycles_per_deciline = get_cycles_per_line_division(44100 * 512, frames, lines, DECILINE_STEP);
    m68k_cycles_per_deciline = get_cycles_per_line_division(44100 * 256, frames, lines, DECILINE_STEP);
    DoMovie();
    MSH2->cycles    = 0;
    MSH2->frtcycles = 0;
    MSH2->depth     = 0;
    SSH2->depth     = 0;
    SSH2->cycles    = 0;
    SSH2->frtcycles = 0;
    u64 cpu_emutime = 0;
    while (!oneframeexec) {
        VIDCore->setupFrame(0);
#ifdef YAB_STATICS
        u64 current_cpu_clock = YabauseGetTicks();
#endif
        sh2ExecuteSync(MSH2, yabsys.LineCycle[yabsys.DecilineCount]);
        if (yabsys.IsSSH2Running) {
            sh2ExecuteSync(SSH2, yabsys.LineCycle[yabsys.DecilineCount]);
        }
#ifdef YAB_STATICS
        cpu_emutime += (YabauseGetTicks() - current_cpu_clock) * 1000000 / yabsys.tickfreq;
#endif
        yabsys.DecilineCount++;
        if (yabsys.DecilineCount == HBLANK_IN_STEP) {
            Vdp1HBlankIN();
            Vdp2HBlankIN();
        }
        if (yabsys.DecilineCount == DECILINE_STEP) {
            Vdp2HBlankOUT();
            Vdp1HBlankOUT();
            yabsys.DecilineCount = 0;
            yabsys.LineCount++;
            if (yabsys.LineCount == yabsys.VBlankLineCount) {
                ScspAddCycles((u64)(44100 * 256 / frames) << SCSP_FRACTIONAL_BITS);
                SmpcINTBACKEnd();
                Vdp1VBlankIN();
                Vdp2VBlankIN();
                SyncCPUtoSCSP();
            } else if (yabsys.LineCount == yabsys.MaxLineCount) {
                Vdp1VBlankOUT();
                Vdp2VBlankOUT();
                yabsys.LineCount = 0;
                oneframeexec     = 1;
            }
        }
        ScuExec((yabsys.DecilineStop >> YABSYS_TIMING_BITS) / 2);
        yabsys.UsecFrac += usecinc;
        SmpcExec(yabsys.UsecFrac >> YABSYS_TIMING_BITS);
        Cs2Exec(yabsys.UsecFrac >> YABSYS_TIMING_BITS);
        yabsys.UsecFrac &= YABSYS_TIMING_MASK;
        saved_m68k_cycles += m68k_cycles_per_deciline;
    }
    syncVideoMode();
    FPSDisplay();
#ifdef YAB_STATICS
    YuiMsg("CPUTIME = %" PRId64 " @ %d \n", cpu_emutime, yabsys.frame_count);
#if 1
    if (yabsys.frame_count >= 4000) {
        static FILE *pfm = NULL;
        if (pfm == NULL) {
#ifdef ANDROID
            pfm = fopen("/mnt/sdcard/cpu.txt", "w");
#else
            pfm = fopen("cpu.txt", "w");
#endif
        }
        if (pfm) {
            fprintf(pfm, "%d\t%" PRId64 "\n", yabsys.frame_count, cpu_emutime);
            fflush(pfm);
        }
        if (yabsys.frame_count >= 6100) {
            fclose(pfm);
            exit(0);
        }
    }
#endif
#endif
#if DYNAREC_DEVMIYAX
    if (SH2Core->id == 3)
        SH2DynShowSttaics(MSH2, SSH2);
#endif
    return ret;
}
void SyncCPUtoSCSP()
{
    YabSemWait(g_scsp_ready);
    YabThreadWake(YAB_THREAD_SCSP);
    YabSemPost(g_cpu_ready);
    saved_m68k_cycles = 0;
}
void YabauseStartSlave(void)
{
    if (yabsys.IsSSH2Running == 1)
        return;
    if (yabsys.emulatebios) {
        MappedMemoryWriteLong(SSH2, 0xFFFFFFE0, 0xA55A03F1);
        MappedMemoryWriteLong(SSH2, 0xFFFFFFE4, 0xA55A00FC);
        MappedMemoryWriteLong(SSH2, 0xFFFFFFE8, 0xA55A5555);
        MappedMemoryWriteLong(SSH2, 0xFFFFFFEC, 0xA55A0070);
        MappedMemoryWriteWord(SSH2, 0xFFFFFEE0, 0x0000);
        MappedMemoryWriteWord(SSH2, 0xFFFFFEE2, 0x0000);
        MappedMemoryWriteWord(SSH2, 0xFFFFFE60, 0x0F00);
        MappedMemoryWriteWord(SSH2, 0xFFFFFE62, 0x6061);
        MappedMemoryWriteWord(SSH2, 0xFFFFFE64, 0x6263);
        MappedMemoryWriteWord(SSH2, 0xFFFFFE66, 0x6465);
        MappedMemoryWriteWord(SSH2, 0xFFFFFE68, 0x6600);
        MappedMemoryWriteWord(SSH2, 0xFFFFFEE4, 0x6869);
        MappedMemoryWriteLong(SSH2, 0xFFFFFFA8, 0x0000006C);
        MappedMemoryWriteLong(SSH2, 0xFFFFFFA0, 0x0000006D);
        MappedMemoryWriteLong(SSH2, 0xFFFFFF0C, 0x0000006E);
        MappedMemoryWriteLong(SSH2, 0xFFFFFE10, 0x00000081);
        SH2GetRegisters(SSH2, &SSH2->regs);
        SSH2->regs.R[15] = Cs2GetSlaveStackAdress();
        SSH2->regs.VBR   = 0x06000400;
        SSH2->regs.PC    = SH2MappedMemoryReadLong(SSH2, 0x06000250);
        if (SH2MappedMemoryReadLong(SSH2, 0x060002AC) != 0)
            SSH2->regs.R[15] = SH2MappedMemoryReadLong(SSH2, 0x060002AC);
        SSH2->regs.SR.part.I = 0;
        SH2SetRegisters(SSH2, &SSH2->regs);
    } else {
        SH2PowerOn(SSH2);
        SH2GetRegisters(SSH2, &SSH2->regs);
        SSH2->regs.PC = 0x20000200;
        SH2SetRegisters(SSH2, &SSH2->regs);
    }
    yabsys.IsSSH2Running = 1;
}
void YabauseStopSlave(void)
{
    if (yabsys.IsSSH2Running == 0)
        return;
    SH2Reset(SSH2);
    yabsys.IsSSH2Running = 0;
}
u64 YabauseGetTicks(void)
{
#ifdef WIN32
    u64 ticks;
    QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
    return ticks;
#elif defined(_arch_dreamcast)
    return (u64)timer_ms_gettime64();
#elif defined(GEKKO)
    return gettime();
#elif defined(PSP)
    return sceKernelGetSystemTimeWide();
#elif defined(ANDROID)
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (u64)tv.tv_sec * 1000000 + tv.tv_usec;
#elif defined(HAVE_LIBSDL)
    return (u64)SDL_GetTicks();
#endif
}
void YabauseSetVideoFormat(int type)
{
    if (Vdp2Regs == NULL)
        return;
    yabsys.IsPal        = (type == VIDEOFORMATTYPE_PAL);
    yabsys.MaxLineCount = type ? 313 : 263;
#ifdef WIN32
    QueryPerformanceFrequency((LARGE_INTEGER *)&yabsys.tickfreq);
#elif defined(_arch_dreamcast)
    yabsys.tickfreq = 1000;
#elif defined(GEKKO)
    yabsys.tickfreq = secs_to_ticks(1);
#elif defined(PSP)
    yabsys.tickfreq = 1000000;
#elif defined(ANDROID)
    yabsys.tickfreq = 1000000;
#elif defined(HAVE_GETTIMEOFDAY)
    yabsys.tickfreq = 1000000;
#elif defined(HAVE_LIBSDL)
    yabsys.tickfreq = 1000;
#endif
    yabsys.OneFrameTime = type ? (yabsys.tickfreq / 50) : (yabsys.tickfreq * 1001 / 60000);
    Vdp2Regs->TVSTAT    = Vdp2Regs->TVSTAT | (type & 0x1);
    ScspChangeVideoFormat(type);
    YabauseChangeTiming(yabsys.CurSH2FreqType);
}
void YabauseSetSkipframe(int skipframe)
{
    yabsys.skipframe = skipframe;
}
void YabauseSpeedySetup(void)
{
    u32 data;
    int i;
    if (yabsys.emulatebios)
        BiosInit(MSH2);
    else {
        for (i = 0; i < 0x210; i += 4) {
            data = MappedMemoryReadLong(MSH2, 0x00000600 + i);
            MappedMemoryWriteLong(MSH2, 0x06000000 + i, data);
        }
        for (i = 0; i < 0x8E0; i += 4) {
            data = MappedMemoryReadLong(MSH2, 0x00000820 + i);
            MappedMemoryWriteLong(MSH2, 0x06000220 + i, data);
        }
        for (i = 0; i < 0x700; i += 4) {
            data = MappedMemoryReadLong(MSH2, 0x00001100 + i);
            MappedMemoryWriteLong(MSH2, 0x06001100 + i, data);
        }
        MappedMemoryWriteLong(MSH2, 0x06000234, 0x000002AC);
        MappedMemoryWriteLong(MSH2, 0x06000238, 0x000002BC);
        MappedMemoryWriteLong(MSH2, 0x0600023C, 0x00000350);
        MappedMemoryWriteLong(MSH2, 0x06000240, 0x32524459);
        MappedMemoryWriteLong(MSH2, 0x0600024C, 0x00000000);
        MappedMemoryWriteLong(MSH2, 0x06000268, MappedMemoryReadLong(MSH2, 0x00001344));
        MappedMemoryWriteLong(MSH2, 0x0600026C, MappedMemoryReadLong(MSH2, 0x00001348));
        MappedMemoryWriteLong(MSH2, 0x0600029C, MappedMemoryReadLong(MSH2, 0x00001354));
        MappedMemoryWriteLong(MSH2, 0x060002C4, MappedMemoryReadLong(MSH2, 0x00001104));
        MappedMemoryWriteLong(MSH2, 0x060002C8, MappedMemoryReadLong(MSH2, 0x00001108));
        MappedMemoryWriteLong(MSH2, 0x060002CC, MappedMemoryReadLong(MSH2, 0x0000110C));
        MappedMemoryWriteLong(MSH2, 0x060002D0, MappedMemoryReadLong(MSH2, 0x00001110));
        MappedMemoryWriteLong(MSH2, 0x060002D4, MappedMemoryReadLong(MSH2, 0x00001114));
        MappedMemoryWriteLong(MSH2, 0x060002D8, MappedMemoryReadLong(MSH2, 0x00001118));
        MappedMemoryWriteLong(MSH2, 0x060002DC, MappedMemoryReadLong(MSH2, 0x0000111C));
        MappedMemoryWriteLong(MSH2, 0x06000328, 0x000004C8);
        MappedMemoryWriteLong(MSH2, 0x0600032C, 0x00001800);
        for (i = 0; i < 0x80; i += 4)
            MappedMemoryWriteLong(MSH2, 0x06000A00 + i, 0x0600083C);
    }
    Cs2Area->reg.HIRQ      = 0xFC1;
    Cs2Area->isdiskchanged = 0;
    Cs2Area->reg.CR1       = (Cs2Area->status << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
    Cs2Area->reg.CR2       = (Cs2Area->ctrladdr << 8) | Cs2Area->track;
    Cs2Area->reg.CR3       = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) & 0xFF);
    Cs2Area->reg.CR4       = (u16)Cs2Area->FAD;
    Cs2Area->satauth       = 4;
    SH2GetRegisters(MSH2, &MSH2->regs);
    for (i = 0; i < 15; i++)
        MSH2->regs.R[i] = 0x00000000;
    MSH2->regs.R[15]  = 0x06002000;
    MSH2->regs.SR.all = 0x00000000;
    MSH2->regs.GBR    = 0x00000000;
    MSH2->regs.VBR    = 0x06000000;
    MSH2->regs.MACH   = 0x00000000;
    MSH2->regs.MACL   = 0x00000000;
    MSH2->regs.PR     = 0x00000000;
    SH2SetRegisters(MSH2, &MSH2->regs);
    ScuRegs->D1AD = ScuRegs->D2AD = 0;
    ScuRegs->D0EN                 = 0x101;
    ScuRegs->IST                  = 0x2006;
    ScuRegs->ITEdge               = 0x2006;
    ScuRegs->AIACK                = 0x1;
    ScuRegs->ASR0 = ScuRegs->ASR1 = 0x1FF01FF0;
    ScuRegs->AREF                 = 0x1F;
    ScuRegs->RSEL                 = 0x1;
    SmpcRegs->COMREG              = 0x10;
    SmpcInternalVars->resd        = 0;
    Vdp1Regs->EDSR                = 3;
    Vdp1Regs->localX              = 160;
    Vdp1Regs->localY              = 112;
    Vdp1Regs->systemclipX2        = 319;
    Vdp1Regs->systemclipY2        = 223;
    memset(Vdp2Regs, 0, sizeof(Vdp2));
    Vdp2Regs->TVMD      = 0x8000;
    Vdp2Regs->TVSTAT    = 0x020A;
    Vdp2Regs->CYCA0L    = 0x0F44;
    Vdp2Regs->CYCA0U    = 0xFFFF;
    Vdp2Regs->CYCA1L    = 0xFFFF;
    Vdp2Regs->CYCA1U    = 0xFFFF;
    Vdp2Regs->CYCB0L    = 0xFFFF;
    Vdp2Regs->CYCB0U    = 0xFFFF;
    Vdp2Regs->CYCB1L    = 0xFFFF;
    Vdp2Regs->CYCB1U    = 0xFFFF;
    Vdp2Regs->BGON      = 0x0001;
    Vdp2Regs->PNCN0     = 0x8000;
    Vdp2Regs->MPABN0    = 0x0303;
    Vdp2Regs->MPCDN0    = 0x0303;
    Vdp2Regs->ZMXN0.all = 0x00010000;
    Vdp2Regs->ZMYN0.all = 0x00010000;
    Vdp2Regs->ZMXN1.all = 0x00010000;
    Vdp2Regs->ZMYN1.all = 0x00010000;
    Vdp2Regs->BKTAL     = 0x4000;
    Vdp2Regs->SPCTL     = 0x0020;
    Vdp2Regs->PRINA     = 0x0007;
    Vdp2Regs->CLOFEN    = 0x0001;
    Vdp2Regs->COAR      = 0x0200;
    Vdp2Regs->COAG      = 0x0200;
    Vdp2Regs->COAB      = 0x0200;
}
int YabauseQuickLoadGame(void)
{
    partition_struct *lgpartition;
    u8               *buffer;
    u32               addr;
    u32               size;
    u32               blocks;
    unsigned int      i, i2;
    dirrec_struct     dirrec;
    Cs2Area->outconcddev    = Cs2Area->filter + 0;
    Cs2Area->outconcddevnum = 0;
    Cs2Area->cdi->ReadTOC(Cs2Area->TOC);
    if ((lgpartition = Cs2ReadUnFilteredSector(150)) == NULL)
        return -1;
    buffer = lgpartition->block[lgpartition->numblocks - 1]->data;
    YabauseSpeedySetup();
    if (memcmp(buffer, "SEGA SEGASATURN", 15) == 0) {
        size   = (buffer[0xE0] << 24) | (buffer[0xE1] << 16) | (buffer[0xE2] << 8) | buffer[0xE3];
        blocks = size >> 11;
        if ((size % 2048) != 0)
            blocks++;
        size              = 16 * 2048;
        blocks            = 16;
        addr              = (buffer[0xF0] << 24) | (buffer[0xF1] << 16) | (buffer[0xF2] << 8) | buffer[0xF3];
        lgpartition->size = 0;
        Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
        lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
        lgpartition->numblocks                            = 0;
        for (i = 0; i < blocks; i++) {
            if ((lgpartition = Cs2ReadUnFilteredSector(150 + i)) == NULL)
                return -1;
            buffer = lgpartition->block[lgpartition->numblocks - 1]->data;
            if (size >= 2048) {
                for (i2 = 0; i2 < 2048; i2++)
                    MappedMemoryWriteByte(MSH2, 0x06002000 + (i * 0x800) + i2, buffer[i2]);
            } else {
                for (i2 = 0; i2 < size; i2++)
                    MappedMemoryWriteByte(MSH2, 0x06002000 + (i * 0x800) + i2, buffer[i2]);
            }
            size -= 2048;
            lgpartition->size = 0;
            Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
            lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
            lgpartition->numblocks                            = 0;
        }
        SH2WriteNotify(MSH2, 0x6002000, blocks << 11);
        if ((lgpartition = Cs2ReadUnFilteredSector(166)) == NULL)
            return -1;
        Cs2CopyDirRecord(lgpartition->block[lgpartition->numblocks - 1]->data + 0x9C, &dirrec);
        lgpartition->size = 0;
        Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
        lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
        lgpartition->numblocks                            = 0;
        if ((lgpartition = Cs2ReadUnFilteredSector(dirrec.lba + 150)) == NULL)
            return -1;
        buffer = lgpartition->block[lgpartition->numblocks - 1]->data;
        for (i = 0; i < 3; i++) {
            Cs2CopyDirRecord(buffer, &dirrec);
            buffer += dirrec.recordsize;
        }
        size   = dirrec.size;
        blocks = size >> 11;
        if ((dirrec.size % 2048) != 0)
            blocks++;
        lgpartition->size = 0;
        Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
        lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
        lgpartition->numblocks                            = 0;
        for (i = 0; i < blocks; i++) {
            if ((lgpartition = Cs2ReadUnFilteredSector(150 + dirrec.lba + i)) == NULL)
                return -1;
            buffer = lgpartition->block[lgpartition->numblocks - 1]->data;
            if (size >= 2048) {
                for (i2 = 0; i2 < 2048; i2++)
                    MappedMemoryWriteByte(MSH2, addr + (i * 0x800) + i2, buffer[i2]);
            } else {
                for (i2 = 0; i2 < size; i2++)
                    MappedMemoryWriteByte(MSH2, addr + (i * 0x800) + i2, buffer[i2]);
            }
            size -= 2048;
            lgpartition->size = 0;
            Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
            lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
            lgpartition->numblocks                            = 0;
        }
        SH2WriteNotify(MSH2, addr, blocks << 11);
        SH2GetRegisters(MSH2, &MSH2->regs);
        MSH2->onchip.VCRC   = 0x64 << 8;
        MSH2->onchip.VCRWDT = 0x6869;
        MSH2->onchip.IPRB   = 0x0F00;
        MSH2->regs.PC       = Cs2GetMasterExecutionAdress();
        MSH2->regs.R[15]    = Cs2GetMasterStackAdress();
        SH2SetRegisters(MSH2, &MSH2->regs);
        Cs2InitializeCDSystem();
        Cs2Area->reg.CR1 = 0x48fc;
        Cs2Area->reg.CR2 = 0x0;
        Cs2Area->reg.CR3 = 0x0;
        Cs2Area->reg.CR4 = 0x0;
        Cs2ResetSelector();
        Cs2GetToc();
        Vdp2WriteWord(MSH2, NULL, 0x0E, 0);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x0, 0x8000);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x2, 0x9908);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x4, 0xCA94);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x6, 0xF39C);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x8, 0xFBDE);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xA, 0xFB16);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xC, 0x9084);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xE, 0xF20C);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x10, 0xF106);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x12, 0xF18A);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x14, 0xB9CE);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x16, 0xA14A);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x18, 0xE318);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1A, 0xEB5A);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1C, 0xF39C);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1E, 0xFBDE);
        Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xFF, 0x0000);
        for (int i = 0; i < 0x80000; i += 0x20) {
            Vdp1RamWriteWord(NULL, Vdp1Ram, i, 0x8000);
        }
    } else {
        lgpartition->size = 0;
        Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
        lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
        lgpartition->numblocks                            = 0;
        return -1;
    }
    return 0;
}
void EnableAutoFrameSkip(void)
{
    autoframeskipenab = 1;
    nextFrameTime     = 0;
    YabauseSetVideoFormat((yabsys.IsPal == 1) ? VIDEOFORMATTYPE_PAL : VIDEOFORMATTYPE_NTSC);
}
int isAutoFrameSkip(void)
{
    return autoframeskipenab;
}
void DisableAutoFrameSkip(void)
{
    autoframeskipenab = 0;
    nextFrameTime     = 0;
    YabauseSetVideoFormat((yabsys.IsPal == 1) ? VIDEOFORMATTYPE_PAL : VIDEOFORMATTYPE_NTSC);
}
