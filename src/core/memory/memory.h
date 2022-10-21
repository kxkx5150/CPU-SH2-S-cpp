
#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "core.h"
#include "sh2core.h"

u8  *T1MemoryInit(u32);
void T1MemoryDeInit(u8 *);

static INLINE u8 T1ReadByte(u8 *mem, u32 addr)
{
    return mem[addr];
}

static INLINE u16 T1ReadWord(u8 *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return *((u16 *)(mem + addr));
#else
    return BSWAP16L(*((u16 *)(mem + addr)));
#endif
}

static INLINE u32 T1ReadLong(u8 *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return *((u32 *)(mem + addr));
#else
    return BSWAP32(*((u32 *)(mem + addr)));
#endif
}

static INLINE void T1WriteByte(u8 *mem, u32 addr, u8 val)
{
    mem[addr] = val;
}

static INLINE void T1WriteWord(u8 *mem, u32 addr, u16 val)
{
#ifdef WORDS_BIGENDIAN
    *((u16 *)(mem + addr)) = val;
#else
    *((u16 *)(mem + addr)) = BSWAP16L(val);
#endif
}

static INLINE void T1WriteLong(u8 *mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
    *((u32 *)(mem + addr)) = val;
#else
    *((u32 *)(mem + addr)) = BSWAP32(val);
#endif
}

#define T2MemoryInit(x)   (T1MemoryInit(x))
#define T2MemoryDeInit(x) (T1MemoryDeInit(x))

static INLINE u8 T2ReadByte(u8 *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return mem[addr];
#else
    return mem[addr ^ 1];
#endif
}

static INLINE u16 T2ReadWord(u8 *mem, u32 addr)
{
    return *((u16 *)(mem + (addr & ~0x1)));
}

static INLINE u32 T2ReadLong(u8 *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return *((u32 *)(mem + (addr & ~0x3)));
#else
    return WSWAP32(*((u32 *)(mem + (addr & ~0x3))));
#endif
}

static INLINE void T2WriteByte(u8 *mem, u32 addr, u8 val)
{
#ifdef WORDS_BIGENDIAN
    mem[addr] = val;
#else
    mem[addr ^ 1]          = val;
#endif
}

static INLINE void T2WriteWord(u8 *mem, u32 addr, u16 val)
{
    *((u16 *)(mem + addr)) = val;
}

static INLINE void T2WriteLong(u8 *mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
    *((u32 *)(mem + addr)) = val;
#else
    *((u32 *)(mem + addr)) = WSWAP32(val);
#endif
}

typedef struct
{
    u8 *base_mem;
    u8 *mem;
} T3Memory;

T3Memory *T3MemoryInit(u32);
void      T3MemoryDeInit(T3Memory *);

static INLINE u8 T3ReadByte(T3Memory *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return mem->mem[addr];
#else
    return (mem->mem - addr - 1)[0];
#endif
}

static INLINE u16 T3ReadWord(T3Memory *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return *((u16 *)(mem->mem + addr));
#else
    return ((u16 *)(mem->mem - addr - 2))[0];
#endif
}

static INLINE u32 T3ReadLong(T3Memory *mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
    return *((u32 *)(mem->mem + addr));
#else
    return ((u32 *)(mem->mem - addr - 4))[0];
#endif
}

static INLINE void T3WriteByte(T3Memory *mem, u32 addr, u8 val)
{
#ifdef WORDS_BIGENDIAN
    mem->mem[addr] = val;
#else
    (mem->mem - addr - 1)[0]          = val;
#endif
}

static INLINE void T3WriteWord(T3Memory *mem, u32 addr, u16 val)
{
#ifdef WORDS_BIGENDIAN
    *((u16 *)(mem->mem + addr)) = val;
#else
    ((u16 *)(mem->mem - addr - 2))[0] = val;
#endif
}

static INLINE void T3WriteLong(T3Memory *mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
    *((u32 *)(mem->mem + addr)) = val;
#else
    ((u32 *)(mem->mem - addr - 4))[0] = val;
#endif
}

