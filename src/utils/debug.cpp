

#include "debug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Debug *DebugInit(const char *n, DebugOutType t, char *s)
{
    Debug *d;

    if ((d = (Debug *)malloc(sizeof(Debug))) == NULL)
        return NULL;

    d->output_type = t;

    if ((d->name = strdup(n)) == NULL) {
        free(d);
        return NULL;
    }

    switch (t) {
        case DEBUG_STREAM:
            d->output.stream = fopen(s, "w");
            break;
        case DEBUG_STRING:
            d->output.string = s;
            break;
        case DEBUG_STDOUT:
            d->output.stream = stdout;
            break;
        case DEBUG_STDERR:
            d->output.stream = stderr;
            break;
        case DEBUG_CALLBACK:
            d->output.callback = (void (*)(char *))s;
            break;
    }

    return d;
}

void DebugDeInit(Debug *d)
{
    if (d == NULL)
        return;

    switch (d->output_type) {
        case DEBUG_STREAM:
            if (d->output.stream)
                fclose(d->output.stream);
            break;
        case DEBUG_STRING:
        case DEBUG_STDOUT:
        case DEBUG_STDERR:
        case DEBUG_CALLBACK:
            break;
    }
    if (d->name)
        free(d->name);
    free(d);
}

void DebugChangeOutput(Debug *d, DebugOutType t, char *s)
{
    if (t != d->output_type) {
        if (d->output_type == DEBUG_STREAM) {
            if (d->output.stream)
                fclose(d->output.stream);
        }
        d->output_type = t;
    }
    switch (t) {
        case DEBUG_STREAM:
            d->output.stream = fopen(s, "w");
            break;
        case DEBUG_STRING:
            d->output.string = s;
            break;
        case DEBUG_CALLBACK:
            d->output.callback = (void (*)(char *))s;
            break;
        case DEBUG_STDOUT:
            d->output.stream = stdout;
            break;
        case DEBUG_STDERR:
            d->output.stream = stderr;
            break;
    }
}

#ifdef _WINDOWS
#include <Windows.h>
#endif

void DebugLog(const char *format, ...)
{
    static char strtmp[512];
    int         i = 0;
    va_list     l;
    va_start(l, format);
    i += vsprintf(strtmp + i, format, l);
    OSDAddLogString(strtmp);
    va_end(l);
}

void DebugPrintf(Debug *d, const char *file, u32 line, const char *format, ...)
{
    va_list     l;
    static char strtmp[512];
    static int  strhash;

    if (d == NULL)
        return;

    va_start(l, format);

    switch (d->output_type) {
        case DEBUG_STDOUT:
        case DEBUG_STDERR:
        case DEBUG_STREAM:
            if (d->output.stream == NULL)
                break;
            fprintf(d->output.stream, "%s (%s:%ld): ", d->name, file, (long)line);
            vfprintf(d->output.stream, format, l);
            break;
        case DEBUG_STRING: {
            int i;
            if (d->output.string == NULL)
                break;

            i = sprintf(d->output.string, "%s (%s:%ld): ", d->name, file, (long)line);
            vsprintf(d->output.string + i, format, l);
        } break;
        case DEBUG_CALLBACK: {
            int i          = 0;
            int strnewhash = 0;
#ifdef _WINDOWS
            static FILE *dfp = NULL;
            if (dfp == NULL) {
                dfp = fopen("debug.txt", "w");
            }
#endif
#ifdef ANDROID
            static FILE *dfp = NULL;
            if (dfp == NULL) {
                dfp = fopen("/mnt/sdcard/debug.txt", "w");
            }
#endif
            i += vsprintf(strtmp + i, format, l);
            OSDAddLogString(strtmp);
#if defined(ANDROID)
            fprintf(dfp, "%s", strtmp);
            fflush(dfp);
#endif
#if defined(_WINDOWS)
            fprintf(dfp, "%s\n", strtmp);
            fflush(dfp);
#endif
        } break;
    }

    va_end(l);
}

Debug *MainLog;

void LogStart(void)
{
    MainLog = DebugInit("main", DEBUG_STDOUT, NULL);
}

void LogStop(void)
{
    DebugDeInit(MainLog);
    MainLog = NULL;
}

void LogChangeOutput(DebugOutType t, char *s)
{

    DebugChangeOutput(MainLog, t, s);
}
