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

static opcContainerRelationType* ensureRelationType(opcContainer *container) {
    return ((opcContainerRelationType*)ensureItem((void**)&container->relationtype_array, container->relationtype_items, sizeof(opcContainerRelationType)))+container->relationtype_items;
}

static opcContainerExternalRelation* ensureExternalRelation(opcContainer *container) {
    return ((opcContainerExternalRelation*)ensureItem((void**)&container->externalrelation_array, container->externalrelation_items, sizeof(opcContainerRelationType)))+container->externalrelation_items;
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
static opc_uint32_t insertRelPrefix(opcContainer *container, const xmlChar *relPrefix) {
    opc_uint32_t ret=-1;
    opc_uint32_t i=container->relprefix_items; 
    for(;i>0 && 0!=xmlStrcmp(container->relprefix_array[i-1].prefix, relPrefix);) {
        i--;
    };
    if (i>0) {
        OPC_ASSERT(0==xmlStrcmp(container->relprefix_array[i-1].prefix, relPrefix));
        return i-1;
    } else {
        if (container->relprefix_items<OPC_MAX_UINT16 && NULL!=ensureRelPrefix(container)) {
            i=container->relprefix_items++;
            container->relprefix_array[i].prefix=xmlStrdup(relPrefix);
            return i;
        } else {
            return -1; // error
        }
    }
}

static opc_uint32_t splitRelPrefix(opcContainer *container, const xmlChar *rel, opc_uint32_t *counter) {
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

static opc_uint32_t createRelId(opcContainer *container, const xmlChar *relPrefix, opc_uint16_t relCounter) {
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

static inline int relationtype_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerRelationType*)array_)[item].type);
}
static opcContainerRelationType *insertRelationType(opcContainer *container, const xmlChar *type, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->relationtype_array, container->relationtype_items, type, relationtype_cmp_fct, &i)) {
        return &container->relationtype_array[i];
    } else if (insert && NULL!=ensureRelationType(container)) {
        ensureGap(container, relationtype_array, relationtype_items, i);
        container->relationtype_array[i].type=xmlStrdup(type);
        return &container->relationtype_array[i];
    } else {
        return NULL;
    }
}

static inline int externalrelation_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerExternalRelation*)array_)[item].target);
}
static opcContainerExternalRelation*insertExternalRelation(opcContainer *container, const xmlChar *target, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->externalrelation_array, container->externalrelation_items, target, externalrelation_cmp_fct, &i)) {
        return &container->externalrelation_array[i];
    } else if (insert && NULL!=ensureExternalRelation(container)) {
        ensureGap(container, externalrelation_array, externalrelation_items, i);
        container->externalrelation_array[i].target=xmlStrdup(target);
        return &container->externalrelation_array[i];
    } else {
        return NULL;
    }
}


static inline int relation_cmp_fct(const void *key, const void *array_, opc_uint32_t item) {
    return (opc_uint32_t)key-((opcContainerRelation*)array_)[item].relation_id;
}
static opcContainerRelation *insertRelation(opcContainerRelation **relation_array, opc_uint32_t *relation_items, 
                                            opc_uint32_t relation_id,
                                            xmlChar *relation_type,
                                            opc_uint32_t target_mode, xmlChar *target_ptr) {
    opc_uint32_t i=0;
    if (relation_items>0) {
        opc_bool_t ret=findItem(*relation_array, *relation_items, (void*)relation_id, relation_cmp_fct, &i);
        if (ret) { // error, relation already exists!
            return NULL;
        }
    }
    if (NULL!=ensureItem((void**)relation_array, *relation_items, sizeof(opcContainerRelation))) {
        for (opc_uint32_t k=(*relation_items);k>i;k--) { 
            (*relation_array)[k]=(*relation_array)[k-1];
        }
        (*relation_items)++;
        OPC_ASSERT(i>=0 && i<(*relation_items));\
        opc_bzero_mem(&(*relation_array)[i], sizeof((*relation_array)[i]));\
        (*relation_array)[i].relation_id=relation_id;
        (*relation_array)[i].relation_type=relation_type;
        (*relation_array)[i].target_mode=target_mode;
        (*relation_array)[i].target_ptr=target_ptr;
        return &(*relation_array)[i];
    } else {
        return NULL; // memory error!
    }
}


void opc_container_normalize_part_to_helper_buffer(opcContainer *c,
                                                   xmlChar *buf, int buf_len,
                                                   const xmlChar *base,
                                                   const xmlChar *name) {
  int j=xmlStrlen(base);
  int i=0;
  OPC_ASSERT(j<=buf_len);
  if (j>0) {
    memcpy(buf, base, j*sizeof(xmlChar));
  }
  while(j>0 && buf[j-1]!='/') j--;  // so make sure base has a trailing "/"
  
  while(name[i]!=0) {
    if (name[i]=='/') {
      j=0; /* absolute path */
      while (name[i]=='/') i++;
    } else if (name[i]=='.' && name[i+1]=='/') {
      /* skip */
      i+=1;
      while (name[i]=='/') i++;
    } else if (name[i]=='.' && name[i+1]=='.' && name[i+2]=='/') {
      while(j>0 &&  buf[j-1]=='/') j--; /* skip base '/' */
      while(j>0 &&  buf[j-1]!='/') j--; /* navigate one dir up */
      i+=2;
      while (name[i]=='/') i++;
    } else {
      /* copy step */
      OPC_ASSERT(j+1<=buf_len);
      while(j+1<buf_len && name[i]!=0 && name[i]!='/') {
        buf[j++]=name[i++];
      }
      if (name[i]=='/' && j+1<buf_len) {
        buf[j++]='/';
      }
      while (name[i]=='/') i++;
    }
  }
  OPC_ASSERT(j+1<buf_len);
  buf[j]=0;
}


