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
    opc_uint32_t rel_segment_id;
} opcContainerPart;

typedef struct OPC_CONTAINER_SEGMENT_STRUCT {
    opcZipSegment zipSegment;
    xmlChar *part_name; // owned by part
    opc_uint32_t next_segment_id;
} opcContainerSegment;

typedef struct OPC_CONTAINER_PART_PREFIX_STRUCT {
    xmlChar *prefix;
} opcContainerRelPrefix;

#define OPC_CONTAINER_RELID_COUNTER(part) ((part)&0xFFFF)
#define OPC_CONTAINER_RELID_PREFIX(part) (((part)>>16)&0xFFFF)

struct OPC_CONTAINER_STRUCT {
    opcContainerPart *part_array;
    opc_uint32_t part_items;
    opcContainerSegment *segment_array;
    opc_uint32_t segment_items;
    opcContainerRelPrefix *relprefix_array;
    opc_uint32_t relprefix_items;
    opcZip *zip;
    opc_uint32_t content_types_segment_id;
    opc_uint32_t rels_segment_id;
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

static opcContainerRelPrefix* ensureRelPrefix(opcContainer *container) {
    return ((opcContainerRelPrefix*)ensureItem((void**)&container->relprefix_array, container->relprefix_items, sizeof(opcContainerRelPrefix)))+container->relprefix_items;
}


static opcContainerPart *insertPart(opcContainer *container, xmlChar *name, opc_bool_t insert) {
    opc_uint32_t i=0;
    opc_uint32_t j=container->part_items;
    while(i<j) {
        opc_uint32_t m=i+(j-i)/2;
        OPC_ASSERT(i<=m && m<j);
        int cmp=xmlStrcmp(name, container->part_array[m].name);
        if (cmp<0) {
            j=m;
        } else if (cmp>0) {
            i=m+1;
        } else {
            return &container->part_array[m];
        }
    }
    OPC_ASSERT(i==j); 
    if (insert && NULL!=ensurePart(container)) {
        for (opc_uint32_t k=container->part_items;k>i;k--) {
            container->part_array[k]=container->part_array[k-1];
        }
        container->part_items++;
        OPC_ASSERT(i>=0 && i<container->part_items);
        opc_bzero_mem(&container->part_array[i], sizeof(container->part_array[i]));
        container->part_array[i].first_segment_id=-1;
        container->part_array[i].last_segment_id=-1;
        container->part_array[i].name=xmlStrdup(name); 
        container->part_array[i].rel_segment_id=-1;
        return &container->part_array[i];
    } else {
        return NULL;
    }
}

#define OPC_MAX_UINT16 65535
static opc_uint32_t insertRelPrefix(opcContainer *container, xmlChar *relPrefix) {
    opc_uint32_t ret=-1;
    opc_uint32_t i=container->relprefix_items; for(;i>0 && 0!=xmlStrcmp(container->relprefix_array[i-1].prefix, relPrefix);i--);
    if (i<container->relprefix_items && 0==xmlStrcmp(container->relprefix_array[i].prefix, relPrefix)) {
        return i;
    } else {
        if (container->relprefix_items<OPC_MAX_UINT16 && NULL!=ensureRelPrefix(container)) {
            i=container->relprefix_items++;
            container->relprefix_array[i].prefix=relPrefix;
            return i;
        } else {
            return -1; // error
        }
    }
}

static opc_uint32_t splitRelPrefix(opcContainer *container, xmlChar *rel, opc_uint32_t *counter) {
    opc_uint32_t len=xmlStrlen(rel);
    while(len>0 && rel[len-1]>='0' && rel[len-1]<='9') len--;
    if (NULL!=counter) {
        if (rel[len]!=0) {
            *counter=atoi((char*)(rel+len));
        } else {
            *counter=-1; // no counter
        }
    }
    return len;
}

static opc_uint32_t createRelId(opcContainer *container, xmlChar *relPrefix, opc_uint16_t relCounter) {
    opc_uint32_t ret=-1;
    opc_uint32_t prefix=insertRelPrefix(container, relPrefix);
    if (-1!=prefix) {
        ret=relCounter;
        ret|=prefix>>16;
    } 
    return ret;
}

opcContainer* opcContainerOpen(const xmlChar *fileName, 
                               opcContainerOpenMode mode, 
                               void *userContext, 
                               const xmlChar *destName) {
    opcContainer *c=(opcContainer *)xmlMalloc(sizeof(opcContainer));
    if (NULL!=c) {
        opc_bzero_mem(c, sizeof(*c));
        int open_flags=OPC_ZIP_READ;
        c->rels_segment_id=-1;
        c->content_types_segment_id=-1;
        c->zip=opcZipOpenFile(fileName, OPC_ZIP_READ);
        if (NULL!=c->zip) {
            // scan local files
            opcZipRawBuffer rawBuffer;
            OPC_ENSURE(OPC_ERROR_NONE==opcZipInitRawBuffer(c->zip, &rawBuffer));
            opcContainerSegment *segment=NULL;
            xmlChar *name=NULL;
            opc_uint32_t segment_number;
            opc_bool_t last_segment;
            opc_bool_t rel_segment;
            while(NULL!=(segment=ensureSegment(c)) && opcZipRawReadLocalFile(c->zip, &rawBuffer, &segment->zipSegment, &name, &segment_number, &last_segment, &rel_segment)) {
                opc_uint32_t segment_id=c->segment_items++;
                if (name[0]==0) {
                    OPC_ASSERT(rel_segment); // empty names are not allowed!
                    OPC_ASSERT(-1==c->rels_segment_id); // set twice??? why?
                    c->rels_segment_id=segment_id;
                    segment->next_segment_id=-1;
                } else if (!rel_segment && 0==xmlStrcmp(name, _X("[Content_Types].xml"))) {
                    OPC_ASSERT(-1==c->content_types_segment_id); // set twice??? why?
                    c->content_types_segment_id=segment_id;
                    segment->next_segment_id=-1;
                } else {
                    opcContainerPart *part=insertPart(c, name, 0==segment_number /* only append, if segment number is 0 */);
                    OPC_ASSERT(NULL!=part);
                    if (NULL!=part) {
                        if (rel_segment) {
                            OPC_ASSERT(-1==part->rel_segment_id); // set twice??? why?
                            part->rel_segment_id=segment_id;
                            segment->next_segment_id=-1;
                        } else {
                            if (-1==part->first_segment_id) {
                                OPC_ASSERT(-1==part->last_segment_id);
                                part->first_segment_id=segment_id;
                            }
                            segment->next_segment_id=part->last_segment_id;
                            part->last_segment_id=segment_id;
                            OPC_ASSERT(segment_number==part->segment_count); // segments out of order?
                            part->segment_count++;
                        }
                        segment->part_name=part->name;
                    }
                }
                xmlFree(name); name=NULL;
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
        for(opc_uint32_t i=0;i<c->relprefix_items;i++) {
            xmlFree(c->relprefix_array[i].prefix);
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




