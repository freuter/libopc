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
#include "internal.h"

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

static opcContainerType* ensureType(opcContainer *container) {
    return ((opcContainerType*)ensureItem((void**)&container->type_array, container->type_items, sizeof(opcContainerType)))+container->type_items;
}

static opcContainerExtension* ensureExtension(opcContainer *container) {
    return ((opcContainerExtension*)ensureItem((void**)&container->extension_array, container->extension_items, sizeof(opcContainerExtension)))+container->extension_items;
}

static opcContainerInputStream** ensureInputStream(opcContainer *container) {
    return ((opcContainerInputStream**)ensureItem((void**)&container->inputstream_array, container->inputstream_items, sizeof(opcContainerInputStream*)))+container->inputstream_items;
}


static opc_bool_t findItem(void *array_, opc_uint32_t items, const void *key, int (*cmp_fct)(const void *key, const void *array_, opc_uint32_t item), opc_uint32_t *pos) {
    opc_uint32_t i=0;
    opc_uint32_t j=items;
    while(i<j) {
        opc_uint32_t m=i+(j-i)/2;
        OPC_ASSERT(i<=m && m<j);
        int cmp=cmp_fct(key, array_, m);
        if (cmp<0) {
            j=m;
        } else if (cmp>0) {
            i=m+1;
        } else {
            *pos=m;
            return OPC_TRUE;
        }
    }
    OPC_ASSERT(i==j); 
    *pos=i;
    return OPC_FALSE;
}

#define ensureGap(container, array_, items_, i) \
{\
    for (opc_uint32_t k=container->items_;k>i;k--) { \
        container->array_[k]=container->array_[k-1];\
    }\
    container->items_++;\
    OPC_ASSERT(i>=0 && i<container->items_);\
    opc_bzero_mem(&container->array_[i], sizeof(container->array_[i]));\
}


static inline int part_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerPart*)array_)[item].name);
}

opcContainerPart *opcContainerInsertPart(opcContainer *container, const xmlChar *name, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->part_array, container->part_items, name, part_cmp_fct, &i)) {
        return &container->part_array[i];
    } else if (insert && NULL!=ensurePart(container)) {
        ensureGap(container, part_array, part_items, i);
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


static inline int type_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerType*)array_)[item].type);
}
static opcContainerType *insertType(opcContainer *container, const xmlChar *type, const xmlChar *basedOn, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->type_array, container->type_items, type, type_cmp_fct, &i)) {
        return &container->type_array[i];
    } else if (insert && NULL!=ensureType(container)) {
        ensureGap(container, type_array, type_items, i);
        container->type_array[i].type=xmlStrdup(type);
        container->type_array[i].basedOn=xmlStrdup(basedOn);
        return &container->type_array[i];
    } else {
        return NULL;
    }
}

