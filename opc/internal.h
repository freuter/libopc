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
#ifndef OPC_INTERNAL_H
#define OPC_INTERNAL_H

#include <opc/config.h>
#include <opc/container.h>

#ifdef __cplusplus
extern "C" {
#endif    

    struct OPC_ZIP_STRUCT {
        opcZipReadCallback *_ioread;
        opcZipWriteCallback *_iowrite;
        opcZipCloseCallback *_ioclose;
        opcZipSeekCallback *_ioseek;
        void *iocontext;
        int flags;
        opcZipRawState state;
        opc_ofs_t file_size;
        opc_ofs_t append_ofs; // last offset in zip where data can be appended
    };

    struct OPC_ZIPSEGMENTINPUTSTREAM_STRUCT {
        opcZipRawBuffer raw;
        opcZipInflateState state;
    };

    typedef struct OPC_CONTAINER_PART_STRUCT {
        xmlChar *name;
        xmlChar *type; // owned by type_array
        opc_uint32_t first_segment_id;
        opc_uint32_t last_segment_id;
        opc_uint32_t segment_count;
        opc_uint32_t rel_segment_id;
    } opcContainerPart;

    typedef struct OPC_CONTAINER_SEGMENT_STRUCT {
        opcZipSegment zipSegment;
        xmlChar *part_name; // owned by part_array
        opc_uint32_t next_segment_id;
    } opcContainerSegment;

    typedef struct OPC_CONTAINER_PART_PREFIX_STRUCT {
        xmlChar *prefix;
    } opcContainerRelPrefix;

    #define OPC_CONTAINER_RELID_COUNTER(part) ((part)&0xFFFF)
    #define OPC_CONTAINER_RELID_PREFIX(part) (((part)>>16)&0xFFFF)

    typedef struct OPC_CONTAINER_TYPE_STRUCT {
        xmlChar *type;
        xmlChar *basedOn;
    } opcContainerType;

    typedef struct OPC_CONTAINER_EXTENSION_STRUCT {
        xmlChar *extension;
        const xmlChar *type; // owned by opcContainerType
    } opcContainerExtension;


    typedef struct OPC_CONTAINER_INPUTSTREAM_STRUCT {
        opcZipSegmentInputStream *segmentInputStream;
        opc_uint32_t segment_id;
        opcContainer *container; // weak reference
        xmlTextReaderPtr reader; // in case we have an xmlTextReader associated
        opc_error_t error;
        opc_uint32_t reader_consume_element : 1;
        opc_uint32_t reader_element_handled : 1;
    } opcContainerInputStream;


    struct OPC_CONTAINER_STRUCT {
        opcContainerPart *part_array;
        opc_uint32_t part_items;
        opcContainerSegment *segment_array;
        opc_uint32_t segment_items;
        opcContainerRelPrefix *relprefix_array;
        opc_uint32_t relprefix_items;
        opcContainerType *type_array;
        opc_uint32_t type_items;
        opcContainerExtension *extension_array;
        opc_uint32_t extension_items;
        opcContainerInputStream **inputstream_array;
        opc_uint32_t inputstream_items;
        opcZip *zip;
        opc_uint32_t content_types_segment_id;
        opc_uint32_t rels_segment_id;
    };

    opc_error_t opcContainerFree(opcContainer *c);


    opcContainerInputStream* opcContainerOpenInputStreamEx(opcContainer *container, opc_uint32_t segment_id);
    opcContainerInputStream* opcContainerOpenInputStream(opcContainer *container, xmlChar *name);
    opc_uint32_t opcContainerReadInputStream(opcContainerInputStream* stream, opc_uint8_t *buffer, opc_uint32_t buffer_len);
    opc_error_t opcContainerCloseInputStream(opcContainerInputStream* stream);
    opcXmlReader* opcXmlReaderOpenEx(opcContainer *container, opc_uint32_t segment_id, const char * URL, const char * encoding, int options);

    opcContainerExtension *opcContainerInsertExtension(opcContainer *container, const xmlChar *extension, opc_bool_t insert);
    opcContainerPart *opcContainerInsertPart(opcContainer *container, const xmlChar *name, opc_bool_t insert);

#ifdef __cplusplus
} /* extern "C" */
#endif    

#endif /* OPC_INTERNAL_H */
