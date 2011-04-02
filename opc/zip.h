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
#include <opc/file.h>
#include <opc/container.h>

#ifndef OPC_ZIP_H
#define OPC_ZIP_H

#ifdef __cplusplus
extern "C" {
#endif    

    #define OPC_DEFAULT_GROWTH_HINT 512
    typedef struct OPC_ZIP_STRUCT opcZip;
    typedef struct OPC_ZIPINPUTSTREAM_STRUCT opcZipInputStream;
    typedef struct OPC_ZIPOUTPUTSTREAM_STRUCT opcZipOutputStream;

    typedef struct OPC_ZIP_SEGMENT_INFO_STRUCT {
        xmlChar name[OPC_MAX_PATH]; 
        opc_uint32_t name_len;
        opc_uint32_t segment_number;
        opc_bool_t   last_segment;
        opc_bool_t   rels_segment;
        opc_uint32_t header_size;
        opc_uint32_t min_header_size;
        opc_uint32_t compressed_size;
        opc_uint32_t uncompressed_size;
        opc_uint16_t bit_flag;
        opc_uint32_t data_crc;
        opc_uint16_t compression_method;
        opc_ofs_t    stream_ofs;
        opc_uint16_t growth_hint;
    } opcZipSegmentInfo_t;
    typedef opc_error_t (opcZipLoaderSegmentCallback_t)(void *iocontext, void *userctx, opcZipSegmentInfo_t *info, opcFileOpenCallback *open, opcFileReadCallback *read, opcFileCloseCallback *close, opcFileSkipCallback *skip);
    opc_error_t opcZipLoader(opcIO_t *io, void *userctx, opcZipLoaderSegmentCallback_t *segmentCallback);

    opcZip *opcZipCreate(opcIO_t *io);
    void opcZipClose(opcZip *zip);
    opc_error_t opcZipCommit(opcZip *zip, opc_bool_t trim);
    opc_error_t opcZipGC(opcZip *zip);

    opc_uint32_t opcZipLoadSegment(opcZip *zip, const xmlChar *partName, opc_bool_t rels_segment, opcZipSegmentInfo_t *info);
    opc_uint32_t opcZipCreateSegment(opcZip *zip, 
                                     const xmlChar *partName, 
                                     opc_bool_t relsSegment, 
                                     opc_uint32_t segment_size, 
                                     opc_uint32_t growth_hint,
                                     opc_uint16_t compression_method,
                                     opc_uint16_t bit_flag);

    opcZipInputStream *opcZipOpenInputStream(opcZip *zip, opc_uint32_t segment_id);
    opcZipOutputStream *opcZipCreateOutputStream(opcZip *zip, 
                                             opc_uint32_t *segment_id, 
                                             const xmlChar *partName, 
                                             opc_bool_t relsSegment, 
                                             opc_uint32_t segment_size, 
                                             opc_uint32_t growth_hint,
                                             opc_uint16_t compression_method,
                                             opc_uint16_t bit_flag);
    opc_error_t opcZipCloseInputStream(opcZip *zip, opcZipInputStream *stream);
    opc_uint32_t opcZipReadInputStream(opcZip *zip, opcZipInputStream *stream, opc_uint8_t *buf, opc_uint32_t buf_len);

    opcZipOutputStream *opcZipOpenOutputStream(opcZip *zip, opc_uint32_t *segment_id);
    opc_error_t opcZipCloseOutputStream(opcZip *zip, opcZipOutputStream *stream, opc_uint32_t *segment_id);
    opc_uint32_t opcZipWriteOutputStream(opcZip *zip, opcZipOutputStream *stream, const opc_uint8_t *buf, opc_uint32_t buf_len);

    opc_uint32_t opcZipGetFirstSegmentId(opcZip *zip);
    opc_uint32_t opcZipGetNextSegmentId(opcZip *zip, opc_uint32_t segment_id);
    opc_error_t opcZipGetSegmentInfo(opcZip *zip, opc_uint32_t segment_id, const xmlChar **name, opc_bool_t *rels_segment, opc_uint32_t *crc);

    opc_bool_t opcZipSegmentDelete(opcZip *zip, opc_uint32_t *first_segment, opc_uint32_t *last_segment);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_ZIP_H */
