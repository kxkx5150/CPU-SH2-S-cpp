
#pragma once

#ifndef __CHD_H__
#define __CHD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "coretypes.h"

#define CHD_HEADER_VERSION 5
#define CHD_V1_HEADER_SIZE 76
#define CHD_V2_HEADER_SIZE 80
#define CHD_V3_HEADER_SIZE 120
#define CHD_V4_HEADER_SIZE 108
#define CHD_V5_HEADER_SIZE 124

#define CHD_MAX_HEADER_SIZE CHD_V5_HEADER_SIZE

#define CHD_MD5_BYTES  16
#define CHD_SHA1_BYTES 20

#define CHDFLAGS_HAS_PARENT   0x00000001
#define CHDFLAGS_IS_WRITEABLE 0x00000002
#define CHDFLAGS_UNDEFINED    0xfffffffc

#define CHD_MAKE_TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define CHDCOMPRESSION_NONE      0
#define CHDCOMPRESSION_ZLIB      1
#define CHDCOMPRESSION_ZLIB_PLUS 2
#define CHDCOMPRESSION_AV        3

#define CHD_CODEC_NONE    0
#define CHD_CODEC_ZLIB    CHD_MAKE_TAG('z', 'l', 'i', 'b')
#define CHD_CODEC_CD_ZLIB CHD_MAKE_TAG('c', 'd', 'z', 'l')
#define CHD_CODEC_CD_LZMA CHD_MAKE_TAG('c', 'd', 'l', 'z')
#define CHD_CODEC_CD_FLAC CHD_MAKE_TAG('c', 'd', 'f', 'l')

#define AV_CODEC_COMPRESS_CONFIG   1
#define AV_CODEC_DECOMPRESS_CONFIG 2

#define CHDMETATAG_WILDCARD  0
#define CHD_METAINDEX_APPEND ((UINT32)-1)

#define CHD_MDFLAGS_CHECKSUM 0x01

#define HARD_DISK_METADATA_TAG    CHD_MAKE_TAG('G', 'D', 'D', 'D')
#define HARD_DISK_METADATA_FORMAT "CYLS:%d,HEADS:%d,SECS:%d,BPS:%d"

#define HARD_DISK_IDENT_METADATA_TAG CHD_MAKE_TAG('I', 'D', 'N', 'T')

#define HARD_DISK_KEY_METADATA_TAG CHD_MAKE_TAG('K', 'E', 'Y', ' ')

#define PCMCIA_CIS_METADATA_TAG CHD_MAKE_TAG('C', 'I', 'S', ' ')

#define CDROM_OLD_METADATA_TAG       CHD_MAKE_TAG('C', 'H', 'C', 'D')
#define CDROM_TRACK_METADATA_TAG     CHD_MAKE_TAG('C', 'H', 'T', 'R')
#define CDROM_TRACK_METADATA_FORMAT  "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d"
#define CDROM_TRACK_METADATA2_TAG    CHD_MAKE_TAG('C', 'H', 'T', '2')
#define CDROM_TRACK_METADATA2_FORMAT "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d PREGAP:%d PGTYPE:%s PGSUB:%s POSTGAP:%d"
#define GDROM_OLD_METADATA_TAG       CHD_MAKE_TAG('C', 'H', 'G', 'T')
#define GDROM_TRACK_METADATA_TAG     CHD_MAKE_TAG('C', 'H', 'G', 'D')
#define GDROM_TRACK_METADATA_FORMAT                                                                                    \
    "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d PAD:%d PREGAP:%d PGTYPE:%s PGSUB:%s POSTGAP:%d"

#define AV_METADATA_TAG    CHD_MAKE_TAG('A', 'V', 'A', 'V')
#define AV_METADATA_FORMAT "FPS:%d.%06d WIDTH:%d HEIGHT:%d INTERLACED:%d CHANNELS:%d SAMPLERATE:%d"

#define AV_LD_METADATA_TAG CHD_MAKE_TAG('A', 'V', 'L', 'D')

#define CHD_OPEN_READ      1
#define CHD_OPEN_READWRITE 2

enum _chd_error
{
    CHDERR_NONE,
    CHDERR_NO_INTERFACE,
    CHDERR_OUT_OF_MEMORY,
    CHDERR_INVALID_FILE,
    CHDERR_INVALID_PARAMETER,
    CHDERR_INVALID_DATA,
    CHDERR_FILE_NOT_FOUND,
    CHDERR_REQUIRES_PARENT,
    CHDERR_FILE_NOT_WRITEABLE,
    CHDERR_READ_ERROR,
    CHDERR_WRITE_ERROR,
    CHDERR_CODEC_ERROR,
    CHDERR_INVALID_PARENT,
    CHDERR_HUNK_OUT_OF_RANGE,
    CHDERR_DECOMPRESSION_ERROR,
    CHDERR_COMPRESSION_ERROR,
    CHDERR_CANT_CREATE_FILE,
    CHDERR_CANT_VERIFY,
    CHDERR_NOT_SUPPORTED,
    CHDERR_METADATA_NOT_FOUND,
    CHDERR_INVALID_METADATA_SIZE,
    CHDERR_UNSUPPORTED_VERSION,
    CHDERR_VERIFY_INCOMPLETE,
    CHDERR_INVALID_METADATA,
    CHDERR_INVALID_STATE,
    CHDERR_OPERATION_PENDING,
    CHDERR_NO_ASYNC_OPERATION,
    CHDERR_UNSUPPORTED_FORMAT
};
typedef enum _chd_error chd_error;

typedef struct _chd_file chd_file;

typedef struct _chd_header chd_header;
struct _chd_header
{
    UINT32 length;
    UINT32 version;
    UINT32 flags;
    UINT32 compression[4];
    UINT32 hunkbytes;
    UINT32 totalhunks;
    UINT64 logicalbytes;
    UINT64 metaoffset;
    UINT64 mapoffset;
    UINT8  md5[CHD_MD5_BYTES];
    UINT8  parentmd5[CHD_MD5_BYTES];
    UINT8  sha1[CHD_SHA1_BYTES];
    UINT8  rawsha1[CHD_SHA1_BYTES];
    UINT8  parentsha1[CHD_SHA1_BYTES];
    UINT32 unitbytes;
    UINT64 unitcount;
    UINT32 hunkcount;

    UINT32 mapentrybytes;
    UINT8 *rawmap;

    UINT32 obsolete_cylinders;
    UINT32 obsolete_sectors;
    UINT32 obsolete_heads;
    UINT32 obsolete_hunksize;
};

typedef struct _chd_verify_result chd_verify_result;
struct _chd_verify_result
{
    UINT8 md5[CHD_MD5_BYTES];
    UINT8 sha1[CHD_SHA1_BYTES];
    UINT8 rawsha1[CHD_SHA1_BYTES];
    UINT8 metasha1[CHD_SHA1_BYTES];
};

chd_error chd_open(const char *filename, int mode, chd_file *parent, chd_file **chd);

void chd_close(chd_file *chd);

core_file *chd_core_file(chd_file *chd);

const char *chd_error_string(chd_error err);

const chd_header *chd_get_header(chd_file *chd);

chd_error chd_read(chd_file *chd, UINT32 hunknum, void *buffer);

chd_error chd_get_metadata(chd_file *chd, UINT32 searchtag, UINT32 searchindex, void *output, UINT32 outputlen,
                           UINT32 *resultlen, UINT32 *resulttag, UINT8 *resultflags);

chd_error chd_codec_config(chd_file *chd, int param, void *config);

const char *chd_get_codec_name(UINT32 codec);

#ifdef __cplusplus
}
#endif

#endif
