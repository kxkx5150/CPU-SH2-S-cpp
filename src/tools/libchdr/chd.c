#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chd.h"
#include "cdrom.h"
// #include "flac.h"
#include "huffman.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <zlib.h>
#define TRUE                     1
#define FALSE                    0
#define MAX(x, y)                (((x) > (y)) ? (x) : (y))
#define MIN(x, y)                (((x) < (y)) ? (x) : (y))
#define SHA1_DIGEST_SIZE         20
#define PRINTF_MAX_HUNK          (0)
#define MAP_STACK_ENTRIES        512
#define MAP_ENTRY_SIZE           16
#define OLD_MAP_ENTRY_SIZE       8
#define METADATA_HEADER_SIZE     16
#define CRCMAP_HASH_SIZE         4095
#define MAP_ENTRY_FLAG_TYPE_MASK 0x0f
#define MAP_ENTRY_FLAG_NO_CRC    0x10
#define CHD_V1_SECTOR_SIZE       512
#define COOKIE_VALUE             0xbaadf00d
#define MAX_ZLIB_ALLOCS          64
#define END_OF_LIST_COOKIE       "EndOfListCookie"
#define NO_MATCH                 (~0)
static const uint8_t  s_cd_sync_header[12] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
typedef unsigned char BYTE;    // 8-bit byte
typedef unsigned int  WORD;    // 32-bit word, change to "long" for 16-bit machines
typedef struct
{
    BYTE               data[64];
    WORD               datalen;
    unsigned long long bitlen;
    WORD               state[5];
    WORD               k[4];
} SHA1_CTX;
enum
{
    V34_MAP_ENTRY_TYPE_INVALID        = 0,
    V34_MAP_ENTRY_TYPE_COMPRESSED     = 1,
    V34_MAP_ENTRY_TYPE_UNCOMPRESSED   = 2,
    V34_MAP_ENTRY_TYPE_MINI           = 3,
    V34_MAP_ENTRY_TYPE_SELF_HUNK      = 4,
    V34_MAP_ENTRY_TYPE_PARENT_HUNK    = 5,
    V34_MAP_ENTRY_TYPE_2ND_COMPRESSED = 6
};
enum
{
    COMPRESSION_TYPE_0 = 0,
    COMPRESSION_TYPE_1 = 1,
    COMPRESSION_TYPE_2 = 2,
    COMPRESSION_TYPE_3 = 3,
    COMPRESSION_NONE   = 4,
    COMPRESSION_SELF   = 5,
    COMPRESSION_PARENT = 6,
    COMPRESSION_RLE_SMALL,
    COMPRESSION_RLE_LARGE,
    COMPRESSION_SELF_0,
    COMPRESSION_SELF_1,
    COMPRESSION_PARENT_SELF,
    COMPRESSION_PARENT_0,
    COMPRESSION_PARENT_1
};
#define EARLY_EXIT(x)                                                                                                  \
    do {                                                                                                               \
        (void)(x);                                                                                                     \
        goto cleanup;                                                                                                  \
    } while (0)
typedef struct _codec_interface codec_interface;
struct _codec_interface
{
    UINT32      compression;
    const char *compname;
    UINT8       lossy;
    chd_error (*init)(void *codec, UINT32 hunkbytes);
    void (*free)(void *codec);
    chd_error (*decompress)(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest, uint32_t destlen);
    chd_error (*config)(void *codec, int param, void *config);
};
typedef struct _map_entry map_entry;
struct _map_entry
{
    UINT64 offset;
    UINT32 crc;
    UINT32 length;
    UINT8  flags;
};
typedef struct _crcmap_entry crcmap_entry;
struct _crcmap_entry
{
    UINT32        hunknum;
    crcmap_entry *next;
};
typedef struct _metadata_entry metadata_entry;
struct _metadata_entry
{
    UINT64 offset;
    UINT64 next;
    UINT64 prev;
    UINT32 length;
    UINT32 metatag;
    UINT8  flags;
};
typedef struct _zlib_allocator zlib_allocator;
struct _zlib_allocator
{
    UINT32 *allocptr[MAX_ZLIB_ALLOCS];
    UINT32 *allocptr2[MAX_ZLIB_ALLOCS];
};
typedef struct _zlib_codec_data zlib_codec_data;
struct _zlib_codec_data
{
    z_stream       inflater;
    zlib_allocator allocator;
};
typedef struct _cdzl_codec_data cdzl_codec_data;
struct _cdzl_codec_data
{
    zlib_codec_data base_decompressor;
    zlib_codec_data subcode_decompressor;
    uint8_t        *buffer;
};
typedef struct _cdlz_codec_data cdlz_codec_data;
struct _cdlz_codec_data
{
    zlib_codec_data subcode_decompressor;
    uint8_t        *buffer;
};
typedef struct _cdfl_codec_data cdfl_codec_data;
struct _cdfl_codec_data
{
    int swap_endian;
    // flac_decoder   decoder;
    z_stream       inflater;
    zlib_allocator allocator;
    uint8_t       *buffer;
};
struct _chd_file
{
    UINT32                 cookie;
    core_file             *file;
    UINT8                  owns_file;
    chd_header             header;
    chd_file              *parent;
    map_entry             *map;
    UINT8                 *cache;
    UINT32                 cachehunk;
    UINT8                 *compare;
    UINT32                 comparehunk;
    UINT8                 *compressed;
    const codec_interface *codecintf[4];
    zlib_codec_data        zlib_codec_data;
    cdzl_codec_data        cdzl_codec_data;
    cdlz_codec_data        cdlz_codec_data;
    cdfl_codec_data        cdfl_codec_data;
    crcmap_entry          *crcmap;
    crcmap_entry          *crcfree;
    crcmap_entry         **crctable;
    UINT32                 maxhunk;
    UINT8                  compressing;
    MD5_CTX                compmd5;
    SHA1_CTX               compsha1;
    UINT32                 comphunk;
    UINT8                  verifying;
    MD5_CTX                vermd5;
    SHA1_CTX               versha1;
    UINT32                 verhunk;
    UINT32                 async_hunknum;
    void                  *async_buffer;
};
typedef struct _metadata_hash metadata_hash;
struct _metadata_hash
{
    UINT8 tag[4];
    UINT8 sha1[CHD_SHA1_BYTES];
};
static const UINT8 nullmd5[CHD_MD5_BYTES]   = {0};
static const UINT8 nullsha1[CHD_SHA1_BYTES] = {0};
static chd_error   header_validate(const chd_header *header);
static chd_error   header_read(chd_file *chd, chd_header *header);
static chd_error   hunk_read_into_cache(chd_file *chd, UINT32 hunknum);
static chd_error   hunk_read_into_memory(chd_file *chd, UINT32 hunknum, UINT8 *dest);
static chd_error   map_read(chd_file *chd);
static chd_error   metadata_find_entry(chd_file *chd, UINT32 metatag, UINT32 metaindex, metadata_entry *metaentry);
static chd_error   zlib_codec_init(void *codec, uint32_t hunkbytes);
static void        zlib_codec_free(void *codec);
static chd_error   zlib_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest,
                                         uint32_t destlen);
static voidpf      zlib_fast_alloc(voidpf opaque, uInt items, uInt size);
static void        zlib_fast_free(voidpf opaque, voidpf address);
static chd_error   cdzl_codec_init(void *codec, uint32_t hunkbytes);
static void        cdzl_codec_free(void *codec);
static chd_error   cdzl_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest,
                                         uint32_t destlen);
static chd_error   cdlz_codec_init(void *codec, uint32_t hunkbytes);
static void        cdlz_codec_free(void *codec);
static chd_error   cdlz_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest,
                                         uint32_t destlen);
