
#include "debug.h"
#include "perlinuxjoy.h"
#include <linux/joystick.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glob.h>
#include <limits.h>

int         PERLinuxJoyInit(void);
void        PERLinuxJoyDeInit(void);
int         PERLinuxJoyHandleEvents(void);
u32         PERLinuxJoyScan(u32 flags);
void        PERLinuxJoyFlush(void);
void        PERLinuxKeyName(u32 key, char *name, int size);
static void PERLinuxKeyPress(u32 key, u8 state);

int getSupportedJoy(const char *name);

#define SERVICE_BUTTON_NUMBER 3
#define SERVICE_BUTTON_EXIT   (PERGUN_AXIS + 1)
#define SERVICE_BUTTON_TOGGLE (PERGUN_AXIS + 2)
#define SERVICE_TOGGLE_EXIT   (PERGUN_AXIS + 3)

typedef struct
{
    const char *name;
    int         code[PERGUN_AXIS + 1 + SERVICE_BUTTON_NUMBER];
} joymapping_struct;

#define MAPPING_NB 5

joymapping_struct joyMapping[MAPPING_NB] = {
    {"SZMy-power LTD CO.  Dual Box WII",
     {
         JS_EVENT_BUTTON << 8 | 12,
         JS_EVENT_BUTTON << 8 | 13,
         JS_EVENT_BUTTON << 8 | 14,
         JS_EVENT_BUTTON << 8 | 15,
         JS_EVENT_BUTTON << 8 | 5,
         JS_EVENT_BUTTON << 8 | 4,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 2,
         JS_EVENT_BUTTON << 8 | 1,
         JS_EVENT_BUTTON << 8 | 7,
         JS_EVENT_BUTTON << 8 | 3,
         JS_EVENT_BUTTON << 8 | 0,
         JS_EVENT_BUTTON << 8 | 6,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         JS_EVENT_BUTTON << 8 | 10,
         -1,
         -1,
     }},
    {"Sony PLAYSTATION(R)3 Controller",
     {
         JS_EVENT_BUTTON << 8 | 4,
         JS_EVENT_BUTTON << 8 | 5,
         JS_EVENT_BUTTON << 8 | 6,
         JS_EVENT_BUTTON << 8 | 7,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 8,
         JS_EVENT_BUTTON << 8 | 3,
         JS_EVENT_BUTTON << 8 | 14,
         JS_EVENT_BUTTON << 8 | 13,
         JS_EVENT_BUTTON << 8 | 11,
         JS_EVENT_BUTTON << 8 | 15,
         JS_EVENT_BUTTON << 8 | 12,
         JS_EVENT_BUTTON << 8 | 10,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         JS_EVENT_BUTTON << 8 | 16,
         -1,
         -1,
     }},
    {"HuiJia  USB GamePad",
     {
         JS_EVENT_BUTTON << 8 | 12,
         JS_EVENT_BUTTON << 8 | 13,
         JS_EVENT_BUTTON << 8 | 14,
         JS_EVENT_BUTTON << 8 | 15,
         JS_EVENT_BUTTON << 8 | 7,
         JS_EVENT_BUTTON << 8 | 5,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 0,
         JS_EVENT_BUTTON << 8 | 1,
         JS_EVENT_BUTTON << 8 | 2,
         JS_EVENT_BUTTON << 8 | 3,
         JS_EVENT_BUTTON << 8 | 4,
         JS_EVENT_BUTTON << 8 | 6,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 0,
     }},
    {"Thrustmaster Run'N' Drive",
     {
         JS_EVENT_AXIS << 8 | 0x10000 | 8,
         JS_EVENT_AXIS << 8 | 7,
         JS_EVENT_AXIS << 8 | 8,
         JS_EVENT_AXIS << 8 | 0x10000 | 7,
         JS_EVENT_AXIS << 8 | 5,
         JS_EVENT_AXIS << 8 | 0x10000 | 5,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 0,
         JS_EVENT_BUTTON << 8 | 3,
         JS_EVENT_BUTTON << 8 | 4,
         JS_EVENT_BUTTON << 8 | 1,
         JS_EVENT_BUTTON << 8 | 2,
         JS_EVENT_BUTTON << 8 | 5,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         JS_EVENT_BUTTON << 8 | 8,
         JS_EVENT_BUTTON << 8 | 9,
     }},
    {"HORI CO.,LTD. Fighting Commander 4",
     {
         JS_EVENT_AXIS << 8 | 0x10000 | 7,
         JS_EVENT_AXIS << 8 | 6,
         JS_EVENT_AXIS << 8 | 7,
         JS_EVENT_AXIS << 8 | 0x10000 | 6,
         JS_EVENT_BUTTON << 8 | 4,
         JS_EVENT_BUTTON << 8 | 10,
         JS_EVENT_BUTTON << 8 | 9,
         JS_EVENT_BUTTON << 8 | 1,
         JS_EVENT_BUTTON << 8 | 2,
         JS_EVENT_BUTTON << 8 | 7,
         JS_EVENT_BUTTON << 8 | 0,
         JS_EVENT_BUTTON << 8 | 3,
         JS_EVENT_BUTTON << 8 | 5,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         -1,
         JS_EVENT_BUTTON << 8 | 12,
         JS_EVENT_BUTTON << 8 | 9,
     }},
};

