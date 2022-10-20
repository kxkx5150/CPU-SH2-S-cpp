#define PLATFORM_LINUX
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
// #include "gameinfo.h"
#include "sh2core.h"
#include "sh2int_kronos.h"
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
#include "sh2int_kronos.h"
#endif
#include <stdio.h>
#include "sh2core.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
typedef void (*k_callback)(unsigned int key, unsigned char state);
// extern int  platform_SetupOpenGL(int w, int h, int fullscreen);
// extern int  platform_YuiRevokeOGLOnThisThread();
// extern int  platform_YuiUseOGLOnThisThread();
// extern void platform_swapBuffers(void);
// extern int  platform_shouldClose();
// extern void platform_Close();
// extern int  platform_Deinit(void);
// extern void platform_HandleEvent();
// extern void platform_SetKeyCallback(k_callback call);
// extern void platform_getFBSize(int *w, int *h);
#define AR                (4.0f / 3.0f)
#define WINDOW_WIDTH      600
#define WINDOW_HEIGHT     ((int)((float)WINDOW_WIDTH / AR))
#define WINDOW_WIDTH_LOW  300
#define WINDOW_HEIGHT_LOW ((int)((float)WINDOW_WIDTH_LOW / AR))
static int Wwidth;
static int Wheight;
#define OSDCORE_DUMMY   0
#define OSDCORE_SOFT    2
#define OSDCORE_NANOVG  3
#define OSDCORE_DEFAULT OSDCORE_NANOVG
M68K_struct           *M68KCoreList[]   = {&M68KDummy, &M68KMusashi, NULL};
SH2Interface_struct   *SH2CoreList[]    = {&SH2KronosInterpreter, &SH2KronosInterpreter, &SH2KronosInterpreter, NULL};
PerInterface_struct   *PERCoreList[]    = {&PERDummy, &PERLinuxJoy, NULL};
CDInterface           *CDCoreList[]     = {&DummyCD, &ISOCD, &ArchCD, NULL};
SoundInterface_struct *SNDCoreList[]    = {&SNDDummy, NULL};
VideoInterface_struct *VIDCoreList[]    = {&VIDOGL, NULL, NULL};
static int             fullscreen       = 0;
static int             scanline         = 0;
static int             lowres_mode      = 0;
static char            biospath[256]    = "\0";
static char            cdpath[256]      = "\0";
static char            stvgamepath[256] = "\0";
static char            stvbiospath[256] = "\0";
static GLFWwindow     *g_window         = NULL;
static GLFWwindow     *g_offscreen_context;
static k_callback      k_call = NULL;
static unsigned char   inputMap[512];
yabauseinit_struct     yinit;
int                    platform_YuiRevokeOGLOnThisThread()
{
#if defined(YAB_ASYNC_RENDERING)
    glfwMakeContextCurrent(g_offscreen_context);
#endif
    return 0;
}
int platform_YuiUseOGLOnThisThread()
{
#if defined(YAB_ASYNC_RENDERING)
    glfwMakeContextCurrent(g_window);
#endif
    return 0;
}
void platform_swapBuffers(void)
{
    if (g_window == NULL) {
        return;
    }
    glfwSwapBuffers(g_window);
}
static void error_callback(int error, const char *description)
{
    fputs(description, stderr);
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (k_call != NULL) {
        switch (action) {
            case GLFW_RELEASE:
                k_call(inputMap[key], 0);
                break;
            case GLFW_PRESS:
                k_call(inputMap[key], 1);
                break;
            case GLFW_REPEAT:
                k_call(inputMap[key], 0);
                k_call(inputMap[key], 1);
                break;
            default:
                break;
        }
    }
}
void platform_getFBSize(int *w, int *h)
{
    glfwGetFramebufferSize(g_window, w, h);
}
int platform_SetupOpenGL(int w, int h, int fullscreen)
{
    int i;
    if (!glfwInit())
        return 0;
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    g_window = glfwCreateWindow(w, h, "", NULL, NULL);
    if (!g_window) {
        glfwTerminate();
        return 0;
    }
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    g_offscreen_context = glfwCreateWindow(w, h, "", NULL, g_window);
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(0);
    glewExperimental = GL_TRUE;
    int maj, min;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);
    printf("OpenGL version is %d.%d (%s, %s)\n", maj, min, glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    for (i = 0; i < 512; i++)
        inputMap[i] = -1;
    inputMap[GLFW_KEY_UP]    = PERPAD_UP;
    inputMap[GLFW_KEY_RIGHT] = PERPAD_RIGHT;
    inputMap[GLFW_KEY_DOWN]  = PERPAD_DOWN;
    inputMap[GLFW_KEY_LEFT]  = PERPAD_LEFT;
    inputMap[GLFW_KEY_R]     = PERPAD_RIGHT_TRIGGER;
    inputMap[GLFW_KEY_F]     = PERPAD_LEFT_TRIGGER;
    inputMap[GLFW_KEY_SPACE] = PERPAD_START;
    inputMap[GLFW_KEY_Q]     = PERPAD_A;
    inputMap[GLFW_KEY_S]     = PERPAD_B;
    inputMap[GLFW_KEY_D]     = PERPAD_C;
    inputMap[GLFW_KEY_A]     = PERPAD_X;
    inputMap[GLFW_KEY_Z]     = PERPAD_Y;
    inputMap[GLFW_KEY_E]     = PERPAD_Z;
    inputMap[GLFW_KEY_G]     = PERJAMMA_SERVICE;
    inputMap[GLFW_KEY_T]     = PERJAMMA_TEST;
    inputMap[GLFW_KEY_Y]     = PERJAMMA_COIN1;
    inputMap[GLFW_KEY_H]     = PERJAMMA_COIN2;
    inputMap[GLFW_KEY_C]     = PERJAMMA_START1;
    inputMap[GLFW_KEY_V]     = PERJAMMA_START2;
    inputMap[GLFW_KEY_P]     = PERJAMMA_PAUSE;
    inputMap[GLFW_KEY_M]     = PERJAMMA_MULTICART;
    glfwSetKeyCallback(g_window, key_callback);
    return 1;
}
int platform_shouldClose()
{
    return glfwWindowShouldClose(g_window);
}
void platform_Close()
{
    glfwSetWindowShouldClose(g_window, GL_TRUE);
}
int platform_Deinit(void)
{
    glfwDestroyWindow(g_window);
    glfwDestroyWindow(g_offscreen_context);
    glfwTerminate();
    return 0;
}
void platform_HandleEvent(void)
{
    glfwPollEvents();
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
    platform_swapBuffers();
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
static int SetupOpenGL()
{
    int w   = (lowres_mode == 0) ? WINDOW_WIDTH : WINDOW_WIDTH_LOW;
    int h   = (lowres_mode == 0) ? WINDOW_HEIGHT : WINDOW_HEIGHT_LOW;
    Wwidth  = w;
    Wheight = h;
    if (!platform_SetupOpenGL(w, h, fullscreen))
        exit(EXIT_FAILURE);

    return 0;
}
void initEmulation()
{
    YuiInit();
    SetupOpenGL();
    if (YabauseSh2Init(&yinit) != 0) {
        printf("YabauseSh2Init error \n\r");
        return;
    }
}
int main(int argc, char *argv[])
{
    int i;
    YuiInit();
    yinit.stvbiospath = NULL;
    yinit.stvgamepath = NULL;
    yinit.vsyncon     = 1;
    SetupOpenGL();
    yinit.cdcoretype  = 1;
    yinit.cdpath      = argv[1];    // argv[1];
    yinit.sndcoretype = 0;
    if (YabauseInit(&yinit) != 0) {
        printf("YabauseInit error \n\r");
        return 1;
    }
    if (lowres_mode == 0) {
        if (yinit.vidcoretype == VIDCORE_OGL) {
            VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
            VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_4XBRZ);
            // VIDCore->SetSettingValue(VDP_SETTING_SCANLINE, scanline);
        }
        VIDCore->Resize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
    } else {
        if (yinit.vidcoretype == VIDCORE_OGL) {
            VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
            VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_2XBRZ);
            // VIDCore->SetSettingValue(VDP_SETTING_SCANLINE, scanline);
        }
        VIDCore->Resize(0, 0, WINDOW_WIDTH_LOW, WINDOW_HEIGHT_LOW, 1);
    }
    platform_SetKeyCallback(PERCore->onKeyEvent);
    while (!platform_shouldClose()) {
        int height;
        int width;
        platform_getFBSize(&width, &height);
        if ((Wwidth != width) || (Wheight != height)) {
            Wwidth  = width;
            Wheight = height;
            VIDCore->Resize(0, 0, Wwidth, Wheight, 1);
        }
        if (YabauseExec() == -1)
            platform_Close();
        platform_HandleEvent();
    }
    YabauseDeInit();
    platform_Deinit();
    return 0;
}
