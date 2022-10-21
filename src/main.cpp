#include <cstdlib>
#define PLATFORM_LINUX
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keycode.h>

#include "sh2core.h"
#include "sh2int.h"
#include "yui.h"
#include "peripheral.h"
#include "yabause.h"
#ifdef HAVE_LIBGL
#include "vidogl.h"
#include "ygl.h"
#endif
#include "cdbase.h"
#include "cs0.h"
#include "cs2.h"
#include "scsp.h"
#ifdef ARCH_IS_LINUX
#include "perlinuxjoy.h"
#endif
#include "cdbase.h"
#include "m68kcore.h"
#include "vdp2.h"
#include "debug.h"
#include "vdp1.h"
#ifdef DYNAREC_KRONOS
#include "sh2int.h"
#endif
#include <stdio.h>
#include "sh2core.h"

#define AR                (4.0f / 3.0f)
#define WINDOW_WIDTH      600
#define WINDOW_HEIGHT     ((int)((float)WINDOW_WIDTH / AR))
#define WINDOW_WIDTH_LOW  300
#define WINDOW_HEIGHT_LOW ((int)((float)WINDOW_WIDTH_LOW / AR))
#define SCALE             1
#define OSDCORE_DUMMY     0
#define OSDCORE_SOFT      2
#define OSDCORE_NANOVG    3
#define OSDCORE_DEFAULT   OSDCORE_NANOVG

typedef void (*k_callback)(unsigned int key, unsigned char state);

M68K_struct           *M68KCoreList[] = {&M68KDummy, &M68KMusashi, NULL};
SH2Interface_struct   *SH2CoreList[]  = {&SH2Interpreter, &SH2Interpreter, &SH2Interpreter, NULL};
PerInterface_struct   *PERCoreList[]  = {&PERDummy, &PERLinuxJoy, NULL};
CDInterface           *CDCoreList[]   = {&DummyCD, &ISOCD, &ArchCD, NULL};
SoundInterface_struct *SNDCoreList[]  = {&SNDDummy, NULL};
VideoInterface_struct *VIDCoreList[]  = {&VIDOGL, NULL, NULL};
yabauseinit_struct     yinit;

SDL_Window   *m_context   = nullptr;
SDL_GLContext m_glcontext = nullptr;
SDL_Renderer *renderer    = nullptr;
bool          Running     = true;

static int           Wwidth;
static int           Wheight;
static int           fullscreen       = 0;
static int           scanline         = 0;
static int           lowres_mode      = 0;
static char          biospath[256]    = "\0";
static char          cdpath[256]      = "\0";
static char          stvgamepath[256] = "\0";
static char          stvbiospath[256] = "\0";
static k_callback    k_call           = NULL;
static unsigned char inputMap[512];