static inline int extension_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerExtension*)array_)[item].extension);
}
opcContainerExtension *opcContainerInsertExtension(opcContainer *container, const xmlChar *extension, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->extension_array, container->extension_items, extension, extension_cmp_fct, &i)) {
        return &container->extension_array[i];
    } else if (insert && NULL!=ensureExtension(container)) {
        ensureGap(container, extension_array, extension_items, i);
        container->extension_array[i].extension=xmlStrdup(extension);
        return &container->extension_array[i];
    } else {
        return NULL;
    }
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
                    opcContainerPart *part=opcContainerInsertPart(c, name, 0==segment_number /* only append, if segment number is 0 */);
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
            if (1) { // check central directory
                opcZipSegment segment;
                xmlChar *name=NULL;
                opc_uint32_t segment_number;
                opc_bool_t last_segment;
                opc_bool_t rel_segment;
                while(opcZipRawReadCentralDirectory(c->zip, &rawBuffer, &segment, &name, &segment_number, &last_segment, &rel_segment)) {
                    if (name[0]!=0 && xmlStrcmp(name, _X("[Content_Types].xml"))) {
                        opcContainerPart *part=opcContainerInsertPart(c, name, OPC_FALSE);
                        OPC_ASSERT(NULL!=part); // not found in central dir??? why?
                    }
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupSegment(&segment));
                    xmlFree(name);
                }
                opc_uint16_t central_dir_entries=0;
                if(!opcZipRawReadEndOfCentralDirectory(c->zip, &rawBuffer, &central_dir_entries)) {
                    opcContainerFree(c); c=NULL; // error
                }
            }
            if (-1!=c->content_types_segment_id) {
                /*
                opcContainerInputStream* stream=opcContainerOpenInputStreamEx(c, c->content_types_segment_id);
                if (NULL!=stream) {
                    opc_uint8_t buf[1024];
                    opc_uint32_t len=0;
                    while((len=opcContainerReadInputStream(stream, buf, sizeof(buf)))>0) {
                        printf("%.*s", len, buf);
                    }
                    opcContainerCloseInputStream(stream);
                }
                */
                opcXmlReader *reader=opcXmlReaderOpenEx(c, c ->content_types_segment_id, NULL, NULL, 0);
                static char ns[]="http://schemas.openxmlformats.org/package/2006/content-types";
                opc_xml_start_document(reader) {
                    opc_xml_element(reader, _X(ns), _X("Types")) {
                        opc_xml_start_attributes(reader) {
                        } opc_xml_end_attributes(reader);
                        opc_xml_start_children(reader) {
                            opc_xml_element(reader, NULL, _X("Default")) {
                                const xmlChar *ext=NULL;
                                const xmlChar *type=NULL;
                                opc_xml_start_attributes(reader) {
                                    opc_xml_attribute(reader, NULL, _X("Extension")) {
                                        ext=opc_xml_const_value(reader);
                                    } else opc_xml_attribute(reader, NULL, _X("ContentType")) {
                                        type=opc_xml_const_value(reader);
                                    }
                                } opc_xml_end_attributes(reader);
                                opc_xml_error_guard_start(reader) {
                                  opc_xml_error(reader, NULL==ext || ext[0]==0, OPC_ERROR_XML, "Missing @Extension attribute!");
                                  opc_xml_error(reader, NULL==type || type[0]==0, OPC_ERROR_XML, "Missing @ContentType attribute!");
                                  opcContainerType *ct=insertType(c, type, NULL, OPC_TRUE);
                                  opc_xml_error(reader, NULL==ct, OPC_ERROR_MEMORY, NULL);
                                  opcContainerExtension *ce=opcContainerInsertExtension(c, ext, OPC_TRUE);
                                  opc_xml_error(reader, NULL==ce, OPC_ERROR_MEMORY, NULL);
                                  opc_xml_error(reader, NULL!=ce->type && 0!=xmlStrcmp(ce->type, type), OPC_ERROR_XML, "Extension \"%s\" is mapped to type \"%s\" as well as \"%s\"", ext, type, ce->type);
                                  ce->type=ct->type;
                                } opc_xml_error_guard_end(reader);
                                opc_xml_start_children(reader) {
                                } opc_xml_end_children(reader);
                            } else opc_xml_element(reader, NULL, _X("Override")) {
                                const xmlChar *name=NULL;
                                const xmlChar *type=NULL;
                                opc_xml_start_attributes(reader) {
                                    opc_xml_attribute(reader, NULL, _X("PartName")) {
                                        name=opc_xml_const_value(reader);
                                    } else opc_xml_attribute(reader, NULL, _X("ContentType")) {
                                        type=opc_xml_const_value(reader);
                                    }
                                } opc_xml_end_attributes(reader);
                                opc_xml_error_guard_start(reader) {
                                    opcContainerType*ct=insertType(c, type, NULL, OPC_TRUE);
                                    opc_xml_error(reader, NULL==ct, OPC_ERROR_MEMORY, NULL);
                                    opc_xml_error_strict(reader, '/'!=name[0], OPC_ERROR_XML, "Part %s MUST start with a '/'", name);
                                    opcContainerPart *part=opcContainerInsertPart(c, (name[0]=='/'?name+1:name), OPC_FALSE);
                                    opc_xml_error_strict(reader, NULL==part, OPC_ERROR_XML, "Part %s does not exist.", name);
                                    if (NULL!=part) {
                                        part->type=ct->type;
                                    }
                                } opc_xml_error_guard_end(reader);
                                opc_xml_start_children(reader) {
                                } opc_xml_end_children(reader);
                            }
                        } opc_xml_end_children(reader);
                    } 
                } opc_xml_end_document(reader);
                OPC_ENSURE(OPC_ERROR_NONE==opcXmlReaderClose(reader));
            }
        } else {
            xmlFree(c); c=NULL; // error
        }
    }
    return c;
}