#define KEYPAD(key, player) ((player << 17) | key)

static void KeyInit()
{
    void *padbits;

    PerPortReset();
    padbits = PerPadAdd(&PORTDATA1);

    PerSetKey(KEYPAD(PERPAD_UP, 0), PERPAD_UP, padbits);
    PerSetKey(KEYPAD(PERPAD_RIGHT, 0), PERPAD_RIGHT, padbits);
    PerSetKey(KEYPAD(PERPAD_DOWN, 0), PERPAD_DOWN, padbits);
    PerSetKey(KEYPAD(PERPAD_LEFT, 0), PERPAD_LEFT, padbits);
    PerSetKey(KEYPAD(PERPAD_RIGHT_TRIGGER, 0), PERPAD_RIGHT_TRIGGER, padbits);
    PerSetKey(KEYPAD(PERPAD_LEFT_TRIGGER, 0), PERPAD_LEFT_TRIGGER, padbits);
    PerSetKey(KEYPAD(PERPAD_START, 0), PERPAD_START, padbits);
    PerSetKey(KEYPAD(PERPAD_A, 0), PERPAD_A, padbits);
    PerSetKey(KEYPAD(PERPAD_B, 0), PERPAD_B, padbits);
    PerSetKey(KEYPAD(PERPAD_C, 0), PERPAD_C, padbits);
    PerSetKey(KEYPAD(PERPAD_X, 0), PERPAD_X, padbits);
    PerSetKey(KEYPAD(PERPAD_Y, 0), PERPAD_Y, padbits);
    PerSetKey(KEYPAD(PERPAD_Z, 0), PERPAD_Z, padbits);

    padbits = PerPadAdd(&PORTDATA2);

    PerSetKey(KEYPAD(PERPAD_UP, 1), PERPAD_UP, padbits);
    PerSetKey(KEYPAD(PERPAD_RIGHT, 1), PERPAD_RIGHT, padbits);
    PerSetKey(KEYPAD(PERPAD_DOWN, 1), PERPAD_DOWN, padbits);
    PerSetKey(KEYPAD(PERPAD_LEFT, 1), PERPAD_LEFT, padbits);
    PerSetKey(KEYPAD(PERPAD_RIGHT_TRIGGER, 1), PERPAD_RIGHT_TRIGGER, padbits);
    PerSetKey(KEYPAD(PERPAD_LEFT_TRIGGER, 1), PERPAD_LEFT_TRIGGER, padbits);
    PerSetKey(KEYPAD(PERPAD_START, 1), PERPAD_START, padbits);
    PerSetKey(KEYPAD(PERPAD_A, 1), PERPAD_A, padbits);
    PerSetKey(KEYPAD(PERPAD_B, 1), PERPAD_B, padbits);
    PerSetKey(KEYPAD(PERPAD_C, 1), PERPAD_C, padbits);
    PerSetKey(KEYPAD(PERPAD_X, 1), PERPAD_X, padbits);
    PerSetKey(KEYPAD(PERPAD_Y, 1), PERPAD_Y, padbits);
    PerSetKey(KEYPAD(PERPAD_Z, 1), PERPAD_Z, padbits);

    padbits = PerCabAdd(NULL);
    PerSetKey(PERPAD_UP, PERPAD_UP, padbits);
    PerSetKey(PERPAD_RIGHT, PERPAD_RIGHT, padbits);
    PerSetKey(PERPAD_DOWN, PERPAD_DOWN, padbits);
    PerSetKey(PERPAD_LEFT, PERPAD_LEFT, padbits);
    PerSetKey(PERPAD_A, PERPAD_A, padbits);
    PerSetKey(PERPAD_B, PERPAD_B, padbits);
    PerSetKey(PERPAD_C, PERPAD_C, padbits);
    PerSetKey(PERPAD_X, PERPAD_X, padbits);
    PerSetKey(PERPAD_Y, PERPAD_Y, padbits);
    PerSetKey(PERPAD_Z, PERPAD_Z, padbits);

    PerSetKey(PERJAMMA_COIN1, PERJAMMA_COIN1, padbits);
    PerSetKey(PERJAMMA_COIN2, PERJAMMA_COIN2, padbits);
    PerSetKey(PERJAMMA_TEST, PERJAMMA_TEST, padbits);
    PerSetKey(PERJAMMA_SERVICE, PERJAMMA_SERVICE, padbits);
    PerSetKey(PERJAMMA_START1, PERJAMMA_START1, padbits);
    PerSetKey(PERJAMMA_START2, PERJAMMA_START2, padbits);
    PerSetKey(PERJAMMA_MULTICART, PERJAMMA_MULTICART, padbits);
    PerSetKey(PERJAMMA_PAUSE, PERJAMMA_PAUSE, padbits);

    PerSetKey(KEYPAD(PERPAD_UP, 1), PERJAMMA_P2_UP, padbits);
    PerSetKey(KEYPAD(PERPAD_RIGHT, 1), PERJAMMA_P2_RIGHT, padbits);
    PerSetKey(KEYPAD(PERPAD_DOWN, 1), PERJAMMA_P2_DOWN, padbits);
    PerSetKey(KEYPAD(PERPAD_LEFT, 1), PERJAMMA_P2_LEFT, padbits);
    PerSetKey(KEYPAD(PERPAD_A, 1), PERJAMMA_P2_BUTTON1, padbits);
    PerSetKey(KEYPAD(PERPAD_B, 1), PERJAMMA_P2_BUTTON2, padbits);
    PerSetKey(KEYPAD(PERPAD_C, 1), PERJAMMA_P2_BUTTON3, padbits);
    PerSetKey(KEYPAD(PERPAD_X, 1), PERJAMMA_P2_BUTTON4, padbits);
}