static chd_error   cdfl_codec_init(void *codec, uint32_t hunkbytes);
static void        cdfl_codec_free(void *codec);
// static chd_error cdfl_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest,
//                                        uint32_t destlen);
chd_error cdlz_codec_init(void *codec, uint32_t hunkbytes)
{
    cdlz_codec_data *cdlz = (cdlz_codec_data *)codec;
    cdlz->buffer          = (uint8_t *)malloc(sizeof(uint8_t) * hunkbytes);
    // lzma_codec_init(&cdlz->base_decompressor, (hunkbytes / CD_FRAME_SIZE) * CD_MAX_SECTOR_DATA);
    zlib_codec_init(&cdlz->subcode_decompressor, (hunkbytes / CD_FRAME_SIZE) * CD_MAX_SUBCODE_DATA);
    if (hunkbytes % CD_FRAME_SIZE != 0)
        return CHDERR_CODEC_ERROR;
    return CHDERR_NONE;
}
void cdlz_codec_free(void *codec)
{
}
chd_error cdlz_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest, uint32_t destlen)
{
    uint8_t         *sector;
    cdlz_codec_data *cdlz          = (cdlz_codec_data *)codec;
    uint32_t         frames        = destlen / CD_FRAME_SIZE;
    uint32_t         complen_bytes = (destlen < 65536) ? 2 : 3;
    uint32_t         ecc_bytes     = (frames + 7) / 8;
    uint32_t         header_bytes  = ecc_bytes + complen_bytes;
    uint32_t         complen_base  = (src[ecc_bytes + 0] << 8) | src[ecc_bytes + 1];
    if (complen_bytes > 2)
        complen_base = (complen_base << 8) | src[ecc_bytes + 2];
    // lzma_codec_decompress(&cdlz->base_decompressor, &src[header_bytes], complen_base, &cdlz->buffer[0],
    //                       frames * CD_MAX_SECTOR_DATA);
    zlib_codec_decompress(&cdlz->subcode_decompressor, &src[header_bytes + complen_base],
                          complen - complen_base - header_bytes, &cdlz->buffer[frames * CD_MAX_SECTOR_DATA],
                          frames * CD_MAX_SUBCODE_DATA);
    for (uint32_t framenum = 0; framenum < frames; framenum++) {
        memcpy(&dest[framenum * CD_FRAME_SIZE], &cdlz->buffer[framenum * CD_MAX_SECTOR_DATA], CD_MAX_SECTOR_DATA);
        memcpy(&dest[framenum * CD_FRAME_SIZE + CD_MAX_SECTOR_DATA],
               &cdlz->buffer[frames * CD_MAX_SECTOR_DATA + framenum * CD_MAX_SUBCODE_DATA], CD_MAX_SUBCODE_DATA);
        sector = (uint8_t *)&dest[framenum * CD_FRAME_SIZE];
        if ((src[framenum / 8] & (1 << (framenum % 8))) != 0) {
            memcpy(sector, s_cd_sync_header, sizeof(s_cd_sync_header));
            ecc_generate(sector);
        }
    }
    return CHDERR_NONE;
}
chd_error cdzl_codec_init(void *codec, uint32_t hunkbytes)
{
    cdzl_codec_data *cdzl = (cdzl_codec_data *)codec;
    zlib_codec_init(&cdzl->base_decompressor, (hunkbytes / CD_FRAME_SIZE) * CD_MAX_SECTOR_DATA);
    zlib_codec_init(&cdzl->subcode_decompressor, (hunkbytes / CD_FRAME_SIZE) * CD_MAX_SUBCODE_DATA);
    cdzl->buffer = (uint8_t *)malloc(sizeof(uint8_t) * hunkbytes);
    if (hunkbytes % CD_FRAME_SIZE != 0)
        return CHDERR_CODEC_ERROR;
    return CHDERR_NONE;
}
void cdzl_codec_free(void *codec)
{
}
chd_error cdzl_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest, uint32_t destlen)
{
    uint8_t         *sector;
    cdzl_codec_data *cdzl          = (cdzl_codec_data *)codec;
    uint32_t         frames        = destlen / CD_FRAME_SIZE;
    uint32_t         complen_bytes = (destlen < 65536) ? 2 : 3;
    uint32_t         ecc_bytes     = (frames + 7) / 8;
    uint32_t         header_bytes  = ecc_bytes + complen_bytes;
    uint32_t         complen_base  = (src[ecc_bytes + 0] << 8) | src[ecc_bytes + 1];
    if (complen_bytes > 2)
        complen_base = (complen_base << 8) | src[ecc_bytes + 2];
    zlib_codec_decompress(&cdzl->base_decompressor, &src[header_bytes], complen_base, &cdzl->buffer[0],
                          frames * CD_MAX_SECTOR_DATA);
    zlib_codec_decompress(&cdzl->subcode_decompressor, &src[header_bytes + complen_base],
                          complen - complen_base - header_bytes, &cdzl->buffer[frames * CD_MAX_SECTOR_DATA],
                          frames * CD_MAX_SUBCODE_DATA);
    for (uint32_t framenum = 0; framenum < frames; framenum++) {
        memcpy(&dest[framenum * CD_FRAME_SIZE], &cdzl->buffer[framenum * CD_MAX_SECTOR_DATA], CD_MAX_SECTOR_DATA);
        memcpy(&dest[framenum * CD_FRAME_SIZE + CD_MAX_SECTOR_DATA],
               &cdzl->buffer[frames * CD_MAX_SECTOR_DATA + framenum * CD_MAX_SUBCODE_DATA], CD_MAX_SUBCODE_DATA);
        sector = (uint8_t *)&dest[framenum * CD_FRAME_SIZE];
        if ((src[framenum / 8] & (1 << (framenum % 8))) != 0) {
            memcpy(sector, s_cd_sync_header, sizeof(s_cd_sync_header));
            ecc_generate(sector);
        }
    }
    return CHDERR_NONE;
}
static uint32_t cdfl_codec_blocksize(uint32_t bytes)
{
    uint32_t hunkbytes = bytes / 4;
    while (hunkbytes > 2048)
        hunkbytes /= 2;
    return hunkbytes;
}
chd_error cdfl_codec_init(void *codec, uint32_t hunkbytes)
{
    cdfl_codec_data *cdfl = (cdfl_codec_data *)codec;
    cdfl->buffer          = (uint8_t *)malloc(sizeof(uint8_t) * hunkbytes);
    if (hunkbytes % CD_FRAME_SIZE != 0)
        return CHDERR_CODEC_ERROR;
    uint16_t native_endian       = 0;
    *(uint8_t *)(&native_endian) = 1;
    cdfl->swap_endian            = (native_endian & 1);
    cdfl->inflater.next_in       = (Bytef *)cdfl;
    cdfl->inflater.avail_in      = 0;
#if 0
	cdfl->allocator.install(cdfl->inflater);
#endif
    cdfl->inflater.zalloc = zlib_fast_alloc;
    cdfl->inflater.zfree  = zlib_fast_free;
    cdfl->inflater.opaque = &cdfl->allocator;
    int zerr              = inflateInit2(&cdfl->inflater, -MAX_WBITS);
    if (zerr == Z_MEM_ERROR)
        return CHDERR_OUT_OF_MEMORY;
    else if (zerr != Z_OK)
        return CHDERR_CODEC_ERROR;
    // flac_decoder_init(&cdfl->decoder);
    return CHDERR_NONE;
}
void cdfl_codec_free(void *codec)
{
    cdfl_codec_data *cdfl = (cdfl_codec_data *)codec;
    inflateEnd(&cdfl->inflater);
}
// chd_error cdfl_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest, uint32_t destlen)
// {
//     cdfl_codec_data *cdfl = (cdfl_codec_data *)codec;
//
//     uint32_t frames = destlen / CD_FRAME_SIZE;
//     // if (!flac_decoder_reset(&cdfl->decoder, 44100, 2, cdfl_codec_blocksize(frames * CD_MAX_SECTOR_DATA), src,
//     // complen))
//     //     return CHDERR_DECOMPRESSION_ERROR;
//     // uint8_t *buffer = &cdfl->buffer[0];
//     // if (!flac_decoder_decode_interleaved(&cdfl->decoder, (int16_t *)(buffer), frames * CD_MAX_SECTOR_DATA / 4,
//     //                                      cdfl->swap_endian))
//     //     return CHDERR_DECOMPRESSION_ERROR;
//
//     uint32_t offset          = flac_decoder_finish(&cdfl->decoder);
//     cdfl->inflater.next_in   = (Bytef *)(src + offset);
//     cdfl->inflater.avail_in  = complen - offset;
//     cdfl->inflater.total_in  = 0;
//     cdfl->inflater.next_out  = &cdfl->buffer[frames * CD_MAX_SECTOR_DATA];
//     cdfl->inflater.avail_out = frames * CD_MAX_SUBCODE_DATA;
//     cdfl->inflater.total_out = 0;
//     int zerr                 = inflateReset(&cdfl->inflater);
//     if (zerr != Z_OK)
//         return CHDERR_DECOMPRESSION_ERROR;
//
//     zerr = inflate(&cdfl->inflater, Z_FINISH);
//     if (zerr != Z_STREAM_END)
//         return CHDERR_DECOMPRESSION_ERROR;
//     if (cdfl->inflater.total_out != frames * CD_MAX_SUBCODE_DATA)
//         return CHDERR_DECOMPRESSION_ERROR;
//
//     for (uint32_t framenum = 0; framenum < frames; framenum++) {
//         memcpy(&dest[framenum * CD_FRAME_SIZE], &cdfl->buffer[framenum * CD_MAX_SECTOR_DATA], CD_MAX_SECTOR_DATA);
//         memcpy(&dest[framenum * CD_FRAME_SIZE + CD_MAX_SECTOR_DATA],
//                &cdfl->buffer[frames * CD_MAX_SECTOR_DATA + framenum * CD_MAX_SUBCODE_DATA], CD_MAX_SUBCODE_DATA);
//     }
//     return CHDERR_NONE;
// }
static const codec_interface codec_interfaces[] = {
    {CHDCOMPRESSION_NONE, "none", FALSE, NULL, NULL, NULL, NULL},
    {CHDCOMPRESSION_ZLIB, "zlib", FALSE, zlib_codec_init, zlib_codec_free, zlib_codec_decompress, NULL},
    {CHDCOMPRESSION_ZLIB_PLUS, "zlib+", FALSE, zlib_codec_init, zlib_codec_free, zlib_codec_decompress, NULL},
    {CHD_CODEC_ZLIB, "zlib (Deflate)", FALSE, zlib_codec_init, zlib_codec_free, zlib_codec_decompress, NULL},
    {CHD_CODEC_CD_ZLIB, "cdzl (CD Deflate)", FALSE, cdzl_codec_init, cdzl_codec_free, cdzl_codec_decompress, NULL},
    //
    // {CHD_CODEC_CD_LZMA, "cdlz (CD LZMA)", FALSE, cdlz_codec_init, cdlz_codec_free, cdlz_codec_decompress, NULL},
    // {CHD_CODEC_CD_FLAC, "cdfl (CD FLAC)", FALSE, cdfl_codec_init, cdfl_codec_free, cdfl_codec_decompress, NULL},
};
static inline UINT64 get_bigendian_uint64(const UINT8 *base)
{
    return ((UINT64)base[0] << 56) | ((UINT64)base[1] << 48) | ((UINT64)base[2] << 40) | ((UINT64)base[3] << 32) |
           ((UINT64)base[4] << 24) | ((UINT64)base[5] << 16) | ((UINT64)base[6] << 8) | (UINT64)base[7];
}
static inline void put_bigendian_uint64(UINT8 *base, UINT64 value)
{
    base[0] = value >> 56;
    base[1] = value >> 48;
    base[2] = value >> 40;
    base[3] = value >> 32;
    base[4] = value >> 24;
    base[5] = value >> 16;
    base[6] = value >> 8;
    base[7] = value;
}
static inline UINT64 get_bigendian_uint48(const UINT8 *base)
{
    return ((UINT64)base[0] << 40) | ((UINT64)base[1] << 32) | ((UINT64)base[2] << 24) | ((UINT64)base[3] << 16) |
           ((UINT64)base[4] << 8) | (UINT64)base[5];
}
static inline void put_bigendian_uint48(UINT8 *base, UINT64 value)
{
    value &= 0xffffffffffff;
    base[0] = value >> 40;
    base[1] = value >> 32;
    base[2] = value >> 24;
    base[3] = value >> 16;
    base[4] = value >> 8;
    base[5] = value;
}
static inline UINT32 get_bigendian_uint32(const UINT8 *base)
{
    return (base[0] << 24) | (base[1] << 16) | (base[2] << 8) | base[3];
}
static inline void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
    base[0] = value >> 24;
    base[1] = value >> 16;
    base[2] = value >> 8;
    base[3] = value;
}
static inline void put_bigendian_uint24(UINT8 *base, UINT32 value)
{
    value &= 0xffffff;
    base[0] = value >> 16;
    base[1] = value >> 8;
    base[2] = value;
}
static inline UINT32 get_bigendian_uint24(const UINT8 *base)
{
    return (base[0] << 16) | (base[1] << 8) | base[2];
}
static inline UINT16 get_bigendian_uint16(const UINT8 *base)
{
    return (base[0] << 8) | base[1];
}
static inline void put_bigendian_uint16(UINT8 *base, UINT16 value)
{
    base[0] = value >> 8;
    base[1] = value;
}
static inline void map_extract(const UINT8 *base, map_entry *entry)
{
    entry->offset = get_bigendian_uint64(&base[0]);
    entry->crc    = get_bigendian_uint32(&base[8]);
    entry->length = get_bigendian_uint16(&base[12]) | (base[14] << 16);
    entry->flags  = base[15];
}
static inline void map_assemble(UINT8 *base, map_entry *entry)
{
    put_bigendian_uint64(&base[0], entry->offset);
    put_bigendian_uint32(&base[8], entry->crc);
    put_bigendian_uint16(&base[12], entry->length);
    base[14] = entry->length >> 16;
    base[15] = entry->flags;
}
static inline int map_size_v5(chd_header *header)
{
    return header->hunkcount * header->mapentrybytes;
}
uint16_t crc16(const void *data, uint32_t length)
{
    uint16_t              crc          = 0xffff;
    static const uint16_t s_table[256] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
        0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a,
        0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
        0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
        0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
        0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87,
        0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
        0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
        0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
        0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
        0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f,
        0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
        0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
        0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
        0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};
    const uint8_t *src = (uint8_t *)data;
    while (length-- != 0)
        crc = (crc << 8) ^ s_table[(crc >> 8) ^ *src++];
    return crc;
}
static inline int compressed(chd_header *header)
{
    return header->compression[0] != CHD_CODEC_NONE;
}
static chd_error decompress_v5_map(chd_file *chd, chd_header *header)
{
    int rawmapsize = map_size_v5(header);
    if (!compressed(header)) {
        header->rawmap = (uint8_t *)malloc(rawmapsize);
        core_fseek(chd->file, header->mapoffset, SEEK_SET);
        core_fread(chd->file, header->rawmap, rawmapsize);
        return CHDERR_NONE;
    }
    uint8_t rawbuf[16];
    core_fseek(chd->file, header->mapoffset, SEEK_SET);
    core_fread(chd->file, rawbuf, sizeof(rawbuf));
    uint32_t const mapbytes   = get_bigendian_uint32(&rawbuf[0]);
    uint64_t const firstoffs  = get_bigendian_uint48(&rawbuf[4]);
    uint16_t const mapcrc     = get_bigendian_uint16(&rawbuf[10]);
    uint8_t const  lengthbits = rawbuf[12];
    uint8_t const  selfbits   = rawbuf[13];
    uint8_t const  parentbits = rawbuf[14];
    uint8_t       *compressed = (uint8_t *)malloc(sizeof(uint8_t) * mapbytes);
    core_fseek(chd->file, header->mapoffset + 16, SEEK_SET);
    core_fread(chd->file, compressed, mapbytes);
    struct bitstream *bitbuf        = create_bitstream(compressed, sizeof(uint8_t) * mapbytes);
    header->rawmap                  = (uint8_t *)malloc(rawmapsize);
    struct huffman_decoder *decoder = create_huffman_decoder(16, 8);
    enum huffman_error      err     = huffman_import_tree_rle(decoder, bitbuf);
    if (err != HUFFERR_NONE)
        return CHDERR_DECOMPRESSION_ERROR;
    uint8_t lastcomp = 0;
    int     repcount = 0;
    for (int hunknum = 0; hunknum < header->hunkcount; hunknum++) {
        uint8_t *rawmap = header->rawmap + (hunknum * 12);
        if (repcount > 0)
            rawmap[0] = lastcomp, repcount--;
        else {
            uint8_t val = huffman_decode_one(decoder, bitbuf);
            if (val == COMPRESSION_RLE_SMALL)
                rawmap[0] = lastcomp, repcount = 2 + huffman_decode_one(decoder, bitbuf);
            else if (val == COMPRESSION_RLE_LARGE)
                rawmap[0] = lastcomp, repcount = 2 + 16 + (huffman_decode_one(decoder, bitbuf) << 4),
                repcount += huffman_decode_one(decoder, bitbuf);
            else
                rawmap[0] = lastcomp = val;
        }
    }
    uint64_t curoffset   = firstoffs;
    uint32_t last_self   = 0;
    uint64_t last_parent = 0;
    for (int hunknum = 0; hunknum < header->hunkcount; hunknum++) {
        uint8_t *rawmap = header->rawmap + (hunknum * 12);
        uint64_t offset = curoffset;
        uint32_t length = 0;
        uint16_t crc    = 0;
        switch (rawmap[0]) {
            case COMPRESSION_TYPE_0:
            case COMPRESSION_TYPE_1:
            case COMPRESSION_TYPE_2:
            case COMPRESSION_TYPE_3:
                curoffset += length = bitstream_read(bitbuf, lengthbits);
                crc                 = bitstream_read(bitbuf, 16);
                break;
            case COMPRESSION_NONE:
                curoffset += length = header->hunkbytes;
                crc                 = bitstream_read(bitbuf, 16);
                break;
            case COMPRESSION_SELF:
                last_self = offset = bitstream_read(bitbuf, selfbits);
                break;
            case COMPRESSION_PARENT:
                offset      = bitstream_read(bitbuf, parentbits);
                last_parent = offset;
                break;
            case COMPRESSION_SELF_1:
                last_self++;
            case COMPRESSION_SELF_0:
                rawmap[0] = COMPRESSION_SELF;
                offset    = last_self;
                break;
            case COMPRESSION_PARENT_SELF:
                rawmap[0]   = COMPRESSION_PARENT;
                last_parent = offset = (((uint64_t)hunknum) * ((uint64_t)header->hunkbytes)) / header->unitbytes;
                break;
            case COMPRESSION_PARENT_1:
                last_parent += header->hunkbytes / header->unitbytes;
            case COMPRESSION_PARENT_0:
                rawmap[0] = COMPRESSION_PARENT;
                offset    = last_parent;
                break;
        }
        put_bigendian_uint24(&rawmap[1], length);
        put_bigendian_uint48(&rawmap[4], offset);
        put_bigendian_uint16(&rawmap[10], crc);
    }
    if (crc16(&header->rawmap[0], header->hunkcount * 12) != mapcrc)
        return CHDERR_DECOMPRESSION_ERROR;
    return CHDERR_NONE;
}
static inline void map_extract_old(const UINT8 *base, map_entry *entry, UINT32 hunkbytes)
{
    entry->offset = get_bigendian_uint64(&base[0]);
    entry->crc    = 0;
    entry->length = entry->offset >> 44;
    entry->flags  = MAP_ENTRY_FLAG_NO_CRC |
                   ((entry->length == hunkbytes) ? V34_MAP_ENTRY_TYPE_UNCOMPRESSED : V34_MAP_ENTRY_TYPE_COMPRESSED);
#ifdef __MWERKS__
    entry->offset = entry->offset & 0x00000FFFFFFFFFFFLL;
#else
    entry->offset = (entry->offset << 20) >> 20;
#endif
}
chd_error chd_open_file(core_file *file, int mode, chd_file *parent, chd_file **chd)
{
    chd_file *newchd = NULL;
    chd_error err;
    int       intfnum;
    if (file == NULL)
        EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
    if (parent != NULL && parent->cookie != COOKIE_VALUE)
        EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
    newchd = (chd_file *)malloc(sizeof(**chd));
    if (newchd == NULL)
        EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);
    memset(newchd, 0, sizeof(*newchd));
    newchd->cookie = COOKIE_VALUE;
    newchd->parent = parent;
    newchd->file   = file;
    err            = header_read(newchd, &newchd->header);
    if (err != CHDERR_NONE)
        EARLY_EXIT(err);
    err = header_validate(&newchd->header);
    if (err != CHDERR_NONE)
        EARLY_EXIT(err);
    if (mode == CHD_OPEN_READWRITE && !(newchd->header.flags & CHDFLAGS_IS_WRITEABLE))
        EARLY_EXIT(err = CHDERR_FILE_NOT_WRITEABLE);
    if (mode == CHD_OPEN_READWRITE && newchd->header.version < CHD_HEADER_VERSION)
        EARLY_EXIT(err = CHDERR_UNSUPPORTED_VERSION);
    if (parent == NULL && (newchd->header.flags & CHDFLAGS_HAS_PARENT))
        EARLY_EXIT(err = CHDERR_REQUIRES_PARENT);
    if (parent != NULL) {
        if (memcmp(nullmd5, newchd->header.parentmd5, sizeof(newchd->header.parentmd5)) != 0 &&
            memcmp(nullmd5, newchd->parent->header.md5, sizeof(newchd->parent->header.md5)) != 0 &&
            memcmp(newchd->parent->header.md5, newchd->header.parentmd5, sizeof(newchd->header.parentmd5)) != 0)
            EARLY_EXIT(err = CHDERR_INVALID_PARENT);
        if (memcmp(nullsha1, newchd->header.parentsha1, sizeof(newchd->header.parentsha1)) != 0 &&
            memcmp(nullsha1, newchd->parent->header.sha1, sizeof(newchd->parent->header.sha1)) != 0 &&
            memcmp(newchd->parent->header.sha1, newchd->header.parentsha1, sizeof(newchd->header.parentsha1)) != 0)
            EARLY_EXIT(err = CHDERR_INVALID_PARENT);
    }
    if (newchd->header.version < 5) {
        err = map_read(newchd);
    } else {
        err = decompress_v5_map(newchd, &(newchd->header));
    }
    if (err != CHDERR_NONE)
        EARLY_EXIT(err);
    newchd->cache   = (UINT8 *)malloc(newchd->header.hunkbytes);
    newchd->compare = (UINT8 *)malloc(newchd->header.hunkbytes);
    if (newchd->cache == NULL || newchd->compare == NULL)
        EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);
    newchd->cachehunk   = ~0;
    newchd->comparehunk = ~0;
    newchd->compressed  = (UINT8 *)malloc(newchd->header.hunkbytes);
    if (newchd->compressed == NULL)
        EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);
    if (newchd->header.version < 5) {
        for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++) {
            if (codec_interfaces[intfnum].compression == newchd->header.compression[0]) {
                newchd->codecintf[0] = &codec_interfaces[intfnum];
                break;
            }
        }
        if (intfnum == ARRAY_LENGTH(codec_interfaces))
            EARLY_EXIT(err = CHDERR_UNSUPPORTED_FORMAT);
        if (newchd->codecintf[0]->init != NULL) {
            err = (*newchd->codecintf[0]->init)(&newchd->zlib_codec_data, newchd->header.hunkbytes);
            if (err != CHDERR_NONE)
                EARLY_EXIT(err);
        }
    } else {
        for (int decompnum = 0; decompnum < ARRAY_LENGTH(newchd->header.compression); decompnum++) {
            for (int i = 0; i < ARRAY_LENGTH(codec_interfaces); i++) {
                if (codec_interfaces[i].compression == newchd->header.compression[decompnum]) {
                    newchd->codecintf[decompnum] = &codec_interfaces[i];
                    break;
                }
            }
            if (newchd->codecintf[decompnum] == NULL && newchd->header.compression[decompnum] != 0)
                EARLY_EXIT(err = CHDERR_UNSUPPORTED_FORMAT);
            if (newchd->codecintf[decompnum]->init != NULL) {
                void *codec = NULL;
                switch (newchd->header.compression[decompnum]) {
                    case CHD_CODEC_ZLIB:
                        codec = &newchd->zlib_codec_data;
                        break;
                    case CHD_CODEC_CD_ZLIB:
                        codec = &newchd->cdzl_codec_data;
                        break;
                        // case CHD_CODEC_CD_LZMA:
                        //     codec = &newchd->cdlz_codec_data;
                        //     break;
                        // case CHD_CODEC_CD_FLAC:
                        //     codec = &newchd->cdfl_codec_data;
                        //     break;
                }
                if (codec == NULL)
                    EARLY_EXIT(err = CHDERR_UNSUPPORTED_FORMAT);
                err = (*newchd->codecintf[decompnum]->init)(codec, newchd->header.hunkbytes);
                if (err != CHDERR_NONE)
                    EARLY_EXIT(err);
            }
        }
    }
    *chd = newchd;
    return CHDERR_NONE;
