/*
 Copyright (c) 2010, Florian Reuter
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions 
 are met:
 
 * Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright 
   notice, this list of conditions and the following disclaimer in 
   the documentation and/or other materials provided with the 
   distribution.
 * Neither the name of Florian Reuter nor the names of its contributors 
   may be used to endorse or promote products derived from this 
   software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
*/
/** @file opc/zip.h
 
 */
#include <opc/config.h>
#include <zlib.h>

#ifndef OPC_ZIP_H
#define OPC_ZIP_H

#ifdef __cplusplus
extern "C" {
#endif    
    #define OPC_DEFAULT_GROWTH_HINT 512

    typedef struct OPC_ZIP_STRUCT opcZip;

    typedef enum OPC_ZIPSEEKMODE_ENUM {
        opcZipSeekSet = SEEK_SET,
        opcZipSeekCur = SEEK_CUR,
        opcZipSeekEnd = SEEK_END
    } opcZipSeekMode;

    typedef int opcZipReadCallback(void *iocontext, char *buffer, int len);
    typedef int opcZipWriteCallback(void *iocontext, const char *buffer, int len);
    typedef int opcZipCloseCallback(void *iocontext);
    typedef opc_ofs_t opcZipSeekCallback(void *iocontext, opc_ofs_t ofs);

#define OPC_ZIP_READ  0x1
#define OPC_ZIP_WRITE 0x2
#define OPC_ZIP_STREAM  0x4

    opcZip *opcZipOpenFile(const xmlChar *fileName, int flags);
    opcZip *opcZipOpenIO(opcZipReadCallback *ioread,
                         opcZipWriteCallback *iowrite,
                         opcZipCloseCallback *ioclose,
                         opcZipSeekCallback *ioseek,
                         void *iocontext,
                         pofs_t file_size,
                         int flags);
    void opcZipClose(opcZip *zip);

    typedef struct OPC_ZIPSEGMENT_STRUCT {
        opc_uint16_t bit_flag;
        opc_uint32_t crc32;
        opc_uint16_t compression_method;
        opc_ofs_t stream_ofs;
        opc_uint32_t header_size;
        opc_ofs_t compressed_size;
        opc_ofs_t uncompressed_size;
        opc_uint32_t growth_hint; 
    } opcZipSegment;

    opc_error_t opcZipInitSegment(opcZipSegment *segment);
    opc_error_t opcZipCleanupSegment(opcZipSegment *segment);

    typedef struct OPC_ZIPSTREAMSTATE_STRUCT {
        opc_error_t err;
        opc_ofs_t   buf_pos; // current pos in file
    } opcZipRawState;

    opc_error_t opcZipInitRawState(opcZip *zip, opcZipRawState *rawState);

    typedef struct OPC_ZIPRAWBUFFER_STRUCT {
        opcZipRawState state;
        puint32_t   buf_ofs;
        puint32_t   buf_len;
        opc_uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
    } opcZipRawBuffer;

    opc_error_t opcZipInitRawBuffer(opcZip *zip, opcZipRawBuffer *rawBuffer);

    typedef struct OPC_ZIPINFLATESTATE_STRUCT {
        z_stream stream;
        opc_uint16_t compression_method;
        int inflate_state;
        opc_ofs_t compressed_size;
    } opcZipInflateState;

    opc_error_t opcZipInitInflateState(opcZipRawState *rawState, opcZipSegment *segment, opcZipInflateState *state);
    opc_error_t opcZipCleanupInflateState(opcZipRawState *rawState, opcZipSegment *segment, opcZipInflateState *state);

    opc_bool_t opcZipRawReadLocalFile(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment, xmlChar **name, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment);
    opc_uint32_t opcZipRawReadFileData(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipInflateState *state, opc_uint8_t *buffer, opc_uint32_t buf_len);
    opc_error_t opcZipRawReadDataDescriptor(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment);
    opc_error_t opcZipRawSkipFileData(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment);

    opc_bool_t opcZipRawReadCentralDirectory(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment, xmlChar **name, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment);
    opc_bool_t opcZipRawReadEndOfCentralDirectory(opcZip *zip, opcZipRawBuffer *rawBuffer, opc_uint16_t *central_dir_entries);

    typedef struct OPC_ZIPSEGMENTINPUTSTREAM_STRUCT opcZipSegmentInputStream;

    opcZipSegmentInputStream* opcZipCreateSegmentInputStream(opcZip *zip, opcZipSegment *segment);
    opc_uint32_t opcZipReadSegmentInputStream(opcZip *zip, opcZipSegmentInputStream *stream, opc_uint8_t *buf, opc_uint32_t buf_len);
    opc_error_t opcZipCloseSegmentInputStream(opcZip *zip, opcZipSegment *segment, opcZipSegmentInputStream *stream);
    opc_error_t opcZipSegmentInputStreamState(opcZipSegmentInputStream *stream);

    typedef struct OPC_ZIPSEGMENTBUFFER_STRUCT opcZipSegmentBuffer;

    opcZipSegmentBuffer* opcZipCreateSegment(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, xmlChar *name, opc_uint32_t segment_number, opc_uint16_t compression_method, opc_uint32_t growth_hint, opc_uint32_t max_size);
    opcZipSegmentBuffer* opcZipFinalizeSegment(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, opcZipSegmentBuffer* buffer); 
    opc_error_t opcZipUpdateHeader(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, xmlChar *name, opc_uint32_t segment_number, opc_bool_t last_segment);

    opc_uint32_t opcZipSegmentBufferWrite(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer, const opc_uint8_t *buf, opc_uint32_t buf_len);
    opc_bool_t opcZipSegmentBufferClose(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer);

    opc_error_t opcZipWriteCentralDirectory(opcZip *zip, opcZipRawState *rawState, opcZipSegment* segment, xmlChar *name, opc_uint32_t segment_number, opc_bool_t last_segment);
    opc_error_t opcZipWriteEndOfCentralDirectory(opcZip *zip, opcZipRawState *rawState, opc_ofs_t central_dir_start_ofs, opc_uint32_t segments);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_ZIP_H */
