

#ifndef CDBASE_H
#define CDBASE_H

#include <stdio.h>
#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CDCORE_DEFAULT -1
#define CDCORE_DUMMY   0
#define CDCORE_ISO     1
#define CDCORE_ARCH    2
#define CDCORE_CHD     3

#define CDCORE_NORMAL 0
#define CDCORE_NODISC 2
#define CDCORE_OPEN   3

typedef struct
{
    int         id;
    const char *Name;
    int (*Init)(const char *);
    void (*DeInit)(void);
    int (*GetStatus)(void);
    s32 (*ReadTOC)(u32 *TOC);
    int (*ReadSectorFAD)(u32 FAD, void *buffer);
    void (*ReadAheadFAD)(u32 FAD);
    void (*SetStatus)(int status);
} CDInterface;

typedef struct
{
    char *filename;
    u8   *zipBuffer;
    u32   size;
} ZipEntry;

extern CDInterface DummyCD;

extern CDInterface ISOCD;

extern CDInterface ArchCD;

#ifdef __cplusplus
}
#endif

#endif
