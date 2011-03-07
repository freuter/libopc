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
#include <opc/opc.h>

typedef struct OPC_CONTAINER_PART_STRUCT {
    xmlChar *name;
    opc_uint32_t first_segment_id;
    opc_uint32_t last_segment_id;
    opc_uint32_t segment_count;
} opcContainerPart;

typedef struct OPC_CONTAINER_SEGMENT_STRUCT {
    opcZipSegment zipSegment;
    opc_uint32_t part_id;
    opc_uint32_t next_segment_id;
} opcContainerSegment;

struct OPC_CONTAINER_STRUCT {
    opcContainerPart *part_array;
    opc_uint32_t part_items;
    opcContainerSegment *segment_array;
    opc_uint32_t segment_items;
    opcZip *zip;
};

static void* ensureItem(void **array_, puint32_t items, puint32_t item_size) {
    *array_=xmlRealloc(*array_, (items+1)*item_size);
    return *array_;
}

static opcContainerPart* ensurePart(opcContainer *container) {
    return ((opcContainerPart*)ensureItem((void**)&container->part_array, container->part_items, sizeof(opcContainerPart)))+container->part_items;
}

static opcContainerSegment* ensureSegment(opcContainer *container) {
    return ((opcContainerSegment*)ensureItem((void**)&container->segment_array, container->segment_items, sizeof(opcContainerSegment)))+container->segment_items;
}


opcContainer* opcContainerOpen(const xmlChar *fileName, 
                               opcContainerOpenMode mode, 
                               void *userContext, 
                               const xmlChar *destName) {
    opcContainer *c=(opcContainer *)xmlMalloc(sizeof(opcContainer));
    if (NULL!=c) {
        opc_bzero_mem(c, sizeof(*c));
        int open_flags=OPC_ZIP_READ;
        c->zip=opcZipOpenFile(fileName, OPC_ZIP_READ);
        if (NULL!=c->zip) {
            // scan local files
            opcZipRawBuffer rawBuffer;
            OPC_ENSURE(OPC_ERROR_NONE==opcZipInitRawBuffer(c->zip, &rawBuffer));
            opcContainerSegment *segment=NULL;
            xmlChar *name=NULL;
            opc_uint32_t segment_number;
            opc_bool_t last_segment;
            while(NULL!=(segment=ensureSegment(c)) && opcZipRawReadLocalFile(c->zip, &rawBuffer, &segment->zipSegment, &name, &segment_number, &last_segment)) {
                opc_uint32_t segment_id=c->segment_items++;
                opcContainerPart *part=NULL;
                if (segment_number>0) {
                    // find part for segment
                    for(opc_uint32_t i=0;NULL==part && i<c->part_items;i++) {
                        if (0==xmlStrcmp(c->part_array[i].name, name)) {
                            part=&c->part_array[i];
                            xmlFree(name); name=NULL; // not needed anymore
                        }
                    }
                } else {
                    part=ensurePart(c);
                    opc_bzero_mem(part, sizeof(*part));
                    part->first_segment_id=-1;
                    part->last_segment_id=-1;
                    c->part_items++;
                    part->name=name; name=NULL; // transfer of ownership
                }
                OPC_ASSERT(NULL!=part);
                if (NULL!=part) {
                    if (-1==part->first_segment_id) {
                        OPC_ASSERT(-1==part->last_segment_id);
                        part->first_segment_id=segment_id;
                    }
                    segment->next_segment_id=part->last_segment_id;
                    part->last_segment_id=segment_id;
                    OPC_ASSERT(segment_number==part->segment_count); // segments out of order?
                    part->segment_count++;
                } else {
                    xmlFree(name); name=NULL; // error
                }
                OPC_ENSURE(OPC_ERROR_NONE==opcZipRawSkipFileData(c->zip, &rawBuffer, &segment->zipSegment));
                OPC_ENSURE(OPC_ERROR_NONE==opcZipRawReadDataDescriptor(c->zip, &rawBuffer, &segment->zipSegment));
            }
        } else {
            xmlFree(c); c=NULL; // error
        }
    }
    return c;
}

opc_error_t opcContainerClose(opcContainer *c, opcContainerCloseMode mode) {
    if (NULL!=c) {
        for(opc_uint32_t i=0;i<c->segment_items;i++) {
        }
        for(opc_uint32_t i=0;i<c->part_items;i++) {
            xmlFree(c->part_array[i].name);
        }
        xmlFree(c->segment_array);
        xmlFree(c->part_array);
        opcZipClose(c->zip);
        xmlFree(c);
    }
    return OPC_ERROR_NONE;
}


opc_error_t opcContainerDump(opcContainer *c, FILE *out) {
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        fprintf(out, "%s\n", c->part_array[i].name);
    }
    return OPC_ERROR_NONE;
}