int platform_YuiRevokeOGLOnThisThread()
{
#if defined(YAB_ASYNC_RENDERING)
    // glfwMakeContextCurrent(g_offscreen_context);
#endif
    return 0;
}
int platform_YuiUseOGLOnThisThread()
{
#if defined(YAB_ASYNC_RENDERING)
    // glfwMakeContextCurrent(g_window);
#endif
    return 0;
}
void platform_swapBuffers(void)
{
    if (!m_context)
        return;
    SDL_GL_SwapWindow(m_context);
}
void platform_getFBSize(int *w, int *h)
{
    // glfwGetFramebufferSize(g_window, w, h);
}
int platform_shouldClose()
{
    return Running;
}
void platform_Close()
{
    Running = 0;
    // glfwSetWindowShouldClose(g_window, GL_TRUE);
}
void platform_HandleEvent(void)
{
    // glfwPollEvents();
}
void platform_SetKeyCallback(k_callback call)
{
    k_call = call;
}
void YuiMsg(const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    vprintf(format, arglist);
    va_end(arglist);
}
void YuiErrorMsg(const char *error_text)
{
    YuiMsg("\n\nError: %s\n", error_text);
}
int YuiRevokeOGLOnThisThread()
{
    platform_YuiRevokeOGLOnThisThread();
    return 0;
}
int YuiUseOGLOnThisThread()
{
    platform_YuiUseOGLOnThisThread();
    return 0;
}
void YuiSwapBuffers(void)
{
    if (!m_context)
        return;
    SDL_GL_SwapWindow(m_context);
}
int YuiGetFB(void)
{
    return 0;
}
void YuiInit()
{
    yinit.m68kcoretype  = M68KCORE_MUSASHI;
    yinit.percoretype   = PERCORE_LINUXJOY;
    yinit.sh2coretype   = 8;
    yinit.vidcoretype   = VIDCORE_OGL;    // VIDCORE_SOFT
    yinit.sndcoretype   = 0;
    yinit.cdcoretype    = CDCORE_DEFAULT;
    yinit.carttype      = CART_DRAM32MBIT;
    yinit.regionid      = REGION_EUROPE;
    yinit.languageid    = 0;
    yinit.biospath      = NULL;
    yinit.cdpath        = NULL;
    yinit.buppath       = "";    //"./backup.ram";
    yinit.extend_backup = 1;
    yinit.mpegpath      = NULL;
    yinit.cartpath      = "";    //"./backup32Mb.ram";
    yinit.osdcoretype   = OSDCORE_DEFAULT;
    yinit.skip_load     = 0;
    yinit.usethreads    = 1;
    yinit.numthreads    = 4;
    yinit.usecache      = 0;
}
int platform_SetupSDL2(int w, int h, int fullscreen)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    m_context = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w * SCALE, h * SCALE,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    renderer  = SDL_CreateRenderer(m_context, -1, 0);
    SDL_RenderSetScale(renderer, SCALE, SCALE);
    SDL_SetRenderTarget(renderer, NULL);
    m_glcontext = SDL_GL_CreateContext(m_context);
    glewInit();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    // SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetSwapInterval(0);
    int maj, min;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);
    printf("OpenGL version is %d.%d (%s, %s)\n", maj, min, glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    return 1;
}
void keyPressEvent(SDL_Keycode sdlkey)
{
    if (k_call) {
        switch (sdlkey) {
            case SDLK_RETURN:
                k_call(PERPAD_START, 1);
                break;

            case SDLK_UP:
                k_call(PERPAD_UP, 1);
                break;
            case SDLK_DOWN:
                k_call(PERPAD_DOWN, 1);
                break;
            case SDLK_LEFT:
                k_call(PERPAD_LEFT, 1);
                break;
            case SDLK_RIGHT:
                k_call(PERPAD_RIGHT, 1);
                break;
            case SDLK_a:
                k_call(PERPAD_A, 1);
                break;
            case SDLK_s:
                k_call(PERPAD_B, 1);
                break;
            case SDLK_z:
                k_call(PERPAD_C, 1);
                break;
            case SDLK_x:
                k_call(PERPAD_X, 1);
                break;
            case SDLK_q:
                k_call(PERPAD_Y, 1);
                break;
            case SDLK_w:
                k_call(PERPAD_Z, 1);
                break;
        }
    }
}
void keyReleaseEvent(SDL_Keycode sdlkey)
{
    if (k_call) {
        switch (sdlkey) {
            case SDLK_RETURN:
                k_call(PERPAD_START, 0);
                break;
            case SDLK_UP:
                k_call(PERPAD_UP, 0);
                break;
            case SDLK_DOWN:
                k_call(PERPAD_DOWN, 0);
                break;
            case SDLK_LEFT:
                k_call(PERPAD_LEFT, 0);
                break;
            case SDLK_RIGHT:
                k_call(PERPAD_RIGHT, 0);
                break;
            case SDLK_a:
                k_call(PERPAD_A, 0);
                break;
            case SDLK_s:
                k_call(PERPAD_B, 0);
                break;
            case SDLK_z:
                k_call(PERPAD_C, 0);
                break;
            case SDLK_x:
                k_call(PERPAD_X, 0);
                break;
            case SDLK_q:
                k_call(PERPAD_Y, 0);
                break;
            case SDLK_w:
                k_call(PERPAD_Z, 0);
                break;
        }
    }
}
int main(int argc, char *argv[])
{
    int i;
    YuiInit();
    yinit.stvbiospath = NULL;
    yinit.stvgamepath = NULL;
    yinit.vsyncon     = 1;

    int w   = (lowres_mode == 0) ? WINDOW_WIDTH : WINDOW_WIDTH_LOW;
    int h   = (lowres_mode == 0) ? WINDOW_HEIGHT : WINDOW_HEIGHT_LOW;
    Wwidth  = w;
    Wheight = h;

    if (!platform_SetupSDL2(w, h, fullscreen))
        exit(EXIT_FAILURE);

    yinit.cdcoretype  = 1;
    yinit.cdpath      = argv[1];
    yinit.sndcoretype = 0;

    // yinit.carttype    = CART_DRAM8MBIT;
    // yinit.stvgamepath = argv[1];

    if (YabauseInit(&yinit) != 0) {
        printf("YabauseInit error \n\r");
        return 1;
    }

    if (lowres_mode == 0) {
        if (yinit.vidcoretype == VIDCORE_OGL) {
            VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
            VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_4XBRZ);
        }
        VIDCore->Resize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
    } else {
        if (yinit.vidcoretype == VIDCORE_OGL) {
            VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
            VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_2XBRZ);
        }
        VIDCore->Resize(0, 0, WINDOW_WIDTH_LOW, WINDOW_HEIGHT_LOW, 1);
    }

    platform_SetKeyCallback(PERCore->onKeyEvent);

    SDL_Event Event;
    while (Running) {
        if (YabauseExec() == -1)
            break;

        while (SDL_PollEvent(&Event)) {
            switch (Event.type) {
                case SDL_QUIT:
                    Running = 0;
                    break;
                case SDL_KEYDOWN:
                    keyPressEvent(Event.key.keysym.sym);
                    break;
                case SDL_KEYUP:
                    keyReleaseEvent(Event.key.keysym.sym);
                    break;
            }
        }
    }
    // YabauseDeInit();
    SDL_Quit();
    printf("finish\n");
    return 0;
}