static INLINE int TSize(const char *filename)
{
    FILE *fp;
    u32   filesize;

    if (!filename)
        return -1;

    if ((fp = fopen(filename, "rb")) == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fclose(fp);
    return filesize;
}

static INLINE int T123Load(void *mem, u32 size, int type, const char *filename)
{
    FILE *fp;
    u32   filesize, filesizecheck;
    u8   *buffer;
    u32   i;

    if (!filename)
        return -1;

    if ((fp = fopen(filename, "rb")) == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filesize < size) {
        fclose(fp);
        return -1;
    }

    if (filesize > size) {
        filesize = size;
    }

    if ((buffer = (u8 *)malloc(filesize)) == NULL) {
        fclose(fp);
        return -1;
    }

    filesizecheck = (u32)fread((void *)buffer, 1, filesize, fp);
    fclose(fp);

    if (filesizecheck != filesize) {
        free(buffer);
        return -1;
    }

    switch (type) {
        case 1: {
            for (i = 0; i < filesize; i++)
                T1WriteByte((u8 *)mem, i, buffer[i]);
            break;
        }
        case 2: {
            for (i = 0; i < filesize; i++)
                T2WriteByte((u8 *)mem, i, buffer[i]);
            break;
        }
        case 3: {
            for (i = 0; i < filesize; i++)
                T3WriteByte((T3Memory *)mem, i, buffer[i]);
            break;
        }
        default: {
            free(buffer);
            return -1;
        }
    }

    free(buffer);

    return 0;
}

static INLINE int T123Save(void *mem, u32 size, int type, const char *filename)
{
    FILE *fp;
    u8   *buffer;
    u32   i;
    u32   sizecheck;

    if (filename == NULL)
        return 0;

    if (filename[0] == 0x00)
        return 0;

    if ((buffer = (u8 *)malloc(size)) == NULL)
        return -1;

    switch (type) {
        case 1: {
            for (i = 0; i < size; i++)
                buffer[i] = T1ReadByte((u8 *)mem, i);
            break;
        }
        case 2: {
            for (i = 0; i < size; i++)
                buffer[i] = T2ReadByte((u8 *)mem, i);
            break;
        }
        case 3: {
            for (i = 0; i < size; i++)
                buffer[i] = T3ReadByte((T3Memory *)mem, i);
            break;
        }
        default: {
            free(buffer);
            return -1;
        }
    }

    if ((fp = fopen(filename, "wb+")) == NULL) {
        free(buffer);
        return -1;
    }

    sizecheck = (u32)fwrite((void *)buffer, 1, size, fp);
    fclose(fp);
    free(buffer);

    if (sizecheck != size)
        return -1;

    return 0;
}

void          MappedMemoryInit(void);
u8 FASTCALL   MappedMemoryReadByte(SH2_struct *context, u32 addr);
u8 FASTCALL   SH2MappedMemoryReadByte(SH2_struct *context, u32 addr);
u16 FASTCALL  MappedMemoryReadWord(SH2_struct *context, u32 addr);
u16 FASTCALL  SH2MappedMemoryReadWord(SH2_struct *context, u32 addr);
u32 FASTCALL  MappedMemoryReadLong(SH2_struct *context, u32 addr);
u32 FASTCALL  SH2MappedMemoryReadLong(SH2_struct *context, u32 addr);
void FASTCALL MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val);
void FASTCALL MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val);
void FASTCALL MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val);
void FASTCALL SH2MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val);
void FASTCALL SH2MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val);
void FASTCALL SH2MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val);

u8 FASTCALL   DMAMappedMemoryReadByte(u32 addr);
u16 FASTCALL  DMAMappedMemoryReadWord(u32 addr);
u32 FASTCALL  DMAMappedMemoryReadLong(u32 addr);
void FASTCALL DMAMappedMemoryWriteByte(u32 addr, u8 val);
void FASTCALL DMAMappedMemoryWriteWord(u32 addr, u16 val);
void FASTCALL DMAMappedMemoryWriteLong(u32 addr, u32 val);

extern u8 *HighWram;
extern u8 *LowWram;
extern u8 *BiosRom;
extern u8 *BupRam;
extern u8  BupRamWritten;

typedef void(FASTCALL *writebytefunc)(SH2_struct *context, u8 *, u32, u8);
typedef void(FASTCALL *writewordfunc)(SH2_struct *context, u8 *, u32, u16);
typedef void(FASTCALL *writelongfunc)(SH2_struct *context, u8 *, u32, u32);

typedef u8(FASTCALL *readbytefunc)(SH2_struct *context, u8 *, u32);
typedef u16(FASTCALL *readwordfunc)(SH2_struct *context, u8 *, u32);
typedef u32(FASTCALL *readlongfunc)(SH2_struct *context, u8 *, u32);

extern writebytefunc WriteByteList[0x1000];
extern writewordfunc WriteWordList[0x1000];
extern writelongfunc WriteLongList[0x1000];

extern readbytefunc ReadByteList[0x1000];
extern readwordfunc ReadWordList[0x1000];
extern readlongfunc ReadLongList[0x1000];

extern u8 **MemoryBuffer[0x1000];

extern readbytefunc  CacheReadByteList[0x1000];
extern readwordfunc  CacheReadWordList[0x1000];
extern readlongfunc  CacheReadLongList[0x1000];
extern writebytefunc CacheWriteByteList[0x1000];
extern writewordfunc CacheWriteWordList[0x1000];
extern writelongfunc CacheWriteLongList[0x1000];

typedef struct
{
    u32 addr;
    u32 val;
} result_struct;

#define SEARCHBYTE 0
#define SEARCHWORD 1
#define SEARCHLONG 2

#define SEARCHEXACT       (0 << 2)
#define SEARCHLESSTHAN    (1 << 2)
#define SEARCHGREATERTHAN (2 << 2)

#define SEARCHUNSIGNED (0 << 4)
#define SEARCHSIGNED   (1 << 4)
#define SEARCHHEX      (2 << 4)
#define SEARCHSTRING   (3 << 4)
#define SEARCHREL8BIT  (6 << 4)
#define SEARCHREL16BIT (7 << 4)

result_struct *MappedMemorySearch(SH2_struct *context, u32 startaddr, u32 endaddr, int searchtype,
                                  const char *searchstr, result_struct *prevresults, u32 *maxresults);

int MappedMemoryLoad(SH2_struct *context, const char *filename, u32 addr);
int MappedMemorySave(SH2_struct *context, const char *filename, u32 addr, u32 size);

int  LoadBios(const char *filename);
int  LoadBackupRam(const char *filename);
void FormatBackupRam(u8 *mem, u32 size);

int YabSaveState(const char *filename);
int YabLoadState(const char *filename);
int YabSaveStateSlot(const char *dirpath, u8 slot);
int YabLoadStateSlot(const char *dirpath, u8 slot);
int YabSaveStateStream(void **stream);
int YabLoadStateStream(const void *stream, size_t size_stream);
int YabSaveStateBuffer(void **buffer, size_t *size);
int YabLoadStateBuffer(const void *buffer, size_t size);

int  BackupInit(const char *path, int extended);
void BackupFlush();
void BackupDeinit();

#ifdef __cplusplus
}
#endif

#define CACHE_LOG

#endif