cleanup:
    if (newchd != NULL)
        chd_close(newchd);
    return err;
}
chd_error chd_open(const char *filename, int mode, chd_file *parent, chd_file **chd)
{
    chd_error  err;
    core_file *file = NULL;
    UINT32     openflags;
    switch (mode) {
        case CHD_OPEN_READ:
            break;
        default:
            err = CHDERR_INVALID_PARAMETER;
            goto cleanup;
    }
    file = core_fopen(filename);
    if (file == 0) {
        err = CHDERR_FILE_NOT_FOUND;
        goto cleanup;
    }
    err = chd_open_file(file, mode, parent, chd);
    if (err != CHDERR_NONE)
        goto cleanup;
    (*chd)->owns_file = TRUE;
cleanup:
    if ((err != CHDERR_NONE) && (file != NULL))
        core_fclose(file);
    return err;
}
void chd_close(chd_file *chd)
{
    if (chd == NULL || chd->cookie != COOKIE_VALUE)
        return;
    if (chd->header.version < 5) {
        if (chd->codecintf[0] != NULL && chd->codecintf[0]->free != NULL)
            (*chd->codecintf[0]->free)(&chd->zlib_codec_data);
    } else {
        for (int i = 0; i < ARRAY_LENGTH(chd->codecintf); i++) {
            void *codec = NULL;
            if (chd->codecintf[i] == NULL)
                continue;
            switch (chd->codecintf[i]->compression) {
                    // case CHD_CODEC_CD_LZMA:
                    //     codec = &chd->cdlz_codec_data;
                    //     break;
                case CHD_CODEC_ZLIB:
                    codec = &chd->zlib_codec_data;
                    break;
                case CHD_CODEC_CD_ZLIB:
                    codec = &chd->cdzl_codec_data;
                    break;
                    // case CHD_CODEC_CD_FLAC:
                    //     codec = &chd->cdfl_codec_data;
                    //     break;
            }
            if (codec) {
                (*chd->codecintf[i]->free)(codec);
            }
        }
        if (chd->header.rawmap != NULL)
            free(chd->header.rawmap);
    }
    if (chd->compressed != NULL)
        free(chd->compressed);
    if (chd->compare != NULL)
        free(chd->compare);
    if (chd->cache != NULL)
        free(chd->cache);
    if (chd->map != NULL)
        free(chd->map);
    if (chd->crctable != NULL)
        free(chd->crctable);
    if (chd->crcmap != NULL)
        free(chd->crcmap);
    if (chd->owns_file && chd->file != NULL)
        core_fclose(chd->file);
    if (PRINTF_MAX_HUNK)
        printf("Max hunk = %d/%d\n", chd->maxhunk, chd->header.totalhunks);
    free(chd);
}
core_file *chd_core_file(chd_file *chd)
{
    return chd->file;
}
const char *chd_error_string(chd_error err)
{
    switch (err) {
        case CHDERR_NONE:
            return "no error";
        case CHDERR_NO_INTERFACE:
            return "no drive interface";
        case CHDERR_OUT_OF_MEMORY:
            return "out of memory";
        case CHDERR_INVALID_FILE:
            return "invalid file";
        case CHDERR_INVALID_PARAMETER:
            return "invalid parameter";
        case CHDERR_INVALID_DATA:
            return "invalid data";
        case CHDERR_FILE_NOT_FOUND:
            return "file not found";
        case CHDERR_REQUIRES_PARENT:
            return "requires parent";
        case CHDERR_FILE_NOT_WRITEABLE:
            return "file not writeable";
        case CHDERR_READ_ERROR:
            return "read error";
        case CHDERR_WRITE_ERROR:
            return "write error";
        case CHDERR_CODEC_ERROR:
            return "codec error";
        case CHDERR_INVALID_PARENT:
            return "invalid parent";
        case CHDERR_HUNK_OUT_OF_RANGE:
            return "hunk out of range";
        case CHDERR_DECOMPRESSION_ERROR:
            return "decompression error";
        case CHDERR_COMPRESSION_ERROR:
            return "compression error";
        case CHDERR_CANT_CREATE_FILE:
            return "can't create file";
        case CHDERR_CANT_VERIFY:
            return "can't verify file";
        case CHDERR_NOT_SUPPORTED:
            return "operation not supported";
        case CHDERR_METADATA_NOT_FOUND:
            return "can't find metadata";
        case CHDERR_INVALID_METADATA_SIZE:
            return "invalid metadata size";
        case CHDERR_UNSUPPORTED_VERSION:
            return "unsupported CHD version";
        case CHDERR_VERIFY_INCOMPLETE:
            return "incomplete verify";
        case CHDERR_INVALID_METADATA:
            return "invalid metadata";
        case CHDERR_INVALID_STATE:
            return "invalid state";
        case CHDERR_OPERATION_PENDING:
            return "operation pending";
        case CHDERR_NO_ASYNC_OPERATION:
            return "no async operation in progress";
        case CHDERR_UNSUPPORTED_FORMAT:
            return "unsupported format";
        default:
            return "undocumented error";
    }
}
const chd_header *chd_get_header(chd_file *chd)
{
    if (chd == NULL || chd->cookie != COOKIE_VALUE)
        return NULL;
    return &chd->header;
}
chd_error chd_read(chd_file *chd, UINT32 hunknum, void *buffer)
{
    if (chd == NULL || chd->cookie != COOKIE_VALUE)
        return CHDERR_INVALID_PARAMETER;
    if (hunknum >= chd->header.totalhunks)
        return CHDERR_HUNK_OUT_OF_RANGE;
    return hunk_read_into_memory(chd, hunknum, (UINT8 *)buffer);
}
chd_error chd_get_metadata(chd_file *chd, UINT32 searchtag, UINT32 searchindex, void *output, UINT32 outputlen,
                           UINT32 *resultlen, UINT32 *resulttag, UINT8 *resultflags)
{
    metadata_entry metaentry;
    chd_error      err;
    UINT32         count;
    err = metadata_find_entry(chd, searchtag, searchindex, &metaentry);
    if (err != CHDERR_NONE) {
        if (chd->header.version < 3 && (searchtag == HARD_DISK_METADATA_TAG || searchtag == CHDMETATAG_WILDCARD) &&
            searchindex == 0) {
            char   faux_metadata[256];
            UINT32 faux_length;
            sprintf(faux_metadata, HARD_DISK_METADATA_FORMAT, chd->header.obsolete_cylinders,
                    chd->header.obsolete_heads, chd->header.obsolete_sectors,
                    chd->header.hunkbytes / chd->header.obsolete_hunksize);
            faux_length = (UINT32)strlen(faux_metadata) + 1;
            memcpy(output, faux_metadata, MIN(outputlen, faux_length));
            if (resultlen != NULL)
                *resultlen = faux_length;
            if (resulttag != NULL)
                *resulttag = HARD_DISK_METADATA_TAG;
            return CHDERR_NONE;
        }
        return err;
    }
    outputlen = MIN(outputlen, metaentry.length);
    core_fseek(chd->file, metaentry.offset + METADATA_HEADER_SIZE, SEEK_SET);
    count = core_fread(chd->file, output, outputlen);
    if (count != outputlen)
        return CHDERR_READ_ERROR;
    if (resultlen != NULL)
        *resultlen = metaentry.length;
    if (resulttag != NULL)
        *resulttag = metaentry.metatag;
    if (resultflags != NULL)
        *resultflags = metaentry.flags;
    return CHDERR_NONE;
}
chd_error chd_codec_config(chd_file *chd, int param, void *config)
{
    return CHDERR_INVALID_PARAMETER;
}
const char *chd_get_codec_name(UINT32 codec)
{
    return "Unknown";
}
static chd_error header_validate(const chd_header *header)
{
    int intfnum;
    if (header->version == 0 || header->version > CHD_HEADER_VERSION)
        return CHDERR_UNSUPPORTED_VERSION;
    if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
        (header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
        (header->version == 3 && header->length != CHD_V3_HEADER_SIZE) ||
        (header->version == 4 && header->length != CHD_V4_HEADER_SIZE) ||
        (header->version == 5 && header->length != CHD_V5_HEADER_SIZE))
        return CHDERR_INVALID_PARAMETER;
    if (header->version <= 4) {
        if (header->flags & CHDFLAGS_UNDEFINED)
            return CHDERR_INVALID_PARAMETER;
        for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++)
            if (codec_interfaces[intfnum].compression == header->compression[0])
                break;
        if (intfnum == ARRAY_LENGTH(codec_interfaces))
            return CHDERR_INVALID_PARAMETER;
        if (header->hunkbytes == 0 || header->hunkbytes >= 65536 * 256)
            return CHDERR_INVALID_PARAMETER;
        if (header->totalhunks == 0)
            return CHDERR_INVALID_PARAMETER;
        if ((header->flags & CHDFLAGS_HAS_PARENT) && memcmp(header->parentmd5, nullmd5, sizeof(nullmd5)) == 0 &&
            memcmp(header->parentsha1, nullsha1, sizeof(nullsha1)) == 0)
            return CHDERR_INVALID_PARAMETER;
        if (header->version >= 3 && (header->obsolete_cylinders != 0 || header->obsolete_sectors != 0 ||
                                     header->obsolete_heads != 0 || header->obsolete_hunksize != 0))
            return CHDERR_INVALID_PARAMETER;
        if (header->version < 3 && (header->obsolete_cylinders == 0 || header->obsolete_sectors == 0 ||
                                    header->obsolete_heads == 0 || header->obsolete_hunksize == 0))
            return CHDERR_INVALID_PARAMETER;
    }
    return CHDERR_NONE;
}
static UINT32 header_guess_unitbytes(chd_file *chd)
{
    char metadata[512];
    int  i0, i1, i2, i3;
    if (chd_get_metadata(chd, HARD_DISK_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) == CHDERR_NONE &&
        sscanf(metadata, HARD_DISK_METADATA_FORMAT, &i0, &i1, &i2, &i3) == 4)
        return i3;
    if (chd_get_metadata(chd, CDROM_OLD_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) == CHDERR_NONE ||
        chd_get_metadata(chd, CDROM_TRACK_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) ==
            CHDERR_NONE ||
        chd_get_metadata(chd, CDROM_TRACK_METADATA2_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) ==
            CHDERR_NONE ||
        chd_get_metadata(chd, GDROM_OLD_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) == CHDERR_NONE ||
        chd_get_metadata(chd, GDROM_TRACK_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL, NULL) == CHDERR_NONE)
        return CD_FRAME_SIZE;
    return chd->header.hunkbytes;
}
static chd_error header_read(chd_file *chd, chd_header *header)
{
    UINT8  rawheader[CHD_MAX_HEADER_SIZE];
    UINT32 count;
    if (header == NULL)
        return CHDERR_INVALID_PARAMETER;
    if (chd->file == NULL)
        return CHDERR_INVALID_FILE;
    core_fseek(chd->file, 0, SEEK_SET);
    count = core_fread(chd->file, rawheader, sizeof(rawheader));
    if (count != sizeof(rawheader))
        return CHDERR_READ_ERROR;
    if (strncmp((char *)rawheader, "MComprHD", 8) != 0)
        return CHDERR_INVALID_DATA;
    memset(header, 0, sizeof(*header));
    header->length  = get_bigendian_uint32(&rawheader[8]);
    header->version = get_bigendian_uint32(&rawheader[12]);
    if (header->version == 0 || header->version > CHD_HEADER_VERSION)
        return CHDERR_UNSUPPORTED_VERSION;
    if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
        (header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
        (header->version == 3 && header->length != CHD_V3_HEADER_SIZE) ||
        (header->version == 4 && header->length != CHD_V4_HEADER_SIZE) ||
        (header->version == 5 && header->length != CHD_V5_HEADER_SIZE))
        return CHDERR_INVALID_DATA;
    header->flags          = get_bigendian_uint32(&rawheader[16]);
    header->compression[0] = get_bigendian_uint32(&rawheader[20]);
    header->compression[1] = CHD_CODEC_NONE;
    header->compression[2] = CHD_CODEC_NONE;
    header->compression[3] = CHD_CODEC_NONE;
    if (header->version < 3) {
        int seclen                 = (header->version == 1) ? CHD_V1_SECTOR_SIZE : get_bigendian_uint32(&rawheader[76]);
        header->obsolete_hunksize  = get_bigendian_uint32(&rawheader[24]);
        header->totalhunks         = get_bigendian_uint32(&rawheader[28]);
        header->obsolete_cylinders = get_bigendian_uint32(&rawheader[32]);
        header->obsolete_heads     = get_bigendian_uint32(&rawheader[36]);
        header->obsolete_sectors   = get_bigendian_uint32(&rawheader[40]);
        memcpy(header->md5, &rawheader[44], CHD_MD5_BYTES);
        memcpy(header->parentmd5, &rawheader[60], CHD_MD5_BYTES);
        header->logicalbytes = (UINT64)header->obsolete_cylinders * (UINT64)header->obsolete_heads *
                               (UINT64)header->obsolete_sectors * (UINT64)seclen;
        header->hunkbytes  = seclen * header->obsolete_hunksize;
        header->unitbytes  = header_guess_unitbytes(chd);
        header->unitcount  = (header->logicalbytes + header->unitbytes - 1) / header->unitbytes;
        header->metaoffset = 0;
    } else if (header->version == 3) {
        header->totalhunks   = get_bigendian_uint32(&rawheader[24]);
        header->logicalbytes = get_bigendian_uint64(&rawheader[28]);
        header->metaoffset   = get_bigendian_uint64(&rawheader[36]);
        memcpy(header->md5, &rawheader[44], CHD_MD5_BYTES);
        memcpy(header->parentmd5, &rawheader[60], CHD_MD5_BYTES);
        header->hunkbytes = get_bigendian_uint32(&rawheader[76]);
        header->unitbytes = header_guess_unitbytes(chd);
        header->unitcount = (header->logicalbytes + header->unitbytes - 1) / header->unitbytes;
        memcpy(header->sha1, &rawheader[80], CHD_SHA1_BYTES);
        memcpy(header->parentsha1, &rawheader[100], CHD_SHA1_BYTES);
    } else if (header->version == 4) {
        header->totalhunks   = get_bigendian_uint32(&rawheader[24]);
        header->logicalbytes = get_bigendian_uint64(&rawheader[28]);
        header->metaoffset   = get_bigendian_uint64(&rawheader[36]);
        header->hunkbytes    = get_bigendian_uint32(&rawheader[44]);
        header->unitbytes    = header_guess_unitbytes(chd);
        header->unitcount    = (header->logicalbytes + header->unitbytes - 1) / header->unitbytes;
        memcpy(header->sha1, &rawheader[48], CHD_SHA1_BYTES);
        memcpy(header->parentsha1, &rawheader[68], CHD_SHA1_BYTES);
        memcpy(header->rawsha1, &rawheader[88], CHD_SHA1_BYTES);
    } else if (header->version == 5) {
        header->compression[0] = get_bigendian_uint32(&rawheader[16]);
        header->compression[1] = get_bigendian_uint32(&rawheader[20]);
        header->compression[2] = get_bigendian_uint32(&rawheader[24]);
        header->compression[3] = get_bigendian_uint32(&rawheader[28]);
        header->logicalbytes   = get_bigendian_uint64(&rawheader[32]);
        header->mapoffset      = get_bigendian_uint64(&rawheader[40]);
        header->metaoffset     = get_bigendian_uint64(&rawheader[48]);
        header->hunkbytes      = get_bigendian_uint32(&rawheader[56]);
        header->hunkcount      = (header->logicalbytes + header->hunkbytes - 1) / header->hunkbytes;
        header->unitbytes      = get_bigendian_uint32(&rawheader[60]);
        header->unitcount      = (header->logicalbytes + header->unitbytes - 1) / header->unitbytes;
        memcpy(header->sha1, &rawheader[84], CHD_SHA1_BYTES);
        memcpy(header->parentsha1, &rawheader[104], CHD_SHA1_BYTES);
        memcpy(header->rawsha1, &rawheader[64], CHD_SHA1_BYTES);
        header->mapentrybytes = compressed(header) ? 12 : 4;
        header->totalhunks    = header->hunkcount;
    } else {
    }
    return CHDERR_NONE;
}
static chd_error hunk_read_into_cache(chd_file *chd, UINT32 hunknum)
{
    chd_error err;
    if (hunknum > chd->maxhunk)
        chd->maxhunk = hunknum;
    if (chd->cachehunk == hunknum)
        return CHDERR_NONE;
    chd->cachehunk = ~0;
    err            = hunk_read_into_memory(chd, hunknum, chd->cache);
    if (err != CHDERR_NONE)
        return err;
    chd->cachehunk = hunknum;
    return CHDERR_NONE;
}
static chd_error hunk_read_into_memory(chd_file *chd, UINT32 hunknum, UINT8 *dest)
{
    chd_error err;
    if (chd->file == NULL)
        return CHDERR_INVALID_FILE;
    if (hunknum >= chd->header.totalhunks)
        return CHDERR_HUNK_OUT_OF_RANGE;
    if (chd->header.version < 5) {
        map_entry *entry = &chd->map[hunknum];
        UINT32     bytes;
        switch (entry->flags & MAP_ENTRY_FLAG_TYPE_MASK) {
            case V34_MAP_ENTRY_TYPE_COMPRESSED:
                core_fseek(chd->file, entry->offset, SEEK_SET);
                bytes = core_fread(chd->file, chd->compressed, entry->length);
                if (bytes != entry->length)
                    return CHDERR_READ_ERROR;
                err         = CHDERR_NONE;
                void *codec = &chd->zlib_codec_data;
                if (chd->codecintf[0]->decompress != NULL)
                    err = (*chd->codecintf[0]->decompress)(codec, chd->compressed, entry->length, dest,
                                                           chd->header.hunkbytes);
                if (err != CHDERR_NONE)
                    return err;
                break;
            case V34_MAP_ENTRY_TYPE_UNCOMPRESSED:
                core_fseek(chd->file, entry->offset, SEEK_SET);
                bytes = core_fread(chd->file, dest, chd->header.hunkbytes);
                if (bytes != chd->header.hunkbytes)
                    return CHDERR_READ_ERROR;
                break;
            case V34_MAP_ENTRY_TYPE_MINI:
                put_bigendian_uint64(&dest[0], entry->offset);
                for (bytes = 8; bytes < chd->header.hunkbytes; bytes++)
                    dest[bytes] = dest[bytes - 8];
                break;
            case V34_MAP_ENTRY_TYPE_SELF_HUNK:
                if (chd->cachehunk == entry->offset && dest == chd->cache)
                    break;
                return hunk_read_into_memory(chd, entry->offset, dest);
            case V34_MAP_ENTRY_TYPE_PARENT_HUNK:
                err = hunk_read_into_memory(chd->parent, entry->offset, dest);
                if (err != CHDERR_NONE)
                    return err;
                break;
        }
        return CHDERR_NONE;
    } else {
        uint64_t blockoffs;
        uint32_t blocklen;
        uint16_t blockcrc;
        uint8_t *rawmap = &chd->header.rawmap[chd->header.mapentrybytes * hunknum];
        if (!compressed(&chd->header)) {
            blockoffs = (uint64_t)get_bigendian_uint32(rawmap) * (uint64_t)chd->header.hunkbytes;
            if (blockoffs != 0) {
                core_fseek(chd->file, blockoffs, SEEK_SET);
                core_fread(chd->file, dest, chd->header.hunkbytes);
            } else if (chd->parent) {
                err = hunk_read_into_memory(chd->parent, hunknum, dest);
                if (err != CHDERR_NONE)
                    return err;
            } else {
                memset(dest, 0, chd->header.hunkbytes);
            }
        }
        blocklen    = get_bigendian_uint24(&rawmap[1]);
        blockoffs   = get_bigendian_uint48(&rawmap[4]);
        blockcrc    = get_bigendian_uint16(&rawmap[10]);
        void *codec = NULL;
        switch (rawmap[0]) {
            case COMPRESSION_TYPE_0:
            case COMPRESSION_TYPE_1:
            case COMPRESSION_TYPE_2:
            case COMPRESSION_TYPE_3:
                core_fseek(chd->file, blockoffs, SEEK_SET);
                core_fread(chd->file, chd->compressed, blocklen);
                switch (chd->codecintf[rawmap[0]]->compression) {
                        // case CHD_CODEC_CD_LZMA:
                        //     codec = &chd->cdlz_codec_data;
                        //     break;
                    case CHD_CODEC_ZLIB:
                        codec = &chd->zlib_codec_data;
                        break;
                    case CHD_CODEC_CD_ZLIB:
                        codec = &chd->cdzl_codec_data;
                        break;
                        // case CHD_CODEC_CD_FLAC:
                        //     codec = &chd->cdfl_codec_data;
                        //     break;
                }
                if (codec == NULL)
                    return CHDERR_DECOMPRESSION_ERROR;
                chd->codecintf[rawmap[0]]->decompress(codec, chd->compressed, blocklen, dest, chd->header.hunkbytes);
                if (dest != NULL && crc16(dest, chd->header.hunkbytes) != blockcrc)
                    return CHDERR_DECOMPRESSION_ERROR;
                return CHDERR_NONE;
            case COMPRESSION_NONE:
                core_fseek(chd->file, blockoffs, SEEK_SET);
                core_fread(chd->file, dest, chd->header.hunkbytes);
                if (crc16(dest, chd->header.hunkbytes) != blockcrc)
                    return CHDERR_DECOMPRESSION_ERROR;
                return CHDERR_NONE;
            case COMPRESSION_SELF:
                return hunk_read_into_memory(chd, blockoffs, dest);
            case COMPRESSION_PARENT:
#if 0
				if (m_parent_missing)
					return CHDERR_REQUIRES_PARENT;
				return m_parent->read_bytes(uint64_t(blockoffs) * uint64_t(m_parent->unit_bytes()), dest, m_hunkbytes);
#endif
                return CHDERR_DECOMPRESSION_ERROR;
        }
        return CHDERR_NONE;
    }
    return CHDERR_DECOMPRESSION_ERROR;
}
static chd_error map_read(chd_file *chd)
{
    UINT32    entrysize = (chd->header.version < 3) ? OLD_MAP_ENTRY_SIZE : MAP_ENTRY_SIZE;
    UINT8     raw_map_entries[MAP_STACK_ENTRIES * MAP_ENTRY_SIZE];
    UINT64    fileoffset, maxoffset = 0;
    UINT8     cookie[MAP_ENTRY_SIZE];
    UINT32    count;
    chd_error err;
    int       i;
    chd->map = (map_entry *)malloc(sizeof(chd->map[0]) * chd->header.totalhunks);
    if (!chd->map)
        return CHDERR_OUT_OF_MEMORY;
    fileoffset = chd->header.length;
    for (i = 0; i < chd->header.totalhunks; i += MAP_STACK_ENTRIES) {
        int entries = chd->header.totalhunks - i, j;
        if (entries > MAP_STACK_ENTRIES)
            entries = MAP_STACK_ENTRIES;
        core_fseek(chd->file, fileoffset, SEEK_SET);
        count = core_fread(chd->file, raw_map_entries, entries * entrysize);
        if (count != entries * entrysize) {
            err = CHDERR_READ_ERROR;
            goto cleanup;
        }
        fileoffset += entries * entrysize;
        if (entrysize == MAP_ENTRY_SIZE) {
            for (j = 0; j < entries; j++)
                map_extract(&raw_map_entries[j * MAP_ENTRY_SIZE], &chd->map[i + j]);
        } else {
            for (j = 0; j < entries; j++)
                map_extract_old(&raw_map_entries[j * OLD_MAP_ENTRY_SIZE], &chd->map[i + j], chd->header.hunkbytes);
        }
        for (j = 0; j < entries; j++)
            if ((chd->map[i + j].flags & MAP_ENTRY_FLAG_TYPE_MASK) == V34_MAP_ENTRY_TYPE_COMPRESSED ||
                (chd->map[i + j].flags & MAP_ENTRY_FLAG_TYPE_MASK) == V34_MAP_ENTRY_TYPE_UNCOMPRESSED)
                maxoffset = MAX(maxoffset, chd->map[i + j].offset + chd->map[i + j].length);
    }
    core_fseek(chd->file, fileoffset, SEEK_SET);
    count = core_fread(chd->file, &cookie, entrysize);
    if (count != entrysize || memcmp(&cookie, END_OF_LIST_COOKIE, entrysize)) {
        err = CHDERR_INVALID_FILE;
        goto cleanup;
    }
    if (maxoffset > core_fsize(chd->file)) {
        err = CHDERR_INVALID_FILE;
        goto cleanup;
    }
    return CHDERR_NONE;
cleanup:
    if (chd->map)
        free(chd->map);
    chd->map = NULL;
    return err;
}
static chd_error metadata_find_entry(chd_file *chd, UINT32 metatag, UINT32 metaindex, metadata_entry *metaentry)
{
    metaentry->offset = chd->header.metaoffset;
    metaentry->prev   = 0;
    while (metaentry->offset != 0) {
        UINT8  raw_meta_header[METADATA_HEADER_SIZE];
        UINT32 count;
        core_fseek(chd->file, metaentry->offset, SEEK_SET);
        count = core_fread(chd->file, raw_meta_header, sizeof(raw_meta_header));
        if (count != sizeof(raw_meta_header))
            break;
        metaentry->metatag = get_bigendian_uint32(&raw_meta_header[0]);
        metaentry->length  = get_bigendian_uint32(&raw_meta_header[4]);
        metaentry->next    = get_bigendian_uint64(&raw_meta_header[8]);
        metaentry->flags   = metaentry->length >> 24;
        metaentry->length &= 0x00ffffff;
        if (metatag == CHDMETATAG_WILDCARD || metaentry->metatag == metatag)
            if (metaindex-- == 0)
                return CHDERR_NONE;
        metaentry->prev   = metaentry->offset;
        metaentry->offset = metaentry->next;
    }
    return CHDERR_METADATA_NOT_FOUND;
}
static chd_error zlib_codec_init(void *codec, uint32_t hunkbytes)
{
    zlib_codec_data *data = (zlib_codec_data *)codec;
    chd_error        err;
    int              zerr;
    memset(data, 0, sizeof(zlib_codec_data));
    data->inflater.next_in  = (Bytef *)data;
    data->inflater.avail_in = 0;
    data->inflater.zalloc   = zlib_fast_alloc;
    data->inflater.zfree    = zlib_fast_free;
    data->inflater.opaque   = &data->allocator;
    zerr                    = inflateInit2(&data->inflater, -MAX_WBITS);
    if (zerr == Z_MEM_ERROR)
        err = CHDERR_OUT_OF_MEMORY;
    else if (zerr != Z_OK)
        err = CHDERR_CODEC_ERROR;
    else
        err = CHDERR_NONE;
    if (err != CHDERR_NONE)
        free(data);
    return err;
}
static void zlib_codec_free(void *codec)
{
    zlib_codec_data *data = (zlib_codec_data *)codec;
    if (data != NULL) {
        int i;
        inflateEnd(&data->inflater);
        zlib_allocator alloc = data->allocator;
        for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
            if (alloc.allocptr[i])
                free(alloc.allocptr[i]);
    }
}
static chd_error zlib_codec_decompress(void *codec, const uint8_t *src, uint32_t complen, uint8_t *dest,
                                       uint32_t destlen)
{
    zlib_codec_data *data = (zlib_codec_data *)codec;
    int              zerr;
    data->inflater.next_in   = (Bytef *)src;
    data->inflater.avail_in  = complen;
    data->inflater.total_in  = 0;
    data->inflater.next_out  = (Bytef *)dest;
    data->inflater.avail_out = destlen;
    data->inflater.total_out = 0;
    zerr                     = inflateReset(&data->inflater);
    if (zerr != Z_OK)
        return CHDERR_DECOMPRESSION_ERROR;
    zerr = inflate(&data->inflater, Z_FINISH);
    if (data->inflater.total_out != destlen)
        return CHDERR_DECOMPRESSION_ERROR;
    return CHDERR_NONE;
}
#define ZLIB_MIN_ALIGNMENT_BITS  512
#define ZLIB_MIN_ALIGNMENT_BYTES (ZLIB_MIN_ALIGNMENT_BITS / 8)
static voidpf zlib_fast_alloc(voidpf opaque, uInt items, uInt size)
{
    zlib_allocator *alloc = (zlib_allocator *)opaque;
    uintptr_t       paddr = 0;
    UINT32         *ptr;
    int             i;
    size = (size * items + 0x3ff) & ~0x3ff;
    for (i = 0; i < MAX_ZLIB_ALLOCS; i++) {
        ptr = alloc->allocptr[i];
        if (ptr && size == *ptr) {
            *ptr |= 1;
            return (voidpf)(alloc->allocptr2[i]);
        }
    }
    ptr = (UINT32 *)malloc(size + sizeof(UINT32) + ZLIB_MIN_ALIGNMENT_BYTES);
    if (!ptr)
        return NULL;
    for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
        if (!alloc->allocptr[i]) {
            alloc->allocptr[i] = ptr;
            paddr              = (((uintptr_t)ptr) + sizeof(UINT32) + (ZLIB_MIN_ALIGNMENT_BYTES - 1)) &
                    (~(ZLIB_MIN_ALIGNMENT_BYTES - 1));
            alloc->allocptr2[i] = (uint32_t *)paddr;
            break;
        }
    *ptr = size | 1;
    return (voidpf)paddr;
}
static void zlib_fast_free(voidpf opaque, voidpf address)
{
    zlib_allocator *alloc = (zlib_allocator *)opaque;
    UINT32         *ptr   = (UINT32 *)address;
    int             i;
    for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
        if (ptr == alloc->allocptr2[i]) {
            *(alloc->allocptr[i]) &= ~1;
            return;
        }
}