int requestExit = 0;
int toggle      = 0;
int service     = 0;

#define TOGGLE_EXIT 0

PerInterface_struct PERLinuxJoy = {PERCORE_LINUXJOY,
                                   "Linux Joystick Interface",
                                   PERLinuxJoyInit,
                                   PERLinuxJoyDeInit,
                                   PERLinuxJoyHandleEvents,
                                   PERLinuxJoyScan,
                                   1,
                                   PERLinuxJoyFlush,
                                   PERLinuxKeyName,
                                   PERLinuxKeyPress};

typedef struct
{
    int  fd;
    int *axis;
    int  axiscount;
    int  id;
    int  mapping;
} perlinuxjoy_struct;

static perlinuxjoy_struct *joysticks = NULL;
static int                 joycount  = 0;

#define PACKEVENT(evt, joy)                                                                                            \
    ((joy->id << 17) | getPerPadKey(evt.value, (evt.value < 0 ? 0x10000 : 0) | (evt.type << 8) | (evt.number), joy))
#define THRESHOLD 1000
#define MAXAXIS   256

static int handleToggle(int state, int val, perlinuxjoy_struct *joystick)
{
    int i;
    int ret = 0;
    for (i = PERGUN_AXIS + 2; i < PERGUN_AXIS + 1 + SERVICE_BUTTON_NUMBER; i++) {
        if (joyMapping[joystick->mapping].code[i] == val) {
            switch (i) {
                case SERVICE_BUTTON_TOGGLE:
                    if (state != 0)
                        toggle = 1;
                    else
                        toggle = 0;
                    break;
                case SERVICE_TOGGLE_EXIT:
                    if (state != 0)
                        service |= (1 << TOGGLE_EXIT);
                    else
                        service &= ~(1 << TOGGLE_EXIT);
                    break;
                default:
                    break;
            }
        }
    }
    if (toggle) {
        requestExit = service & (1 << TOGGLE_EXIT);
        if (requestExit)
            ret = 1;
    }
    return ret;
}

