
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "junzip.h"

unsigned char jzBuffer[JZ_BUFFER_SIZE];

int jzReadEndRecord(JZFile *zip, JZEndRecord *endRecord)
{
    long         fileSize, readBytes, i;
    JZEndRecord *er;

    if (zip->seek(zip, 0, SEEK_END)) {
        fprintf(stderr, "Couldn't go to end of zip file!");
        return Z_ERRNO;
    }

    if ((fileSize = zip->tell(zip)) <= sizeof(JZEndRecord)) {
        fprintf(stderr, "Too small file to be a zip!");
        return Z_ERRNO;
    }

    readBytes = (fileSize < sizeof(jzBuffer)) ? fileSize : sizeof(jzBuffer);

    if (zip->seek(zip, fileSize - readBytes, SEEK_SET)) {
        fprintf(stderr, "Cannot seek in zip file!");
        return Z_ERRNO;
    }

    if (zip->read(zip, jzBuffer, readBytes) < readBytes) {
        fprintf(stderr, "Couldn't read end of zip file!");
        return Z_ERRNO;
    }

    for (i = readBytes - sizeof(JZEndRecord); i >= 0; i--) {
        er = (JZEndRecord *)(jzBuffer + i);
        if (er->signature == 0x06054B50)
            break;
    }

    if (i < 0) {
        fprintf(stderr, "End record signature not found in zip!");
        return Z_ERRNO;
    }

    memcpy(endRecord, er, sizeof(JZEndRecord));

    if (endRecord->diskNumber || endRecord->centralDirectoryDiskNumber ||
        endRecord->numEntries != endRecord->numEntriesThisDisk) {
        fprintf(stderr, "Multifile zips not supported!");
        return Z_ERRNO;
    }

    return Z_OK;
}

int jzReadCentralDirectory(JZFile *zip, JZEndRecord *endRecord, JZRecordCallback callback, void *user_data)
{
    JZGlobalFileHeader fileHeader;
    JZFileHeader       header;
    int                i;

    if (zip->seek(zip, endRecord->centralDirectoryOffset, SEEK_SET)) {
        fprintf(stderr, "Cannot seek in zip file!");
        return Z_ERRNO;
    }

    for (i = 0; i < endRecord->numEntries; i++) {
        if (zip->read(zip, &fileHeader, sizeof(JZGlobalFileHeader)) < sizeof(JZGlobalFileHeader)) {
            fprintf(stderr, "Couldn't read file header %d!", i);
            return Z_ERRNO;
        }

        if (fileHeader.signature != 0x02014B50) {
            fprintf(stderr, "Invalid file header signature %d!", i);
            return Z_ERRNO;
        }

        if (fileHeader.fileNameLength + 1 >= JZ_BUFFER_SIZE) {
            fprintf(stderr, "Too long file name %d!", i);
            return Z_ERRNO;
        }

        if (zip->read(zip, jzBuffer, fileHeader.fileNameLength) < fileHeader.fileNameLength) {
            fprintf(stderr, "Couldn't read filename %d!", i);
            return Z_ERRNO;
        }

        jzBuffer[fileHeader.fileNameLength] = '\0';

        if (zip->seek(zip, fileHeader.extraFieldLength, SEEK_CUR) ||
            zip->seek(zip, fileHeader.fileCommentLength, SEEK_CUR)) {
            fprintf(stderr, "Couldn't skip extra field or file comment %d", i);
            return Z_ERRNO;
        }

        memcpy(&header, &fileHeader.compressionMethod, sizeof(header));
        header.offset = fileHeader.relativeOffsetOflocalHeader;

        if (!callback(zip, i, &header, (char *)jzBuffer, user_data))
            break;
    }

    return Z_OK;
}