void opcConstainerParseRels(opcContainer *c, const xmlChar *base, opc_uint32_t rel_segment_id, opcContainerRelation **relation_array, opc_uint32_t *relation_items) {
    opcXmlReader *reader=opcXmlReaderOpenEx(c, rel_segment_id, NULL, NULL, 0);
    static char ns[]="http://schemas.openxmlformats.org/package/2006/relationships";
    opc_xml_start_document(reader) {
        opc_xml_element(reader, _X(ns), _X("Relationships")) {
            opc_xml_start_attributes(reader) {
            } opc_xml_end_attributes(reader);
            opc_xml_start_children(reader) {
                opc_xml_element(reader, NULL, _X("Relationship")) {
                    const xmlChar *id=NULL;
                    const xmlChar *type=NULL;
                    const xmlChar *target=NULL;
                    const xmlChar *mode=NULL;
                    opc_xml_start_attributes(reader) {
                        opc_xml_attribute(reader, NULL, _X("Id")) {
                            id=opc_xml_const_value(reader);
                        } else opc_xml_attribute(reader, NULL, _X("Type")) {
                            type=opc_xml_const_value(reader);
                        } else opc_xml_attribute(reader, NULL, _X("Target")) {
                            target=opc_xml_const_value(reader);
                        } else opc_xml_attribute(reader, NULL, _X("TargetMode")) {
                            mode=opc_xml_const_value(reader);
                        }
                    } opc_xml_end_attributes(reader);
                    opc_xml_error_guard_start(reader) {
                        opc_xml_error(reader, NULL==id || id[0]==0, OPC_ERROR_XML, "Missing @Id attribute!");
                        opc_xml_error(reader, NULL==type || type[0]==0, OPC_ERROR_XML, "Missing @Type attribute!");
                        opc_xml_error(reader, NULL==target || target[0]==0, OPC_ERROR_XML, "Missing @Id attribute!");
                        opcContainerRelationType *rel_type=insertRelationType(c, type, OPC_TRUE);
                        opc_xml_error(reader, NULL==rel_type, OPC_ERROR_MEMORY, NULL);
                        opc_uint32_t counter=-1;
                        opc_uint32_t id_len=splitRelPrefix(c, id, &counter);
                        ((xmlChar *)id)[id_len]=0;
                        opc_uint32_t rel_id=createRelId(c, id, counter);
                        if (NULL==mode || 0==xmlStrcasecmp(mode, _X("Internal"))) {
                            xmlChar target_part_name[OPC_MAXPATH];
                            opc_container_normalize_part_to_helper_buffer(c, target_part_name, sizeof(target_part_name), base, target);
    //                        printf("%s (%s;%s)\n", target_part_name, base, target);
                            opcContainerPart *target_part=opcContainerInsertPart(c, target_part_name, OPC_FALSE);
                            opc_xml_error(reader, NULL==target_part, OPC_ERROR_XML, "Referenced part %s (%s;%s) does not exists!", target_part_name, base, target);
//                            printf("%s %i %s %s\n", id, counter, rel_type->type, target_part->name);
                            opcContainerRelation *rel=insertRelation(relation_array, relation_items, rel_id, rel_type->type, 0, target_part->name);
                            OPC_ASSERT(NULL!=rel);
                        } else if (0==xmlStrcasecmp(mode, _X("External"))) {
                            opcContainerExternalRelation *ext_rel=insertExternalRelation(c, target, OPC_TRUE);
                            opc_xml_error(reader, NULL==ext_rel, OPC_ERROR_MEMORY, NULL);
                            opcContainerRelation *rel=insertRelation(relation_array, relation_items, rel_id, rel_type->type, 1, ext_rel->target);
                            OPC_ASSERT(NULL!=rel);
                        } else {
                            opc_xml_error(reader, OPC_TRUE, OPC_ERROR_XML, "TargetMode %s unknown!\n", mode);
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
            if (-1!=c->rels_segment_id) {
                opcConstainerParseRels(c, _X(""), c->rels_segment_id, &c->relation_array, &c->relation_items);
            }
            for(opc_uint32_t i=0;i<c->part_items;i++) {
                opcContainerPart *part=&c->part_array[i];
                if (-1!=part->rel_segment_id) {
                    opcConstainerParseRels(c, part->name, part->rel_segment_id, &part->relation_array, &part->relation_items);
                }
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
        for(opc_uint32_t i=0;i<c->relationtype_items;i++) {
            xmlFree(c->relationtype_array[i].type);
        }
        for(opc_uint32_t i=0;i<c->externalrelation_items;i++) {
            xmlFree(c->externalrelation_array[i].target);
        }
        xmlFree(c->relation_array);
        for(opc_uint32_t i=0;i<c->part_items;i++) {
            xmlFree(c->part_array[i].relation_array);
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

static void opcContainerDumpString(FILE *out, const xmlChar *str, opc_uint32_t max_len) {
    opc_uint32_t len=xmlStrlen(str);
    if (len<=max_len) {
        fputs((const char *)str, out);
        for(opc_uint32_t i=len;i<max_len;i++) fputc(' ', out);
    } else {
        static const char prefix[]="...";
        static opc_uint32_t prefix_len=sizeof(prefix)-1;
        opc_uint32_t ofs=len-max_len;
        if (ofs+prefix_len<len) ofs+=prefix_len; else ofs=len;
        fputs(prefix, out);
        fputs((const char *)(str+ofs), out);
    }
}

static void opcContainerDumpType(opcContainer *c, FILE *out, opcContainerType *type, opc_uint32_t level) {
    if (NULL!=type->basedOn) {
        opcContainerType *baseType=insertType(c, type->basedOn, NULL, OPC_FALSE);
    }
    if (level>0) {
        for(opc_uint32_t i=0;i<level;i++) fprintf(out, " ");
        fputs("|", out);
    }
    opcContainerDumpString(out, type->type, 79-(level>0?1+level:0));
    fputs("\n", out);
}

static void opcContainerDumpRel(opcContainer *c, FILE *out, opcContainerRelation *rel, opc_uint32_t max_part_name_len) {
    fputs("[", out);
    OPC_ASSERT(OPC_CONTAINER_RELID_PREFIX(rel->relation_id)>=0 && OPC_CONTAINER_RELID_PREFIX(rel->relation_id)<c->relation_items);
    opc_uint32_t l=xmlStrlen(c->relprefix_array[OPC_CONTAINER_RELID_PREFIX(rel->relation_id)].prefix);
    fputs((const char *)c->relprefix_array[OPC_CONTAINER_RELID_PREFIX(rel->relation_id)].prefix, out);
    if (-1!=OPC_CONTAINER_RELID_COUNTER(rel->relation_id)) {
        l+=fprintf(out, "%i", OPC_CONTAINER_RELID_COUNTER(rel->relation_id));
    }
    fputs("]", out); 
    l+=2;
    opcContainerDumpString(out, rel->target_ptr, max_part_name_len);
    opc_uint32_t type_len=(l+max_part_name_len<79?79-(l+max_part_name_len):10);
    opcContainerDumpString(out, rel->relation_type, type_len);
    fputs("\n", out);

}

opc_error_t opcContainerDump(opcContainer *c, FILE *out) {
    fprintf(out, "Content Types:\n");
    fprintf(out, "-------------------------------------------------------------------------------\n");
    for(opc_uint32_t i=0;i<c->type_items;i++) {
        opcContainerDumpType(c, out, &c->type_array[i], 0);
    }
    fputs("\n", out);
    fprintf(out, "Registered Extensions:\n");
    fprintf(out, "-------------------------------------------------------------------------------\n");
    for(opc_uint32_t i=0;i<c->extension_items;i++) {
        opcContainerDumpString(out, c->extension_array[i].extension, 11);
        opcContainerDumpString(out, c->extension_array[i].type, 68);
        fputs("\n", out);
    }
    fputs("\n", out);
    fprintf(out, "Relation Types:\n");
    fprintf(out, "-------------------------------------------------------------------------------\n");
    for(opc_uint32_t i=0;i<c->relationtype_items;i++) {
        opcContainerDumpString(out, c->relationtype_array[i].type, 79);
        fputs("\n", out);
    }
    fputs("\n", out);
    fprintf(out, "External Relations:\n");
    fprintf(out, "-------------------------------------------------------------------------------\n");
    for(opc_uint32_t i=0;i<c->externalrelation_items;i++) {
        opcContainerDumpString(out, c->externalrelation_array[i].target, 79);
        fputs("\n", out);
    }
    fputs("\n", out);
    fprintf(out, "Parts:               Type:\n");
    fprintf(out, "-------------------------------------------------------------------------------\n");
    opc_uint32_t max_part=0;
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        opc_uint32_t len=xmlStrlen(c->part_array[i].name);
        if (len>max_part) max_part=len;
    }
    if (max_part>60) max_part=60;
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        opcContainerDumpString(out, c->part_array[i].name, max_part);
        fputc(' ', out);
        const xmlChar *type=opcPartGetType(c, i);
        opcContainerDumpString(out, type, 79-max_part-1);
        /*
        fprintf(out, "%s [%s]\n", c->part_array[i].name, type);
        */
        fputs("\n", out);
        for(opc_uint32_t j=0;j<c->part_array[i].relation_items;j++) {
            opcContainerRelation *rel=&c->part_array[i].relation_array[j];
            opcContainerDumpRel(c, out, rel, max_part);
        }
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






