
#ifndef __JUNZIP_H
#define __JUNZIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct JZFile JZFile;

struct JZFile
{
    size_t (*read)(JZFile *file, void *buf, size_t size);
    size_t (*tell)(JZFile *file);
    int (*seek)(JZFile *file, size_t offset, int whence);
    int (*error)(JZFile *file);
    void (*close)(JZFile *file);
};

JZFile *jzfile_from_stdio_file(FILE *fp);

#pragma pack(push, 1)
typedef struct
{
    uint32_t signature;
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
} JZLocalFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint32_t signature;
    uint16_t versionMadeBy;
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
    uint16_t fileCommentLength;
    uint16_t diskNumberStart;
    uint16_t internalFileAttributes;
    uint32_t externalFileAttributes;
    uint32_t relativeOffsetOflocalHeader;
} JZGlobalFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint32_t offset;
} JZFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint32_t signature;
    uint16_t diskNumber;
    uint16_t centralDirectoryDiskNumber;
    uint16_t numEntriesThisDisk;
    uint16_t numEntries;
    uint32_t centralDirectorySize;
    uint32_t centralDirectoryOffset;
    uint16_t zipCommentLength;
} JZEndRecord;
#pragma pack(pop)

typedef int (*JZRecordCallback)(JZFile *zip, int index, JZFileHeader *header, char *filename, void *user_data);

#define JZ_BUFFER_SIZE 65536

int jzReadEndRecord(JZFile *zip, JZEndRecord *endRecord);

int jzReadCentralDirectory(JZFile *zip, JZEndRecord *endRecord, JZRecordCallback callback, void *user_data);

int jzReadLocalFileHeader(JZFile *zip, JZFileHeader *header, char *filename, int len);

int jzReadData(JZFile *zip, JZFileHeader *header, void *buffer);

#ifdef __cplusplus
};
#endif

#endif