opc_error_t opcContainerFree(opcContainer *c) {
    if (NULL!=c) {
        for(opc_uint32_t i=0;i<c->inputstream_items;i++) {
            OPC_ASSERT(NULL!=c->inputstream_array[i] && NULL==c->inputstream_array[i]->segmentInputStream); // not closed???
            xmlFree(c->inputstream_array[i]); c->inputstream_array[i]=NULL;
        }
        for(opc_uint32_t i=0;i<c->segment_items;i++) {
        }
        for(opc_uint32_t i=0;i<c->extension_items;i++) {
            xmlFree(c->extension_array[i].extension);
        }
        for(opc_uint32_t i=0;i<c->type_items;i++) {
            xmlFree(c->type_array[i].type);
            xmlFree(c->type_array[i].basedOn);
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

opc_error_t opcContainerClose(opcContainer *c, opcContainerCloseMode mode) {
    opc_error_t err=OPC_ERROR_NONE;
    err=(OPC_ERROR_NONE==err?opcContainerFree(c):err);
    return err;
}


opc_error_t opcContainerDump(opcContainer *c, FILE *out) {
    fprintf(out, "Content Types:\n");
    for(opc_uint32_t i=0;i<c->type_items;i++) {
        fprintf(out, "%s %s\n", c->type_array[i].type, c->type_array[i].basedOn);
    }
    fprintf(out, "Registered Extensions:\n");
    for(opc_uint32_t i=0;i<c->extension_items;i++) {
        fprintf(out, "%s %s\n", c->extension_array[i].extension, c->extension_array[i].type);
    }
    fprintf(out, "Parts:\n");
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        const xmlChar *type=opcPartGetType(c, i);
        fprintf(out, "%s [%s]\n", c->part_array[i].name, type);
    }
    return OPC_ERROR_NONE;
}

opcContainerInputStream* opcContainerOpenInputStreamEx(opcContainer *container, opc_uint32_t segment_id) {
    OPC_ASSERT(segment_id>=0 && segment_id<container->segment_items);
    opcContainerInputStream **stream=NULL;
    //@TODO search for closed stream we can reuse
    if (segment_id>=0 && segment_id<container->segment_items && NULL!=(stream=ensureInputStream(container))) {
        *stream=(opcContainerInputStream*)xmlMalloc(sizeof(opcContainerInputStream));
        if (NULL!=*stream) {
            opc_bzero_mem((*stream), sizeof(*(*stream)));
            if (NULL!=((*stream)->segmentInputStream=opcZipCreateSegmentInputStream(container->zip, &container->segment_array[segment_id].zipSegment))) {
                container->inputstream_items++;
                (*stream)->container=container;
                (*stream)->segment_id=segment_id;
            } else {
                xmlFree(*stream); *stream=NULL;
            }
        }
    }
    return (NULL!=stream?*stream:NULL);
}

opcContainerInputStream* opcContainerOpenInputStream(opcContainer *container, xmlChar *name) {
    opcContainerPart *part=opcContainerInsertPart(container, name, OPC_FALSE);
    if (NULL!=part) {
        return opcContainerOpenInputStreamEx(container, part->first_segment_id);
    } else {
        return NULL;
    }
}

opc_uint32_t opcContainerReadInputStream(opcContainerInputStream* stream, opc_uint8_t *buffer, opc_uint32_t buffer_len) {
    opc_uint32_t ret=0;
    if (NULL!=stream) {
        ret=opcZipReadSegmentInputStream(stream->container->zip, stream->segmentInputStream, buffer, buffer_len);
    }
    return ret;
}

opc_error_t opcContainerCloseInputStream(opcContainerInputStream* stream) {
    opc_error_t ret=OPC_ERROR_STREAM;
    if (NULL!=stream) {
        ret=opcZipCloseSegmentInputStream(stream->container->zip, &stream->container->segment_array[stream->segment_id].zipSegment, stream->segmentInputStream);
        stream->segmentInputStream=NULL;
    }
    return ret;
}






