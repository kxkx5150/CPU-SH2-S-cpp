

#include "scsp.h"

static int  SNDDummyInit(void);
static void SNDDummyDeInit(void);
static int  SNDDummyReset(void);
static int  SNDDummyChangeVideoFormat(int vertfreq);
static void SNDDummyUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
static u32  SNDDummyGetAudioSpace(void);
static void SNDDummyMuteAudio(void);
static void SNDDummyUnMuteAudio(void);
static void SNDDummySetVolume(int volume);
#ifdef USE_SCSPMIDI
int SNDDummyMidiChangePorts(int inport, int outport);
u8  SNDDummyMidiIn(int *isdata);
int SNDDummyMidiOut(u8 data);
#endif

SoundInterface_struct SNDDummy = {SNDCORE_DUMMY,
                                  "Dummy Sound Interface",
                                  SNDDummyInit,
                                  SNDDummyDeInit,
                                  SNDDummyReset,
                                  SNDDummyChangeVideoFormat,
                                  SNDDummyUpdateAudio,
                                  SNDDummyGetAudioSpace,
                                  SNDDummyMuteAudio,
                                  SNDDummyUnMuteAudio,
                                  SNDDummySetVolume,
#ifdef USE_SCSPMIDI
                                  SNDDummyMidiChangePorts,
                                  SNDDummyMidiIn,
                                  SNDDummyMidiOut
#endif
};

static int SNDDummyInit(void)
{
    return 0;
}

static void SNDDummyDeInit(void)
{
}

static int SNDDummyReset(void)
{
    return 0;
}

static int SNDDummyChangeVideoFormat(UNUSED int vertfreq)
{
    return 0;
}

static void SNDDummyUpdateAudio(UNUSED u32 *leftchanbuffer, UNUSED u32 *rightchanbuffer, UNUSED u32 num_samples)
{
}

static u32 SNDDummyGetAudioSpace(void)
{
    static int i = 0;
    i++;
    if (i == 55) {
        i = 0;
        return 85;
    } else {
        return 0;
    }
}

void SNDDummyMuteAudio()
{
}

void SNDDummyUnMuteAudio()
{
}

void SNDDummySetVolume(UNUSED int volume)
{
}

#ifdef USE_SCSPMIDI
int SNDDummyMidiChangePorts(int inport, int outport)
{
    return 0;
}

u8 SNDDummyMidiIn(int *isdata)
{
    *isdata = 0;
    return 0;
}

int SNDDummyMidiOut(u8 data)
{
    return 1;
}
#endif