static int getPerPadKey(int state, int val, perlinuxjoy_struct *joystick)
{
    int i;
    int ret = -1;
    if (handleToggle(state, val, joystick))
        return ret;
    for (i = 0; i < PERGUN_AXIS + 2; i++) {
        if (joyMapping[joystick->mapping].code[i] == val) {
            switch (i) {
                case SERVICE_BUTTON_EXIT:
                    requestExit = 1;
                    break;
                default:
                    ret = i;
            }
            break;
        }
    }
    return ret;
}

static void PERLinuxKeyPress(u32 key, u8 state)
{
    switch (state) {
        case 0:
            PerKeyUp(key);
            break;
        case 1:
            PerKeyDown(key);
            break;
        default:
            break;
    }
}

static int LinuxJoyInit(perlinuxjoy_struct *joystick, const char *path, int id)
{
    int             i;
    int             fd;
    int             axisinit[MAXAXIS];
    char            name[128];
    struct js_event evt;
    ssize_t         num_read;

    joystick->fd = open(path, O_RDONLY | O_NONBLOCK);
    if (joystick->fd == -1)
        return -1;
    if (ioctl(joystick->fd, JSIOCGNAME(sizeof(name)), name) < 0)
        strncpy(name, "Unknown", sizeof(name));
    printf("Joyinit open %s ", name);
    if ((joystick->mapping = getSupportedJoy(name)) == -1) {
        printf("is not supported\n");
        return -1;
    } else {
        printf("is supported => Player %d\n", id + 1);
    }
    joystick->axiscount = 0;
    joystick->id        = id;
    while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0) {
        if (evt.type == (JS_EVENT_AXIS | JS_EVENT_INIT)) {
            axisinit[evt.number] = evt.value;
            if (evt.number + 1 > joystick->axiscount) {
                joystick->axiscount = evt.number + 1;
            }
        }
    }

    if (joystick->axiscount > MAXAXIS)
        joystick->axiscount = MAXAXIS;

    joystick->axis = (int *)malloc(sizeof(int) * joystick->axiscount);
    for (i = 0; i < joystick->axiscount; i++) {
        joystick->axis[i] = axisinit[i];
    }

    KeyInit();
    return 0;
}

static void LinuxJoyDeInit(perlinuxjoy_struct *joystick)
{
    if (joystick->fd == -1)
        return;

    close(joystick->fd);
    free(joystick->axis);
}