int jzReadLocalFileHeader(JZFile *zip, JZFileHeader *header, char *filename, int len)
{
    JZLocalFileHeader localHeader;

    if (zip->read(zip, &localHeader, sizeof(JZLocalFileHeader)) < sizeof(JZLocalFileHeader))
        return Z_ERRNO;

    if (localHeader.signature != 0x04034B50)
        return Z_ERRNO;

    if (len) {
        if (localHeader.fileNameLength >= len)
            return Z_ERRNO;

        if (zip->read(zip, filename, localHeader.fileNameLength) < localHeader.fileNameLength)
            return Z_ERRNO;

        filename[localHeader.fileNameLength] = '\0';
    } else {
        if (zip->seek(zip, localHeader.fileNameLength, SEEK_CUR))
            return Z_ERRNO;
    }

    if (localHeader.extraFieldLength) {
        if (zip->seek(zip, localHeader.extraFieldLength, SEEK_CUR))
            return Z_ERRNO;
    }

    if (localHeader.compressionMethod == 0 && (localHeader.compressedSize != localHeader.uncompressedSize))
        return Z_ERRNO;

    memcpy(header, &localHeader.compressionMethod, sizeof(JZFileHeader));
    header->offset = 0;

    return Z_OK;
}

int jzReadData(JZFile *zip, JZFileHeader *header, void *buffer)
{
    unsigned char *bytes = (unsigned char *)buffer;
    long           compressedLeft, uncompressedLeft;
    z_stream       strm;
    int            ret;

    if (header->compressionMethod == 0) {
        if (zip->read(zip, buffer, header->uncompressedSize) < header->uncompressedSize || zip->error(zip))
            return Z_ERRNO;
    } else if (header->compressionMethod == 8) {
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;

        strm.avail_in = 0;
        strm.next_in  = Z_NULL;

        if ((ret = inflateInit2(&strm, -MAX_WBITS)) != Z_OK)
            return ret;

        for (compressedLeft = header->compressedSize, uncompressedLeft = header->uncompressedSize;
             compressedLeft && uncompressedLeft && ret != Z_STREAM_END; compressedLeft -= strm.avail_in) {
            strm.avail_in =
                zip->read(zip, jzBuffer, (sizeof(jzBuffer) < compressedLeft) ? sizeof(jzBuffer) : compressedLeft);

            if (strm.avail_in == 0 || zip->error(zip)) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }

            strm.next_in   = jzBuffer;
            strm.avail_out = uncompressedLeft;
            strm.next_out  = bytes;

            compressedLeft -= strm.avail_in;

            ret = inflate(&strm, Z_NO_FLUSH);

            if (ret == Z_STREAM_ERROR)
                return ret;

            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }

            bytes += uncompressedLeft - strm.avail_out;
            uncompressedLeft = strm.avail_out;
        }

        inflateEnd(&strm);
    } else {
        return Z_ERRNO;
    }

    return Z_OK;
}

typedef struct
{
    JZFile handle;
    FILE  *fp;
} StdioJZFile;

static size_t stdio_read_file_handle_read(JZFile *file, void *buf, size_t size)
{
    StdioJZFile *handle = (StdioJZFile *)file;
    return fread(buf, 1, size, handle->fp);
}

static size_t stdio_read_file_handle_tell(JZFile *file)
{
    StdioJZFile *handle = (StdioJZFile *)file;
    return ftell(handle->fp);
}

static int stdio_read_file_handle_seek(JZFile *file, size_t offset, int whence)
{
    StdioJZFile *handle = (StdioJZFile *)file;
    return fseek(handle->fp, offset, whence);
}

static int stdio_read_file_handle_error(JZFile *file)
{
    StdioJZFile *handle = (StdioJZFile *)file;
    return ferror(handle->fp);
}

static void stdio_read_file_handle_close(JZFile *file)
{
    StdioJZFile *handle = (StdioJZFile *)file;
    fclose(handle->fp);
    free(file);
}

JZFile *jzfile_from_stdio_file(FILE *fp)
{
    StdioJZFile *handle = (StdioJZFile *)malloc(sizeof(StdioJZFile));

    handle->handle.read  = stdio_read_file_handle_read;
    handle->handle.tell  = stdio_read_file_handle_tell;
    handle->handle.seek  = stdio_read_file_handle_seek;
    handle->handle.error = stdio_read_file_handle_error;
    handle->handle.close = stdio_read_file_handle_close;
    handle->fp           = fp;

    return &(handle->handle);
}
