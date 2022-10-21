
#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALIGNED
#ifdef _MSC_VER
#define ALIGNED(x) __declspec(align(x))
#else
#define ALIGNED(x) __attribute__((aligned(x)))
#endif
#endif

#ifndef STDCALL
#ifdef _MSC_VER
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

#ifndef FASTCALL
#ifdef __MINGW32__
#define FASTCALL __attribute__((fastcall))
#elif defined(__i386__)
#define FASTCALL __attribute__((regparm(3)))
#else
#define FASTCALL
#endif
#endif

#if defined(__BIG_ENDIAN__) || defined(__LITTLE_ENDIAN__)
#undef WORDS_BIGENDIAN
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN
#endif
#endif

#ifndef INLINE

#ifdef _MSC_VER
#define INLINE __forceinline
#else
#ifdef __GNUC__
#define INLINE __attribute__((always_inline)) inline
#else
#define INLINE inline
#endif
#endif

#endif

#ifdef GEKKO
#include <gccore.h>
typedef unsigned long pointer;

#else

#ifdef HAVE_STDINT_H

#include <stdint.h>
typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;
typedef uintptr_t pointer;

#else

typedef unsigned char  u8;
typedef unsigned short u16;

typedef signed char   s8;
typedef signed short  s16;

#if defined(__LP64__)
typedef unsigned int  u32;
typedef unsigned long u64;
typedef unsigned long pointer;

typedef signed int  s32;
typedef signed long s64;

#elif defined(_MSC_VER)
typedef unsigned long      u32;
typedef unsigned __int64   u64;
typedef unsigned long long u64;
#ifdef _WIN64
typedef __int64            pointer;
#else
typedef unsigned long pointer;
#endif

typedef signed long      s32;
typedef __int64          s64;
typedef signed long long s64;

#else
typedef unsigned long      u32;
typedef unsigned long long u64;
typedef unsigned long      pointer;

typedef signed long      s32;
typedef signed long long s64;
#endif

#endif

#endif

void MemStateWrite(void *ptr, size_t size, size_t nmemb, void **stream);
void MemStateWriteOffset(void *ptr, size_t size, size_t nmemb, void **stream, int offset);
int  MemStateWriteHeader(void **stream, const char *name, int version);
int  MemStateFinishHeader(void **stream, int offset);
void MemStateRead(void *ptr, size_t size, size_t nmemb, const void *stream);
void MemStateReadOffset(void *ptr, size_t size, size_t nmemb, const void *stream, int offset);
int  MemStateCheckRetrieveHeader(const void *stream, const char *name, int *version, int *size);
void MemStateSetOffset(int offset);
int  MemStateGetOffset();

typedef struct
{
    unsigned int size;
    unsigned int done;
} IOCheck_struct;

static INLINE void ywrite(IOCheck_struct *check, void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    check->done += (unsigned int)fwrite(ptr, size, nmemb, stream);
    check->size += (unsigned int)nmemb;
}

static INLINE void yread(IOCheck_struct *check, void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    check->done += (unsigned int)fread(ptr, size, nmemb, stream);
    check->size += (unsigned int)nmemb;
}

static INLINE int StateCheckRetrieveHeader(FILE *fp, const char *name, int *version, int *size)
{
    char   id[4];
    size_t ret;

    if ((ret = fread((void *)id, 1, 4, fp)) != 4)
        return -1;

    if (strncmp(name, id, 4) != 0)
        return -2;

    if ((ret = fread((void *)version, 4, 1, fp)) != 1)
        return -1;

    if (fread((void *)size, 4, 1, fp) != 1)
        return -1;

    return 0;
}

#ifdef HAVE_C99_VARIADIC_MACROS
#define AddString(s, ...)                                                                                              \
    {                                                                                                                  \
        sprintf(s, __VA_ARGS__);                                                                                       \
        s += strlen(s);                                                                                                \
    }
#else
#define AddString(s, ...)                                                                                              \
    {                                                                                                                  \
        sprintf(s, ##r);                                                                                               \
        s += strlen(s);                                                                                                \
    }
#endif

#ifdef HAVE_LIBMINI18N
#include "mini18n.h"
#else
#ifndef _
#define _(a) (a)
#endif
#endif

#undef MIN
#undef MAX
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef PSP
#define BSWAP16(x)  ((typeof(x))__builtin_allegrex_wsbh((x)))
#define BSWAP16L(x) BSWAP16(x)
#define BSWAP32(x)  ((typeof(x))__builtin_allegrex_wsbw((x)))
#define WSWAP32(x)  ((typeof(x))__builtin_allegrex_rotr((x), 16))
#endif

#ifdef __GNUC__
#ifdef HAVE_BUILTIN_BSWAP16
#define BSWAP16(x)  ((__builtin_bswap16((x) >> 16) << 16) | __builtin_bswap16((x)))
#define BSWAP16L(x) (__builtin_bswap16((x)))
#endif
#ifdef HAVE_BUILTIN_BSWAP32
#define BSWAP32(x) (__builtin_bswap32((x)))
#endif
#endif

#ifdef _MSC_VER
#define BSWAP16(x)  ((_byteswap_ushort((x) >> 16) << 16) | _byteswap_ushort((x)))
#define BSWAP16L(x) (_byteswap_ushort((x)))
#define BSWAP32(x)  (_byteswap_ulong((x)))
#define WSWAP32(x)  (_lrotr((x), 16))
#endif

#ifndef BSWAP16
#define BSWAP16(x) (((u32)(x) >> 8 & 0x00FF00FF) | ((u32)(x)&0x00FF00FF) << 8)
#endif
#ifndef BSWAP16L
#define BSWAP16L(x) (((u16)(x) >> 8 & 0xFF) | ((u16)(x)&0xFF) << 8)
#endif
#ifndef BSWAP32
#define BSWAP32(x) ((u32)(x) >> 24 | ((u32)(x) >> 8 & 0xFF00) | ((u32)(x)&0xFF00) << 8 | (u32)(x) << 24)
#endif
#ifndef WSWAP32
#define WSWAP32(x) ((u32)(x) >> 16 | (u32)(x) << 16)
#endif

#ifdef __GNUC__

#define UNUSED __attribute((unused))

#ifdef DEBUG
#define USED_IF_DEBUG
#else
#define USED_IF_DEBUG __attribute((unused))
#endif

#ifdef SMPC_DEBUG
#define USED_IF_SMPC_DEBUG
#else
#define USED_IF_SMPC_DEBUG __attribute((unused))
#endif

#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#else

#define UNUSED
#define USED_IF_DEBUG
#define USED_IF_SMPC_DEBUG
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)

#endif

#ifdef USE_16BPP
typedef u16 pixel_t;
#else
typedef u32       pixel_t;
#endif

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

#ifdef __cplusplus
}
#endif

#endif