static void LinuxJoyHandleEvents(perlinuxjoy_struct *joystick)
{
    struct js_event evt;
    ssize_t         num_read;
    int             key;

    if (joystick->fd == -1)
        return;

    while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0) {
        if (evt.type == JS_EVENT_AXIS) {
            int disp;
            u8  axis = evt.number;

            if (axis >= joystick->axiscount)
                return;

            disp = abs(evt.value);
            if (disp < THRESHOLD)
                evt.value = 0;
            else if (evt.value < 0)
                evt.value = -1;
            else
                evt.value = 1;
        }
        key = PACKEVENT(evt, joystick);
        if (evt.value != 0) {
            if ((key & 0x1FFFF) != 0x1FFFF) {
                PerKeyDown(key);
            }
        } else {
            if ((key & 0x1FFFF) != 0x1FFFF) {
                PerKeyUp(key);
                evt.value = -1;
                key       = PACKEVENT(evt, joystick);
                if ((key & 0x1FFFF) != 0x1FFFF)
                    PerKeyUp(key);
            }
        }
    }
}

static int LinuxJoyScan(perlinuxjoy_struct *joystick)
{
    struct js_event evt;
    ssize_t         num_read;
    int             key;
    if (joystick->fd == -1)
        return 0;

    if ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) <= 0)
        return 0;
    if (evt.type == JS_EVENT_AXIS) {
        int disp;
        u8  axis = evt.number;

        if (axis >= joystick->axiscount)
            return 0;

        disp = abs(evt.value);
        if (disp < THRESHOLD)
            return 0;
        else if (evt.value < 0)
            evt.value = -1;
        else
            evt.value = 1;
    }
    key = PACKEVENT(evt, joystick);
    if ((key & 0x1FFFF) != 0x1FFFF)
        return key;
    else
        return 0;
}

static void LinuxJoyFlush(perlinuxjoy_struct *joystick)
{
    struct js_event evt;
    ssize_t         num_read;

    if (joystick->fd == -1)
        return;

    while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0)
        ;
}

int getSupportedJoy(const char *name)
{
    int i;
    for (i = 0; i < MAPPING_NB; i++) {
        if (strncmp(name, joyMapping[i].name, 128) == 0)
            return i;
    }
    return -1;
}

int PERLinuxJoyInit(void)
{
    int    i;
    int    fd;
    int    j = 0;
    glob_t globbuf;

    glob("/dev/input/js*", 0, NULL, &globbuf);

    joycount  = globbuf.gl_pathc;
    joysticks = (perlinuxjoy_struct *)calloc(sizeof(perlinuxjoy_struct), joycount);
    for (i = 0; i < globbuf.gl_pathc; i++) {
        perlinuxjoy_struct *joy = joysticks + j;
        j                       = (LinuxJoyInit(joy, globbuf.gl_pathv[i], j) < 0) ? j : j + 1;
    }
    joycount = j;
    globfree(&globbuf);

    if (globbuf.gl_pathc <= 0)
        KeyInit();

    return 0;
}

void PERLinuxJoyDeInit(void)
{
    int i;

    for (i = 0; i < joycount; i++)
        LinuxJoyDeInit(joysticks + i);

    free(joysticks);
}

int PERLinuxJoyHandleEvents(void)
{
    int i;

    for (i = 0; i < joycount; i++)
        LinuxJoyHandleEvents(joysticks + i);

    if (requestExit)
        return -1;

    return 0;
}

u32 PERLinuxJoyScan(u32 flags)
{
    int i;

    for (i = 0; i < joycount; i++) {
        int ret = LinuxJoyScan(joysticks + i);
        if (ret != 0)
            return ret;
    }

    return 0;
}

void PERLinuxJoyFlush(void)
{
    int i;

    for (i = 0; i < joycount; i++)
        LinuxJoyFlush(joysticks + i);
}

void PERLinuxKeyName(u32 key, char *name, UNUSED int size)
{
    sprintf(name, "%x", (int)key);
}
