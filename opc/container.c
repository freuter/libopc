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

static opcContainerRelPrefix* ensureRelPrefix(opcContainer *container) {
    return ((opcContainerRelPrefix*)ensureItem((void**)&container->relprefix_array, container->relprefix_items, sizeof(opcContainerRelPrefix)))+container->relprefix_items;
}

static opcContainerType* ensureType(opcContainer *container) {
    return ((opcContainerType*)ensureItem((void**)&container->type_array, container->type_items, sizeof(opcContainerType)))+container->type_items;
}

static opcContainerExtension* ensureExtension(opcContainer *container) {
    return ((opcContainerExtension*)ensureItem((void**)&container->extension_array, container->extension_items, sizeof(opcContainerExtension)))+container->extension_items;
}

static opcContainerRelationType* ensureRelationType(opcContainer *container) {
    return ((opcContainerRelationType*)ensureItem((void**)&container->relationtype_array, container->relationtype_items, sizeof(opcContainerRelationType)))+container->relationtype_items;
}

static opcContainerExternalRelation* ensureExternalRelation(opcContainer *container) {
    return ((opcContainerExternalRelation*)ensureItem((void**)&container->externalrelation_array, container->externalrelation_items, sizeof(opcContainerRelationType)))+container->externalrelation_items;
}


static opc_bool_t findItem(void *array_, opc_uint32_t items, const void *key1, opc_uint32_t key2, int (*cmp_fct)(const void *key1, opc_uint32_t key2, const void *array_, opc_uint32_t item), opc_uint32_t *pos) {
    opc_uint32_t i=0;
    opc_uint32_t j=items;
    while(i<j) {
        opc_uint32_t m=i+(j-i)/2;
        OPC_ASSERT(i<=m && m<j);
        int cmp=cmp_fct(key1, key2, array_, m);
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

#define ensureGap(array_, items_, i) \
{\
    for (opc_uint32_t k=items_;k>i;k--) { \
        (array_)[k]=(array_)[k-1];\
    }\
    (items_)++;\
    OPC_ASSERT(i>=0 && i<(items_));\
    opc_bzero_mem(&(array_)[i], sizeof((array_)[i]));\
}


static inline int part_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerPart*)array_)[item].name);
}


opcContainerPart *opcContainerInsertPart(opcContainer *container, const xmlChar *name, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->part_array, container->part_items, name, 0, part_cmp_fct, &i)) {
        return &container->part_array[i];
    } else if (insert && NULL!=ensurePart(container)) {
        ensureGap(container->part_array, container->part_items, i);
        container->part_array[i].first_segment_id=-1;
        container->part_array[i].last_segment_id=-1;
        container->part_array[i].name=xmlStrdup(name); 
        container->part_array[i].rel_segment_id=-1;
        return &container->part_array[i];
    } else {
        return NULL;
    }
}

#define deleteItem(array_, items_, i) \
{\
    for(opc_uint32_t k=i+1;k<items_;k++) {\
        array_[k-1]=array_[k];\
    }\
    items_--;\
}


static opc_error_t opcContainerDeleteAllRelationsToPart(opcContainer *container, opcPart part, opcContainerRelation **relation_array, opc_uint32_t *relation_items) {
    for(opc_uint32_t i=0;i<*relation_items;) {
        if (0==(*relation_array)[i].target_mode && part==(*relation_array)[i].target_ptr) {
            deleteItem((*relation_array), (*relation_items), i);
        } else {
            i++;
        }
    }
    return OPC_ERROR_NONE;
}

opc_error_t opcContainerDeletePart(opcContainer *container, const xmlChar *name) {
    opc_error_t ret=OPC_ERROR_NONE;
    opc_uint32_t i=0;
    if (findItem(container->part_array, container->part_items, name, 0, part_cmp_fct, &i)) {
        if (-1!=container->part_array[i].first_segment_id) {
            opcContainerDeletePartEx(container, name, OPC_FALSE);
        }
        if (-1!=container->part_array[i].rel_segment_id) {
            opcContainerDeletePartEx(container, name, OPC_TRUE);
        }
        OPC_ENSURE(OPC_ERROR_NONE==opcContainerDeleteAllRelationsToPart(container, container->part_array[i].name, &container->relation_array, &container->relation_items));
        for(opc_uint32_t j=0;j<container->part_items;j++) {
            OPC_ENSURE(OPC_ERROR_NONE==opcContainerDeleteAllRelationsToPart(container, container->part_array[i].name, &container->part_array[j].relation_array, &container->part_array[j].relation_items));
        }
        if (NULL!=container->part_array[i].relation_array){
            xmlFree(container->part_array[i].relation_array);
        }
        if (NULL!=container->part_array[i].name){
            xmlFree(container->part_array[i].name);   
        }
        deleteItem(container->part_array, container->part_items, i);
    }
    return ret;
}

#define OPC_MAX_UINT16 65535
static opc_uint32_t insertRelPrefix(opcContainer *container, const xmlChar *relPrefix) {
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

static opc_uint32_t findRelPrefix(opcContainer *container, const xmlChar *relPrefix, opc_uint32_t relPrefixLen) {
    for(opc_uint32_t i=0;i<=container->relprefix_items;i++) {
        if (0==xmlStrncmp(container->relprefix_array[i].prefix, relPrefix, relPrefixLen) && 0==container->relprefix_array[i].prefix[relPrefixLen]) {
            return i;
        }
    }
    return -1; // not found
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

static opc_uint32_t assembleRelId(opc_uint32_t prefix, opc_uint16_t relCounter) {
    opc_uint32_t ret=relCounter;
    if (-1!=prefix) {
        ret|=prefix<<16;
    } 
    return ret;
}

static opc_uint32_t createRelId(opcContainer *container, const xmlChar *relPrefix, opc_uint16_t relCounter) {
    opc_uint32_t prefix=insertRelPrefix(container, relPrefix);
    opc_uint32_t ret=assembleRelId(prefix, relCounter);
    return ret;
}


static inline int type_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerType*)array_)[item].type);
}

opcContainerType *insertType(opcContainer *container, const xmlChar *type, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->type_array, container->type_items, type, 0, type_cmp_fct, &i)) {
        return &container->type_array[i];
    } else if (insert && NULL!=ensureType(container)) {
        ensureGap(container->type_array, container->type_items, i);
        container->type_array[i].type=xmlStrdup(type);
        return &container->type_array[i];
    } else {
        return NULL;
    }
}

static inline int extension_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerExtension*)array_)[item].extension);
}

opcContainerExtension *opcContainerInsertExtension(opcContainer *container, const xmlChar *extension, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->extension_array, container->extension_items, extension, 0, extension_cmp_fct, &i)) {
        return &container->extension_array[i];
    } else if (insert && NULL!=ensureExtension(container)) {
        ensureGap(container->extension_array, container->extension_items, i);
        container->extension_array[i].extension=xmlStrdup(extension);
        return &container->extension_array[i];
    } else {
        return NULL;
    }
}

static inline int relationtype_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerRelationType*)array_)[item].type);
}
opcContainerRelationType *opcContainerInsertRelationType(opcContainer *container, const xmlChar *type, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->relationtype_array, container->relationtype_items, type, 0, relationtype_cmp_fct, &i)) {
        return &container->relationtype_array[i];
    } else if (insert && NULL!=ensureRelationType(container)) {
        ensureGap(container->relationtype_array, container->relationtype_items, i);
        container->relationtype_array[i].type=xmlStrdup(type);
        return &container->relationtype_array[i];
    } else {
        return NULL;
    }
}

static inline int externalrelation_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    return xmlStrcmp((xmlChar*)key, ((opcContainerExternalRelation*)array_)[item].target);
}

opcContainerExternalRelation*insertExternalRelation(opcContainer *container, const xmlChar *target, opc_bool_t insert) {
    opc_uint32_t i=0;
    if (findItem(container->externalrelation_array, container->externalrelation_items, target, 0, externalrelation_cmp_fct, &i)) {
        return &container->externalrelation_array[i];
    } else if (insert && NULL!=ensureExternalRelation(container)) {
        ensureGap(container->externalrelation_array, container->externalrelation_items, i);
        container->externalrelation_array[i].target=xmlStrdup(target);
        return &container->externalrelation_array[i];
    } else {
        return NULL;
    }
}


static inline int relation_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    opcRelation r1=v;
    opcRelation r2=((opcContainerRelation*)array_)[item].relation_id;
    if (OPC_CONTAINER_RELID_PREFIX(r1)==OPC_CONTAINER_RELID_PREFIX(r2)) {
        if (OPC_CONTAINER_RELID_COUNTER_NONE==OPC_CONTAINER_RELID_COUNTER(r1)) {
            return (OPC_CONTAINER_RELID_COUNTER_NONE==OPC_CONTAINER_RELID_COUNTER(r2)?0:-1);
        } else if (OPC_CONTAINER_RELID_COUNTER_NONE==OPC_CONTAINER_RELID_COUNTER(r2)) {
            OPC_ASSERT(OPC_CONTAINER_RELID_COUNTER_NONE!=OPC_CONTAINER_RELID_COUNTER(r1));
            return 1;
        } else {
            OPC_ASSERT(OPC_CONTAINER_RELID_COUNTER_NONE!=OPC_CONTAINER_RELID_COUNTER(r1) && OPC_CONTAINER_RELID_COUNTER_NONE!=OPC_CONTAINER_RELID_COUNTER(r2));
            return OPC_CONTAINER_RELID_COUNTER(r1)-OPC_CONTAINER_RELID_COUNTER(r2);
        }
    } else {
        return OPC_CONTAINER_RELID_PREFIX(r1)-OPC_CONTAINER_RELID_PREFIX(r2);
    }
}
opcContainerRelation *opcContainerInsertRelation(opcContainerRelation **relation_array, opc_uint32_t *relation_items, 
                                            opc_uint32_t relation_id,
                                            xmlChar *relation_type,
                                            opc_uint32_t target_mode, xmlChar *target_ptr) {
    OPC_ASSERT(NULL!=relation_items);
    opc_uint32_t i=0;
    if (*relation_items>0) {
        opc_bool_t ret=findItem(*relation_array, *relation_items, NULL, relation_id, relation_cmp_fct, &i);
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

opcContainerRelation *opcContainerFindRelation(opcContainer *container, opcContainerRelation *relation_array, opc_uint32_t relation_items, opcRelation relation) {
    opc_uint32_t i=0;
    opc_bool_t ret=findItem(relation_array, relation_items, NULL, relation, relation_cmp_fct, &i);
    return (ret?&relation_array[i]:NULL);
}

opc_error_t opcContainerDeleteRelation(opcContainer *container, opcContainerRelation **relation_array, opc_uint32_t *relation_items, opcRelation relation) {
    opc_error_t err=OPC_ERROR_NONE;
    opc_uint32_t i=0;
    opc_bool_t ret=findItem(*relation_array, *relation_items, NULL, relation, relation_cmp_fct, &i);
    if (ret) {
        deleteItem((*relation_array), (*relation_items), i);
    }
    return err;
}


opcContainerRelation *opcContainerFindRelationById(opcContainer *container, opcContainerRelation *relation_array, opc_uint32_t relation_items, const xmlChar *relation_id) {
    opc_uint32_t counter=-1;
    opc_uint32_t id_len=splitRelPrefix(container, relation_id, &counter);
    opc_uint32_t rel=-1;
    if (id_len>0) {
        opc_uint32_t prefix=findRelPrefix(container, relation_id, id_len);
        if (-1!=prefix) {
            rel=assembleRelId(prefix, counter);
        }
    } else {
        rel=assembleRelId(-1, counter);
    }

    opcContainerRelation *ret=(-1!=rel?opcContainerFindRelation(container, relation_array, relation_items, rel):NULL);
    return ret;
}



static void opc_container_normalize_part_to_helper_buffer(xmlChar *buf, int buf_len,
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

static void opcConstainerParseRels(opcContainer *c, const xmlChar *partName, opcContainerRelation **relation_array, opc_uint32_t *relation_items) {
    mceTextReader_t reader;
    if (OPC_ERROR_NONE==opcXmlReaderOpenEx(c, &reader, partName, OPC_TRUE, NULL, NULL, 0)) {
        static const char ns[]="http://schemas.openxmlformats.org/package/2006/relationships";
        mce_start_document(&reader) {
            mce_start_element(&reader, _X(ns), _X("Relationships")) {
                mce_skip_attributes(&reader);
                mce_start_children(&reader) {
                    mce_start_element(&reader, NULL, _X("Relationship")) {
                        const xmlChar *id=NULL;
                        const xmlChar *type=NULL;
                        const xmlChar *target=NULL;
                        const xmlChar *mode=NULL;
                        mce_start_attributes(&reader) {
                            mce_start_attribute(&reader, NULL, _X("Id")) {
                                id=xmlTextReaderConstValue(reader.reader);
                            } mce_end_attribute(&reader);
                            mce_start_attribute(&reader, NULL, _X("Type")) {
                                type=xmlTextReaderConstValue(reader.reader);
                            } mce_end_attribute(&reader);
                            mce_start_attribute(&reader, NULL, _X("Target")) {
                                target=xmlTextReaderConstValue(reader.reader);
                            } mce_end_attribute(&reader);
                            mce_start_attribute(&reader, NULL, _X("TargetMode")) {
                                mode=xmlTextReaderConstValue(reader.reader);
                            }
                        } mce_end_attributes(&reader);
                        mce_error_guard_start(&reader) {
                            mce_error(&reader, NULL==id || id[0]==0, MCE_ERROR_VALIDATION, "Missing @Id attribute!");
                            mce_error(&reader, NULL==type || type[0]==0, MCE_ERROR_VALIDATION, "Missing @Type attribute!");
                            mce_error(&reader, NULL==target || target[0]==0, MCE_ERROR_VALIDATION, "Missing @Id attribute!");
                            opcContainerRelationType *rel_type=opcContainerInsertRelationType(c, type, OPC_TRUE);
                            mce_error(&reader, NULL==rel_type, MCE_ERROR_MEMORY, NULL);
                            opc_uint32_t counter=-1;
                            opc_uint32_t id_len=splitRelPrefix(c, id, &counter);
                            ((xmlChar *)id)[id_len]=0;
                            opc_uint32_t rel_id=createRelId(c, id, counter);
                            if (NULL==mode || 0==xmlStrcasecmp(mode, _X("Internal"))) {
                                xmlChar target_part_name[OPC_MAX_PATH];
                                opc_container_normalize_part_to_helper_buffer(target_part_name, sizeof(target_part_name), partName, target);
        //                        printf("%s (%s;%s)\n", target_part_name, base, target);
                                opcContainerPart *target_part=opcContainerInsertPart(c, target_part_name, OPC_FALSE);
                                mce_errorf(&reader, NULL==target_part, MCE_ERROR_VALIDATION, "Referenced part %s (%s;%s) does not exists!", target_part_name, partName, target);
    //                            printf("%s %i %s %s\n", id, counter, rel_type->type, target_part->name);
                                opcContainerRelation *rel=opcContainerInsertRelation(relation_array, relation_items, rel_id, rel_type->type, 0, target_part->name);
                                OPC_ASSERT(NULL!=rel);
                            } else if (0==xmlStrcasecmp(mode, _X("External"))) {
                                opcContainerExternalRelation *ext_rel=insertExternalRelation(c, target, OPC_TRUE);
                                mce_error(&reader, NULL==ext_rel, MCE_ERROR_MEMORY, NULL);
                                opcContainerRelation *rel=opcContainerInsertRelation(relation_array, relation_items, rel_id, rel_type->type, 1, ext_rel->target);
                                OPC_ASSERT(NULL!=rel);
                            } else {
                                mce_errorf(&reader, OPC_TRUE, MCE_ERROR_VALIDATION, "TargetMode %s unknown!\n", mode);
                            }
                        } mce_error_guard_end(reader);
                        mce_skip_children(&reader);
                    } mce_end_element(&reader);
                } mce_end_children(&reader);
            } mce_end_element(&reader);
        } mce_end_document(reader);
        OPC_ENSURE(0==mceTextReaderCleanup(&reader));
    }
}

static const xmlChar OPC_SEGMENT_CONTENTTYPES[]={'[', 'C', 'o', 'n', 't', 'e', 'n', 't', '_', 'T', 'y', 'p', 'e', 's', ']', '.', 'x', 'm', 'l', 0};
static const xmlChar OPC_SEGMENT_ROOTRELS[]={0};

static opc_error_t opcContainerFree(opcContainer *c) {
    if (NULL!=c) {
        for(opc_uint32_t i=0;i<c->extension_items;i++) {
            xmlFree(c->extension_array[i].extension);
        }
        for(opc_uint32_t i=0;i<c->type_items;i++) {
            xmlFree(c->type_array[i].type);
        }
        for(opc_uint32_t i=0;i<c->relationtype_items;i++) {
            xmlFree(c->relationtype_array[i].type);
        }
        for(opc_uint32_t i=0;i<c->externalrelation_items;i++) {
            xmlFree(c->externalrelation_array[i].target);
        }
        for(opc_uint32_t i=0;i<c->part_items;i++) {
            xmlFree(c->part_array[i].relation_array);
            xmlFree(c->part_array[i].name);
        }
        for(opc_uint32_t i=0;i<c->relprefix_items;i++) {
            xmlFree(c->relprefix_array[i].prefix);
        }
        if (NULL!=c->part_array) xmlFree(c->part_array);
        if (NULL!=c->relprefix_array) xmlFree(c->relprefix_array);
        if (NULL!=c->type_array) xmlFree(c->type_array);
        if (NULL!=c->extension_array) xmlFree(c->extension_array);
        if (NULL!=c->relationtype_array) xmlFree(c->relationtype_array);
        if (NULL!=c->externalrelation_array) xmlFree(c->externalrelation_array);
        if (NULL!=c->relation_array) xmlFree(c->relation_array);
        opcZipClose(c->storage, NULL);
        xmlFree(c);
    }
    return OPC_ERROR_NONE;
}

static void opcContainerDumpString(FILE *out, const xmlChar *str, opc_uint32_t max_len, opc_bool_t new_line) {
    opc_uint32_t len=(NULL!=str?xmlStrlen(str):0);
    if (len<=max_len) {
        if (NULL!=str) fputs((const char *)str, out);
        for(opc_uint32_t i=len;i<max_len;i++) fputc(' ', out);
    } else {
        static const char prefix[]="...";
        static opc_uint32_t prefix_len=sizeof(prefix)-1;
        opc_uint32_t ofs=len-max_len;
        if (ofs+prefix_len<len) ofs+=prefix_len; else ofs=len;
        fputs(prefix, out);
        fputs((const char *)(str+ofs), out);
    }
    if (new_line) {
        fputc('\n', out);
    }
}

static void opcContainerDumpLine(FILE *out, const xmlChar line_char, opc_uint32_t max_len, opc_bool_t new_line) {
    for(opc_uint32_t i=0;i<max_len;i++) {
        fputc(line_char, out);
    }
    if (new_line) {
        fputc('\n', out);
    }
}


static void opcContainerRelCalcMax(opcContainer *c, 
                                   const xmlChar *part_name,
                                   opcContainerRelation *relation_array, opc_uint32_t relation_items, 
                                   opc_uint32_t *max_rel_src,
                                   opc_uint32_t *max_rel_id,
                                   opc_uint32_t *max_rel_dest,
                                   opc_uint32_t *max_rel_type) {
    if (relation_items>0) {
        opc_uint32_t const src_len=xmlStrlen(part_name); 
        if (src_len>*max_rel_src) *max_rel_src=src_len;
        for(opc_uint32_t j=0;j<relation_items;j++) {
            const xmlChar *prefix=NULL;
            opc_uint32_t counter=-1;
            const xmlChar *type=NULL;
            char buf[20]="";
            opcRelationGetInformation(c, (opcPart)part_name, relation_array[j].relation_id, &prefix, &counter, &type);
            if (-1!=counter) {
                sprintf(buf, "%i", counter);
            }
            opc_uint32_t const type_len=xmlStrlen(type); 
            if (type_len>*max_rel_type) *max_rel_type=type_len;
            opc_uint32_t const id_len=xmlStrlen(prefix)+xmlStrlen(_X(buf)); 
            if (id_len>*max_rel_id) *max_rel_id=id_len;
            opc_uint32_t const dest_len=xmlStrlen(relation_array[j].target_ptr); 
            if (dest_len>*max_rel_dest) *max_rel_dest=dest_len;
        }
    }
}

static void opcContainerRelDump(opcContainer *c, 
                                FILE *out,
                                const xmlChar *part_name,
                                opcContainerRelation *relation_array, opc_uint32_t relation_items, 
                                opc_uint32_t max_rel_src,
                                opc_uint32_t max_rel_id,
                                opc_uint32_t max_rel_dest,
                                opc_uint32_t max_rel_type) {
    for(opc_uint32_t j=0;j<relation_items;j++) {
        opcContainerDumpString(out, (part_name==NULL?_X("[root]"):part_name), max_rel_src, OPC_FALSE); fputc('|', out);
        const xmlChar *prefix=NULL;
        opc_uint32_t counter=-1;
        const xmlChar *type=NULL;
        char buf[20]="";
        opcRelationGetInformation(c, (opcPart)part_name, relation_array[j].relation_id, &prefix, &counter, &type);
        if (-1!=counter) {
            sprintf(buf, "%i", counter);
        }
        opc_uint32_t prefix_len=xmlStrlen(prefix);
        opcContainerDumpString(out, prefix, prefix_len, OPC_FALSE);
        OPC_ASSERT(xmlStrlen(_X(buf))+prefix_len<=max_rel_id);
        opcContainerDumpString(out, _X(buf), max_rel_id-prefix_len, OPC_FALSE); fputc('|', out);
        opcContainerDumpString(out, relation_array[j].target_ptr, max_rel_dest, OPC_FALSE); fputc('|', out);
        opcContainerDumpString(out, type, max_rel_type, OPC_TRUE);
    }
}

opc_error_t opcContainerDump(opcContainer *c, FILE *out) {
    opc_uint32_t max_content_type_len=xmlStrlen(_X("Content Types")); 
    for(opc_uint32_t i=0;i<c->type_items;i++) { 
        opc_uint32_t const len=xmlStrlen(c->type_array[i].type); 
        if (len>max_content_type_len) max_content_type_len=len;
    }
    opc_uint32_t max_extension_len=xmlStrlen(_X("Extension")); 
    opc_uint32_t max_extension_type_len=xmlStrlen(_X("Type")); 
    for(opc_uint32_t i=0;i<c->extension_items;i++) { 
        opc_uint32_t const len=xmlStrlen(c->extension_array[i].extension); 
        if (len>max_extension_len) max_extension_len=len;
        opc_uint32_t const type_len=xmlStrlen(c->extension_array[i].type); 
        if (type_len>max_extension_type_len) max_extension_type_len=type_len;
    }
    opc_uint32_t max_rel_type_len=xmlStrlen(_X("Relation Types")); 
    for(opc_uint32_t i=0;i<c->relationtype_items;i++) { 
        opc_uint32_t const len=xmlStrlen(c->relationtype_array[i].type); 
        if (len>max_rel_type_len) max_rel_type_len=len;
    }

    opc_uint32_t max_ext_rel_len=xmlStrlen(_X("External Relations")); 
    for(opc_uint32_t i=0;i<c->externalrelation_items;i++) { 
        opc_uint32_t const len=xmlStrlen(c->externalrelation_array[i].target); 
        if (len>max_ext_rel_len) max_ext_rel_len=len;
    }
    opc_uint32_t max_part_name=xmlStrlen(_X("Part")); 
    opc_uint32_t max_part_type=xmlStrlen(_X("Type")); 
    for(opc_uint32_t i=0;i<c->part_items;i++) { 
        if (-1!=c->part_array[i].first_segment_id) { // deleted?
            opc_uint32_t const name_len=xmlStrlen(c->part_array[i].name); 
            if (name_len>max_part_name) max_part_name=name_len;
            opc_uint32_t const type_len=xmlStrlen(opcPartGetType(c, c->part_array[i].name)); 
            if (type_len>max_part_type) max_part_type=type_len;
        }
    }

    opc_uint32_t max_rel_src=xmlStrlen(_X("Source")); 
    opc_uint32_t max_rel_id=xmlStrlen(_X("Id")); 
    opc_uint32_t max_rel_dest=xmlStrlen(_X("Destination")); 
    opc_uint32_t max_rel_type=xmlStrlen(_X("Type")); 
    if (c->relationtype_items>0) {
        opc_uint32_t const src_len=xmlStrlen(_X("[root]")); 
        if (src_len>max_rel_src) max_rel_src=src_len;
        opcContainerRelCalcMax(c, NULL, c->relation_array, c->relation_items, &max_rel_src, &max_rel_id, &max_rel_dest, &max_rel_type);
    }
    for(opc_uint32_t i=0;i<c->part_items;i++) { 
        if (c->part_array[i].relation_items>0) {
            opcContainerRelCalcMax(c, 
                                   c->part_array[i].name, 
                                   c->part_array[i].relation_array, c->part_array[i].relation_items, 
                                   &max_rel_src, 
                                   &max_rel_id, 
                                   &max_rel_dest, 
                                   &max_rel_type);
        }
    }

    opcContainerDumpString(out, _X("Content Types"), max_content_type_len, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_content_type_len, OPC_TRUE);
    for(opc_uint32_t i=0;i<c->type_items;i++) {
        opcContainerDumpString(out, c->type_array[i].type, max_content_type_len, OPC_TRUE);
    }
    opcContainerDumpLine(out, '-', max_content_type_len, OPC_TRUE);
    opcContainerDumpString(out, _X(""), 0, OPC_TRUE);

    opcContainerDumpString(out, _X("Extension"), max_extension_len, OPC_FALSE); fputc('|', out);
    opcContainerDumpString(out, _X("Type"), max_extension_type_len, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_extension_len, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_extension_type_len, OPC_TRUE);
    for(opc_uint32_t i=0;i<c->extension_items;i++) {
        opcContainerDumpString(out, c->extension_array[i].extension, max_extension_len, OPC_FALSE); fputc('|', out);
        opcContainerDumpString(out, c->extension_array[i].type, max_extension_type_len, OPC_TRUE);
    }
    opcContainerDumpLine(out, '-', max_extension_len, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_extension_type_len, OPC_TRUE);
    opcContainerDumpString(out, _X(""), 0, OPC_TRUE);

    opcContainerDumpString(out, _X("Relation Types"), max_rel_type_len, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_rel_type_len, OPC_TRUE);
    for(opc_uint32_t i=0;i<c->relationtype_items;i++) {
        opcContainerDumpString(out, c->relationtype_array[i].type, max_rel_type_len, OPC_TRUE);
    }
    opcContainerDumpLine(out, '-', max_rel_type_len, OPC_TRUE);
    opcContainerDumpString(out, _X(""), 0, OPC_TRUE);

    opcContainerDumpString(out, _X("External Relations"), max_ext_rel_len, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_ext_rel_len, OPC_TRUE);
    for(opc_uint32_t i=0;i<c->externalrelation_items;i++) {
        opcContainerDumpString(out, c->externalrelation_array[i].target, max_ext_rel_len, OPC_TRUE);
    }
    opcContainerDumpLine(out, '-', max_ext_rel_len, OPC_TRUE);
    opcContainerDumpString(out, _X(""), 0, OPC_TRUE);

    opcContainerDumpString(out, _X("Part"), max_part_name, OPC_FALSE); fputc('|', out);
    opcContainerDumpString(out, _X("Type"), max_part_type, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_part_name, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_part_type, OPC_TRUE);
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        if (-1!=c->part_array[i].first_segment_id) { // deleted?
            opcContainerDumpString(out, c->part_array[i].name, max_part_name, OPC_FALSE); fputc('|', out);
            opcContainerDumpString(out, opcPartGetType(c, c->part_array[i].name), max_part_type, OPC_TRUE);
        }
    }
    opcContainerDumpLine(out, '-', max_part_name, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_part_type, OPC_TRUE);
    opcContainerDumpString(out, _X(""), 0, OPC_TRUE);

    opcContainerDumpString(out, _X("Source"), max_rel_src, OPC_FALSE); fputc('|', out);
    opcContainerDumpString(out, _X("Id"), max_rel_id, OPC_FALSE); fputc('|', out);
    opcContainerDumpString(out, _X("Destination"), max_rel_dest, OPC_FALSE); fputc('|', out);
    opcContainerDumpString(out, _X("Type"), max_rel_type, OPC_TRUE);
    opcContainerDumpLine(out, '-', max_rel_src, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_id, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_dest, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_type, OPC_TRUE);
    if (c->relation_items>0) {
        opcContainerRelDump(c, 
                            out, 
                            NULL, 
                            c->relation_array, 
                            c->relation_items, 
                            max_rel_src, 
                            max_rel_id, 
                            max_rel_dest, 
                            max_rel_type);
    }
    for(opc_uint32_t i=0;i<c->part_items;i++) { 
        if (-1!=c->part_array[i].first_segment_id && c->part_array[i].relation_items>0) {
            opcContainerRelDump(c, 
                                out,
                                c->part_array[i].name, 
                                c->part_array[i].relation_array, c->part_array[i].relation_items, 
                                max_rel_src, 
                                max_rel_id, 
                                max_rel_dest, 
                                max_rel_type);
        }
    }
    opcContainerDumpLine(out, '-', max_rel_src, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_id, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_dest, OPC_FALSE); fputc('|', out);
    opcContainerDumpLine(out, '-', max_rel_type, OPC_TRUE);

    return OPC_ERROR_NONE;
}

static opc_error_t opcContainerZipLoaderLoadSegment(void *iocontext, 
                              void *userctx, 
                              opcZipSegmentInfo_t *info, 
                              opcZipLoaderOpenCallback *open, 
                              opcZipLoaderReadCallback *read, 
                              opcZipLoaderCloseCallback *close, 
                              opcZipLoaderSkipCallback *skip) {
    opc_error_t err=OPC_ERROR_NONE;
    opcContainer *c=(opcContainer *)userctx;
    OPC_ENSURE(0==skip(iocontext));
    if (info->rels_segment) {
        if (info->name[0]==0) {
            OPC_ASSERT(-1==c->rels_segment_id); // loaded twice??
            c->rels_segment_id=opcZipLoadSegment(c->storage, OPC_SEGMENT_ROOTRELS, info->rels_segment, info);
            OPC_ASSERT(-1!=c->rels_segment_id); // not loaded??
        } else {
            opcContainerPart *part=opcContainerInsertPart(c, info->name, OPC_TRUE);
            if (NULL!=part) {
                OPC_ASSERT(-1==part->rel_segment_id); // loaded twice??
                OPC_ASSERT(NULL!=part->name); // no name given???
                part->rel_segment_id=opcZipLoadSegment(c->storage, part->name, info->rels_segment, info);
                OPC_ASSERT(-1!=part->rel_segment_id);  // not loaded???
            } else {
                err=OPC_ERROR_MEMORY;
            }
        }
    } else if (xmlStrcmp(info->name, OPC_SEGMENT_CONTENTTYPES)==0) {
        OPC_ASSERT(-1==c->content_types_segment_id); // loaded twice??
        c->content_types_segment_id=opcZipLoadSegment(c->storage, OPC_SEGMENT_CONTENTTYPES, info->rels_segment, info);
        OPC_ASSERT(-1!=c->content_types_segment_id); // not loaded??
    } else {
        opcContainerPart *part=opcContainerInsertPart(c, info->name, OPC_TRUE);
        if (NULL!=part) {
            OPC_ASSERT(-1==part->first_segment_id); // loaded twice???
            OPC_ASSERT(NULL!=part->name); // no name given???
            part->first_segment_id=opcZipLoadSegment(c->storage, part->name, info->rels_segment, info);
            OPC_ASSERT(-1!=part->first_segment_id);  // not loaded???
            part->last_segment_id=part->first_segment_id;

            if (info->name_len>0 && ('/'==info->name[info->name_len-1] || '\\'==info->name[info->name_len-1])) {
                // it is a directoy, get rid of it...
                opcContainerDeletePartEx(c, part->name, info->rels_segment);
                OPC_ASSERT(-1==part->first_segment_id && -1==part->last_segment_id);
                part=NULL; // no longer valid
            }
        } else {
            err=OPC_ERROR_MEMORY;
        }
    }
    return err;
}

static void opcContainerGetOutputPartSegment(opcContainer *container, const xmlChar *name, opc_bool_t rels_segment, opc_uint32_t **first_segment_ref, opc_uint32_t **last_segment_ref) {
    if (OPC_SEGMENT_CONTENTTYPES==name) {
        OPC_ASSERT(!rels_segment);
        *first_segment_ref=&container->content_types_segment_id;
        *last_segment_ref=NULL;
    } else if (OPC_SEGMENT_ROOTRELS==name) {
        OPC_ASSERT(rels_segment);
        *first_segment_ref=&container->rels_segment_id;
        *last_segment_ref=NULL;
    } else {
        opcContainerPart *part=opcContainerInsertPart(container, name, OPC_FALSE);
        if (NULL!=part) {
            if (rels_segment) {
                *first_segment_ref=&part->rel_segment_id;
                *last_segment_ref=NULL;
            } else {
                *first_segment_ref=&part->first_segment_id;
                *last_segment_ref=&part->last_segment_id;
            }
        } else {
            OPC_ASSERT(OPC_FALSE); // should not happen
            *first_segment_ref=NULL;
            *last_segment_ref=NULL;
        }
    }
}


opcContainerInputStream* opcContainerOpenInputStreamEx(opcContainer *container, const xmlChar *name, opc_bool_t rels_segment) {
    opcContainerInputStream* ret=NULL;
    opc_uint32_t *first_segment=NULL;
    opc_uint32_t *last_segment=NULL;
    opcContainerGetOutputPartSegment(container, name, rels_segment, &first_segment, &last_segment);
    OPC_ASSERT(NULL!=first_segment);
    if (NULL!=first_segment) {
        ret=(opcContainerInputStream*)xmlMalloc(sizeof(opcContainerInputStream));
        if (NULL!=ret) {
            opc_bzero_mem(ret, sizeof(*ret));
            ret->container=container;
            ret->stream=opcZipOpenInputStream(container->storage, *first_segment);
            if (NULL==ret->stream) {
                xmlFree(ret); ret=NULL; // error
            }
        }
    }
    return ret;
}

opcContainerInputStream* opcContainerOpenInputStream(opcContainer *container, const xmlChar *name) {
    return opcContainerOpenInputStreamEx(container, name, OPC_FALSE);
}

opc_uint32_t opcContainerReadInputStream(opcContainerInputStream* stream, opc_uint8_t *buffer, opc_uint32_t buffer_len) {
    return opcZipReadInputStream(stream->container->storage, stream->stream, buffer, buffer_len);
}

opc_error_t opcContainerCloseInputStream(opcContainerInputStream* stream) {
    opc_error_t ret=opcZipCloseInputStream(stream->container->storage, stream->stream);
    xmlFree(stream);
    return ret;
}

opcCompressionOption_t opcContainerGetInputStreamCompressionOption(opcContainerInputStream* stream) {
    opcCompressionOption_t ret=OPC_COMPRESSIONOPTION_NONE;
    if (8==stream->stream->inflateState.compression_method) {
        // for now its just enough to know that we have a compression...
        ret=OPC_COMPRESSIONOPTION_NORMAL; //@TODO look at stream to figure out real compression i.e. NORMAL, FAST, etc...
    }
    return ret;
}


opcContainerOutputStream* opcContainerCreateOutputStreamEx(opcContainer *container, const xmlChar *name, opc_bool_t rels_segment, opcCompressionOption_t compression_option) {
    opcContainerOutputStream* ret=NULL;
    opc_uint32_t *first_segment=NULL;
    opc_uint32_t *last_segment=NULL;
    opcContainerGetOutputPartSegment(container, name, rels_segment, &first_segment, &last_segment);
    OPC_ASSERT(NULL!=first_segment);
    if (NULL!=first_segment) {
        ret=(opcContainerOutputStream*)xmlMalloc(sizeof(opcContainerOutputStream));
        if (NULL!=ret) {
            opc_bzero_mem(ret, sizeof(*ret));
            ret->container=container;
            opc_uint16_t compression_method=0; // no compression by default
            opc_uint16_t bit_flag=0;
            switch(compression_option) {
            case OPC_COMPRESSIONOPTION_NONE:
                OPC_ASSERT(0==compression_method);
                OPC_ASSERT(0==bit_flag);
                break;
            case OPC_COMPRESSIONOPTION_NORMAL:
                compression_method=8;
                bit_flag|=0<<1;
                break;
            case OPC_COMPRESSIONOPTION_MAXIMUM:
                compression_method=8;
                bit_flag|=1<<1;
                break;
            case OPC_COMPRESSIONOPTION_FAST:
                compression_method=8;
                bit_flag|=2<<1;
                break;
            case OPC_COMPRESSIONOPTION_SUPERFAST:
                compression_method=8;
                bit_flag|=3<<1;
                break;
            }
            ret->stream=opcZipCreateOutputStream(container->storage, first_segment, name, rels_segment, 0, 0, compression_method, bit_flag);
            ret->partName=name;
            ret->rels_segment=rels_segment;
            if (NULL==ret->stream) {
                xmlFree(ret); ret=NULL; // error
            }
        }
    }
    return ret;
}

opcContainerOutputStream* opcContainerCreateOutputStream(opcContainer *container, const xmlChar *name, opcCompressionOption_t compression_option) {
    return opcContainerCreateOutputStreamEx(container, name, OPC_FALSE, compression_option);
}

opc_uint32_t opcContainerWriteOutputStream(opcContainerOutputStream* stream, const opc_uint8_t *buffer, opc_uint32_t buffer_len) {
    return opcZipWriteOutputStream(stream->container->storage, stream->stream, buffer, buffer_len);
}

opc_error_t opcContainerCloseOutputStream(opcContainerOutputStream* stream) {
    opc_error_t ret=OPC_ERROR_MEMORY;
    opc_uint32_t *first_segment=NULL;
    opc_uint32_t *last_segment=NULL;
    opcContainerGetOutputPartSegment(stream->container, stream->partName, stream->rels_segment, &first_segment, &last_segment);
    OPC_ASSERT(NULL!=first_segment);
    if (NULL!=first_segment) {
        ret=opcZipCloseOutputStream(stream->container->storage, stream->stream, first_segment);
        if (NULL!=last_segment) {
            *last_segment=*first_segment; 
        }
        xmlFree(stream);
    }
    return ret;
}

static opc_error_t opcContainerInit(opcContainer *c, opcContainerOpenMode mode, void *userContext) {
    opc_bzero_mem(c, sizeof(*c));
    c->content_types_segment_id=-1;
    c->rels_segment_id=-1;
    c->mode=mode;
    c->userContext=userContext;
    return OPC_ERROR_NONE;
}

static opcContainer *opcContainerLoadFromZip(opcContainer *c) {
    OPC_ASSERT(NULL==c->storage); // loaded twice??
    c->storage=opcZipCreate(&c->io);
    if (NULL!=c->storage) {
        if (OPC_ERROR_NONE==opcZipLoader(&c->io, c, opcContainerZipLoaderLoadSegment)) {
            // successfull loaded!
            OPC_ENSURE(OPC_ERROR_NONE==opcZipGC(c->storage));
            if (-1!=c->content_types_segment_id) {
                mceTextReader_t reader;
                if (OPC_ERROR_NONE==opcXmlReaderOpenEx(c, &reader, OPC_SEGMENT_CONTENTTYPES, OPC_FALSE, NULL, NULL, 0)) {
                    static const char ns[]="http://schemas.openxmlformats.org/package/2006/content-types";
                    mce_start_document(&reader) {
                        mce_start_element(&reader, _X(ns), _X("Types")) {
                            mce_skip_attributes(&reader);
                            mce_start_children(&reader) {
                                mce_start_element(&reader, NULL, _X("Default")) {
                                    const xmlChar *ext=NULL;
                                    const xmlChar *type=NULL;
                                    mce_start_attributes(&reader) {
                                        mce_start_attribute(&reader, NULL, _X("Extension")) {
                                            ext=xmlTextReaderConstValue(reader.reader);
                                        } mce_end_attribute(&reader);
                                        mce_start_attribute(&reader, NULL, _X("ContentType")) {
                                            type=xmlTextReaderConstValue(reader.reader);
                                        } mce_end_attribute(&reader);
                                    } mce_end_attributes(&reader);
                                    mce_error_guard_start(&reader) {
                                        mce_error(&reader, NULL==ext || ext[0]==0, MCE_ERROR_VALIDATION, "Missing @Extension attribute!");
                                        mce_error(&reader, NULL==type || type[0]==0, MCE_ERROR_VALIDATION, "Missing @ContentType attribute!");
                                        opcContainerType *ct=insertType(c, type, OPC_TRUE);
                                        mce_error(&reader, NULL==ct, MCE_ERROR_MEMORY, NULL);
                                        opcContainerExtension *ce=opcContainerInsertExtension(c, ext, OPC_TRUE);
                                        mce_error(&reader, NULL==ce, MCE_ERROR_MEMORY, NULL);
                                        mce_errorf(&reader, NULL!=ce->type && 0!=xmlStrcmp(ce->type, type), MCE_ERROR_VALIDATION, "Extension \"%s\" is mapped to type \"%s\" as well as \"%s\"", ext, type, ce->type);
                                        ce->type=ct->type;
                                    } mce_error_guard_end(&reader);
                                    mce_skip_children(&reader);
                                } mce_end_element(&reader);
                                mce_start_element(&reader, NULL, _X("Override")) {
                                    const xmlChar *name=NULL;
                                    const xmlChar *type=NULL;
                                    mce_start_attributes(&reader) {
                                        mce_start_attribute(&reader, NULL, _X("PartName")) {
                                            name=xmlTextReaderConstValue(reader.reader);
                                        } mce_end_attribute(&reader);
                                        mce_start_attribute(&reader, NULL, _X("ContentType")) {
                                            type=xmlTextReaderConstValue(reader.reader);
                                        } mce_end_attribute(&reader);
                                    } mce_end_attributes(&reader);
                                    mce_error_guard_start(&reader) {
                                        mce_error(&reader, NULL==name, MCE_ERROR_XML, "Attribute @PartName not given!");
                                        mce_error(&reader, NULL==type, MCE_ERROR_XML, "Attribute @ContentType not given!");
                                        opcContainerType*ct=insertType(c, type, OPC_TRUE);
                                        mce_error(&reader, NULL==ct, MCE_ERROR_MEMORY, NULL);
                                        mce_error_strictf(&reader, '/'!=name[0], MCE_ERROR_MEMORY, "Part %s MUST start with a '/'", name);
                                        opcContainerPart *part=opcContainerInsertPart(c, (name[0]=='/'?name+1:name), OPC_FALSE);
                                        mce_error_strictf(&reader, NULL==part, MCE_ERROR_MEMORY, "Part %s does not exist.", name);
                                        if (NULL!=part) {
                                            part->type=ct->type;
                                        }
                                    } mce_error_guard_end(&reader);
                                    mce_skip_children(&reader);
                                } mce_end_element(&reader);
                                mce_start_text(&reader) {
                                    //@TODO ensure whitespaces...
                                } mce_end_text(&reader);
                            } mce_end_children(&reader);
                        } mce_end_element(&reader);
                    } mce_end_document(&reader);
                    OPC_ENSURE(0==mceTextReaderCleanup(&reader));
                }
            }
            if (NULL!=c && -1!=c->rels_segment_id) {
                opcConstainerParseRels(c, OPC_SEGMENT_ROOTRELS, &c->relation_array, &c->relation_items);
            }
            for(opc_uint32_t i=0;NULL!=c && i<c->part_items;i++) {
                opcContainerPart *part=&c->part_array[i];
                if (-1!=part->rel_segment_id) {
                    opcConstainerParseRels(c, part->name, &part->relation_array, &part->relation_items);
                }
            }
        } else {
            opcFileCleanupIO(&c->io); // error loading
            opcZipClose(c->storage, NULL);
            xmlFree(c); c=NULL;
        }
    } else {
        opcFileCleanupIO(&c->io); // error creating zip
        xmlFree(c); c=NULL;
    }
    return c;
}

static opc_uint32_t opcContainerGenerateFileFlags(opcContainerOpenMode mode) {
    opc_uint32_t flags=(OPC_OPEN_READ_ONLY!=mode?OPC_FILE_WRITE | OPC_FILE_READ:OPC_FILE_READ);
    if (OPC_OPEN_WRITE_ONLY==mode) flags=flags | OPC_FILE_TRUNC;
    return flags;
}

opcContainer* opcContainerOpen(const xmlChar *fileName, 
                               opcContainerOpenMode mode, 
                               void *userContext, 
                               const xmlChar *destName) {
    opcContainer*c=(opcContainer*)xmlMalloc(sizeof(opcContainer));
    if (NULL!=c) {
        OPC_ENSURE(OPC_ERROR_NONE==opcContainerInit(c, mode, userContext));
        if (OPC_ERROR_NONE==opcFileInitIOFile(&c->io, fileName, opcContainerGenerateFileFlags(mode))) {
            c=opcContainerLoadFromZip(c);
        } else {
            xmlFree(c); c=NULL; // error init io
        }
    }
    return c;
}

opcContainer* opcContainerOpenMem(const opc_uint8_t *data, opc_uint32_t data_len,
                                  opcContainerOpenMode mode, 
                                  void *userContext) {
    opcContainer*c=(opcContainer*)xmlMalloc(sizeof(opcContainer));
    if (NULL!=c) {
        OPC_ENSURE(OPC_ERROR_NONE==opcContainerInit(c, mode, userContext));
        if (OPC_ERROR_NONE==opcFileInitIOMemory(&c->io, data, data_len, opcContainerGenerateFileFlags(mode))) {
            c=opcContainerLoadFromZip(c);
        } else {
            xmlFree(c); c=NULL; // error init io
        }
    }
    return c;
}

opcContainer* opcContainerOpenIO(opcFileReadCallback *ioread,
                                 opcFileWriteCallback *iowrite,
                                 opcFileCloseCallback *ioclose,
                                 opcFileSeekCallback *ioseek,
                                 opcFileTrimCallback *iotrim,
                                 opcFileFlushCallback *ioflush,
                                 void *iocontext,
                                 pofs_t file_size,
                                 opcContainerOpenMode mode, 
                                 void *userContext) {
    opcContainer*c=(opcContainer*)xmlMalloc(sizeof(opcContainer));
    if (NULL!=c) {
        OPC_ENSURE(OPC_ERROR_NONE==opcContainerInit(c, mode, userContext));
        if (OPC_ERROR_NONE==opcFileInitIO(&c->io, ioread, iowrite, ioclose, ioseek, iotrim, ioflush, iocontext, file_size, opcContainerGenerateFileFlags(mode))) {
            c=opcContainerLoadFromZip(c);
        } else {
            xmlFree(c); c=NULL; // error init io
        }
    }
    return c;
}


static void opcContainerWriteUtf8Raw(opcContainerOutputStream *out, const xmlChar *str) {
    opc_uint32_t str_len=xmlStrlen(str);
    OPC_ENSURE(str_len==opcContainerWriteOutputStream(out, str, str_len));
}

static void opcContainerWriteUtf8(opcContainerOutputStream *out, const xmlChar *str) {
    for(;0!=*str; str++) {
        switch(*str) {
        case '"':
            OPC_ENSURE(6==opcContainerWriteOutputStream(out, _X("&quot;"), 6));
            break;
        case '\'':
            OPC_ENSURE(6==opcContainerWriteOutputStream(out, _X("&apos;"), 6));
            break;
        case '&':
            OPC_ENSURE(5==opcContainerWriteOutputStream(out, _X("&amp;"), 5));
            break;
        case '<':
            OPC_ENSURE(4==opcContainerWriteOutputStream(out, _X("&lt;"), 4));
            break;
        case '>':
            OPC_ENSURE(4==opcContainerWriteOutputStream(out, _X("&gt;"), 4));
            break;
        default:
            OPC_ENSURE(1==opcContainerWriteOutputStream(out, str, 1));
            break;
        }
    }
}


static void opcContainerWriteContentTypes(opcContainer *c) {
    opcContainerOutputStream *out=opcContainerCreateOutputStreamEx(c, OPC_SEGMENT_CONTENTTYPES, OPC_FALSE, OPC_COMPRESSIONOPTION_NORMAL);
    if (NULL!=out) {
        opcContainerWriteUtf8Raw(out, _X("<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"));
        for(opc_uint32_t i=0;i<c->extension_items;i++) {
            opcContainerWriteUtf8Raw(out, _X("<Default Extension=\""));
            opcContainerWriteUtf8(out, c->extension_array[i].extension);
            opcContainerWriteUtf8Raw(out, _X("\" ContentType=\""));
            opcContainerWriteUtf8(out, c->extension_array[i].type);
            opcContainerWriteUtf8Raw(out, _X("\"/>"));
        }
        for(opc_uint32_t i=0;i<c->part_items;i++) {
            if (NULL!=c->part_array[i].type) {
                opcContainerWriteUtf8Raw(out, _X("<Override PartName=\"/"));
                opcContainerWriteUtf8(out, c->part_array[i].name);
                opcContainerWriteUtf8Raw(out, _X("\" ContentType=\""));
                opcContainerWriteUtf8(out, c->part_array[i].type);
                opcContainerWriteUtf8Raw(out, _X("\"/>"));
            }
        }
        opcContainerWriteUtf8Raw(out, _X("</Types>"));
        opcContainerCloseOutputStream(out);
    }
}

static void opcHelperCalcRelPath(xmlChar *rel_path, opc_uint32_t rel_path_max, const xmlChar *base, const xmlChar *path) {
    opc_uint32_t base_pos=0;
    opc_uint32_t path_pos=0;
    opc_uint32_t rel_pos=0;

    while(0!=base[base_pos]) {
        opc_uint32_t base_next=base_pos; while(0!=base[base_next] && '/'!=base[base_next]) base_next++;
        opc_uint32_t path_next=path_pos;  while(0!=base[path_next] && '/'!=base[path_next]) path_next++;
        if ('/'==base[base_next]) {
            if (base_next==path_next && 0==xmlStrncmp(base+base_pos, path+path_pos, base_next-base_pos)) {
                base_pos=base_next+1;
                path_pos=path_next+1;
            } else {
                base_pos=base_next+1;
                strncpy((char *)(rel_path+rel_pos), "../", rel_path_max-rel_pos);
                rel_pos+=3;
            }
        } else {
            OPC_ASSERT(0==base[base_next]);
            base_pos=base_next;
            OPC_ASSERT(0==base[base_pos]);
        }
    }
    strncpy((char *)(rel_path+rel_pos), (const char *)(path+path_pos), rel_path_max-rel_pos);
#if 0 // for debugging only...
    xmlChar helper[OPC_MAX_PATH];
    opc_container_normalize_part_to_helper_buffer(helper, sizeof(helper), base, rel_path);
    OPC_ASSERT(0==xmlStrcmp(path, helper));
#endif
}

static void opcContainerWriteRels(opcContainer *c, const xmlChar *part_name, opcContainerRelation *relation_array, opc_uint32_t relation_items) {
    opcContainerOutputStream *out=opcContainerCreateOutputStreamEx(c, part_name, OPC_TRUE, OPC_COMPRESSIONOPTION_NORMAL);
    if (NULL!=out) {
        opcContainerWriteUtf8Raw(out, _X("<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"));
        for(opc_uint32_t i=0;i<relation_items;i++) {
            opcContainerWriteUtf8Raw(out, _X("<Relationship Id=\""));
            opcContainerWriteUtf8(out, c->relprefix_array[OPC_CONTAINER_RELID_PREFIX(relation_array[i].relation_id)].prefix);
            if (OPC_CONTAINER_RELID_COUNTER_NONE!=OPC_CONTAINER_RELID_COUNTER(relation_array[i].relation_id)) {
                char buf[20];
                snprintf(buf, sizeof(buf), "%i", OPC_CONTAINER_RELID_COUNTER(relation_array[i].relation_id));
                opcContainerWriteUtf8Raw(out, _X(buf));
            }
            opcContainerWriteUtf8Raw(out, _X("\" Type=\""));
            opcContainerWriteUtf8(out, relation_array[i].relation_type);
            if (0==relation_array[i].target_mode) {
                opcContainerWriteUtf8Raw(out, _X("\" Target=\""));
                xmlChar rel_path[OPC_MAX_PATH];
                opcHelperCalcRelPath(rel_path, sizeof(rel_path), part_name, relation_array[i].target_ptr);
                opcContainerWriteUtf8(out, rel_path);
            } else {
                OPC_ASSERT(1==relation_array[i].target_mode);
                opcContainerWriteUtf8Raw(out, _X("\" TargetMode=\"External\" Target=\""));
                opcContainerWriteUtf8(out, relation_array[i].target_ptr);
            }
            opcContainerWriteUtf8Raw(out, _X("\"/>"));
        }
        opcContainerWriteUtf8Raw(out, _X("</Relationships>"));
        opcContainerCloseOutputStream(out);
    }
}


static void opcContainerWriteAllRels(opcContainer *c) {
    if (c->relation_items>0) {
        opcContainerWriteRels(c, OPC_SEGMENT_ROOTRELS, c->relation_array, c->relation_items);
    }
    for(opc_uint32_t i=0;i<c->part_items;i++) {
        if (c->part_array[i].relation_items>0) {
            opcContainerWriteRels(c, c->part_array[i].name, c->part_array[i].relation_array, c->part_array[i].relation_items);
        }
    }
}

opc_error_t opcContainerCommit(opcContainer *c, opc_bool_t trim) {
    opc_error_t ret=OPC_ERROR_NONE;
    if (OPC_OPEN_READ_ONLY!=c->mode) {
        opcContainerWriteContentTypes(c);
        opcContainerWriteAllRels(c);
        ret=opcZipCommit(c->storage, trim);
    }
    return ret;
}

opc_error_t opcContainerClose(opcContainer *c, opcContainerCloseMode mode) {
    opc_bool_t trim=(mode!=OPC_CLOSE_NOW);
    opc_error_t ret=opcContainerCommit(c, trim);
    opcZipClose(c->storage, NULL); c->storage=NULL;
    opcContainerFree(c);
    return ret;
}

opc_bool_t opcContainerDeletePartEx(opcContainer *container, const xmlChar *partName, opc_bool_t rels_segment) {
    opc_bool_t ret=OPC_FALSE;
    if (OPC_SEGMENT_CONTENTTYPES==partName) {
        OPC_ASSERT(!rels_segment);
        ret=opcZipSegmentDelete(container->storage, &container->content_types_segment_id, NULL, NULL);
    } else if (OPC_SEGMENT_ROOTRELS==partName) {
        OPC_ASSERT(rels_segment);
        ret=opcZipSegmentDelete(container->storage, &container->rels_segment_id, NULL, NULL);
    } else {
        opcContainerPart *part=opcContainerInsertPart(container, partName, OPC_FALSE);
        if (NULL!=part) {
            if (rels_segment) {
                ret=opcZipSegmentDelete(container->storage, &part->rel_segment_id, NULL, NULL);
            } else {
                ret=opcZipSegmentDelete(container->storage, &part->first_segment_id, &part->last_segment_id, NULL);
            }
        }
    }
    return ret;
}


const xmlChar *opcContentTypeFirst(opcContainer *container) {
    if (container->type_items>0) {
        return container->type_array[0].type;
    } else {
        return NULL;
    }
}

const xmlChar *opcContentTypeNext(opcContainer *container, const xmlChar *type) {
    opcContainerType *t=insertType(container, type, OPC_FALSE);
    if (NULL!=t && t>=container->type_array && t+1<container->type_array+container->type_items) {
        return (t+1)->type;
    } else {
        return NULL;
    }
}

const xmlChar *opcExtensionFirst(opcContainer *container) {
    if (container->extension_items>0) {
        return container->extension_array[0].extension;
    } else {
        return NULL;
    }
}


const xmlChar *opcExtensionNext(opcContainer *container, const xmlChar *ext) {
    opcContainerExtension *e=opcContainerInsertExtension(container, ext, OPC_FALSE);
    if (NULL!=e && e>=container->extension_array && e+1<container->extension_array+container->extension_items) {
        return (e+1)->extension;
    } else {
        return NULL;
    }
}

const xmlChar *opcExtensionGetType(opcContainer *container, const xmlChar *ext) {
    opcContainerExtension *e=opcContainerInsertExtension(container, ext, OPC_FALSE);
    if (NULL!=e && e>=container->extension_array && e<container->extension_array+container->extension_items) {
        return e->type;
    } else {
        return NULL;
    }
}

const xmlChar *opcExtensionRegister(opcContainer *container, const xmlChar *ext, const xmlChar *type) {
    opcContainerType *_type=insertType(container, type, OPC_TRUE);
    opcContainerExtension *_ext=opcContainerInsertExtension(container, ext, OPC_TRUE);
    if (_ext!=NULL && _type!=NULL) {
        OPC_ASSERT(NULL==_ext->type);
        _ext->type=_type->type;
        return _ext->extension;
    } else {
        return NULL;
    }
}

opc_uint32_t opcRelationAdd(opcContainer *container, opcPart src, const xmlChar *rid, opcPart dest, const xmlChar *type) {
    opc_uint32_t ret=-1;
    opcContainerRelation **relation_array=NULL;
    opc_uint32_t *relation_items=NULL;
    if (OPC_PART_INVALID==src) {
        relation_array=&container->relation_array;
        relation_items=&container->relation_items;
    } else {
        opcContainerPart *src_part=opcContainerInsertPart(container, src, OPC_FALSE);
        if (NULL!=src_part) {
            relation_array=&src_part->relation_array;
            relation_items=&src_part->relation_items;
        }
    }
    opcContainerPart *dest_part=opcContainerInsertPart(container, dest, OPC_FALSE);
    char buf[OPC_MAX_PATH];
    strncpy(buf, (const char *)rid, OPC_MAX_PATH);
    opc_uint32_t counter=-1;
    opc_uint32_t id_len=splitRelPrefix(container, _X(buf), &counter);
    buf[id_len]=0;
    opc_uint32_t rel_id=createRelId(container, _X(buf), counter);

    if (NULL!=relation_array && NULL!=dest_part) {
        opcContainerRelationType *rel_type=(NULL!=type?opcContainerInsertRelationType(container, type, OPC_TRUE):NULL);
        opcContainerRelation *rel=opcContainerInsertRelation(relation_array, relation_items, rel_id, (NULL!=rel_type?rel_type->type:NULL), 0, dest_part->name);
        if (NULL!=rel) {
            OPC_ASSERT(rel>=*relation_array && rel<*relation_array+*relation_items);
            OPC_ASSERT(0==rel->target_mode);
            ret=rel_id;
        }
    }
    return ret;
}

opc_uint32_t opcRelationAddExternal(opcContainer *container, opcPart src, const xmlChar *rid, const xmlChar *target, const xmlChar *type) {
    opc_uint32_t ret=-1;
    opcContainerRelation **relation_array=NULL;
    opc_uint32_t *relation_items=NULL;
    if (OPC_PART_INVALID==src) {
        relation_array=&container->relation_array;
        relation_items=&container->relation_items;
    } else {
        opcContainerPart *src_part=opcContainerInsertPart(container, src, OPC_FALSE);
        if (NULL!=src_part) {
            relation_array=&src_part->relation_array;
            relation_items=&src_part->relation_items;
        }
    }
    opcContainerExternalRelation *_target=insertExternalRelation(container, target, OPC_TRUE);
    char buf[OPC_MAX_PATH];
    strncpy(buf, (const char *)rid, OPC_MAX_PATH);
    opc_uint32_t counter=-1;
    opc_uint32_t id_len=splitRelPrefix(container, _X(buf), &counter);
    buf[id_len]=0;
    opc_uint32_t rel_id=createRelId(container, _X(buf), counter);

    if (NULL!=relation_array && NULL!=_target) {
        opcContainerRelationType *rel_type=(NULL!=type?opcContainerInsertRelationType(container, type, OPC_TRUE):NULL);
        opcContainerRelation *rel=opcContainerInsertRelation(relation_array, relation_items, rel_id, (NULL!=rel_type?rel_type->type:NULL), 0, _target->target);
        if (NULL!=rel) {
            OPC_ASSERT(rel>=*relation_array && rel<*relation_array+*relation_items);
            rel->target_mode=1;
            ret=rel_id;
        }
    }
    return ret;
}

static inline int qname_level_cmp_fct(const void *key, opc_uint32_t v, const void *array_, opc_uint32_t item) {
    opcQNameLevel_t *q1=(opcQNameLevel_t*)key;
    opcQNameLevel_t *q2=&((opcQNameLevel_t*)array_)[item];
    int const ns_cmp=(NULL==q1->ns?(NULL==q2->ns?0:-1):(NULL==q2->ns?+1:xmlStrcmp(q1->ns, q2->ns)));
    return (0==ns_cmp?xmlStrcmp(q1->ln, q2->ln):ns_cmp);
}


opc_error_t opcQNameLevelAdd(opcQNameLevel_t **list_array, opc_uint32_t *list_items, opcQNameLevel_t *item) {
    opc_uint32_t i=0;
    opc_error_t ret=OPC_ERROR_NONE;
    if (!findItem(*list_array, *list_items, item, 0, qname_level_cmp_fct, &i)) {
        if (NULL!=ensureItem((void**)list_array, *list_items, sizeof(opcQNameLevel_t))) {
            ensureGap(*list_array, *list_items, i);
            (*list_array)[i]=*item;
        } else {
            ret=OPC_ERROR_MEMORY;
        }
    }
    return ret;
}

opcQNameLevel_t* opcQNameLevelLookup(opcQNameLevel_t *list_array, opc_uint32_t list_items, const xmlChar *ns, const xmlChar *ln) {
    opcQNameLevel_t item;
    item.level=0;
    item.ln=(xmlChar *)ln;
    item.ns=ns;
    opc_uint32_t i=0;
    opc_bool_t ret=NULL!=list_array && list_items>0 && findItem(list_array, list_items, &item, 0, qname_level_cmp_fct, &i);
    return (ret?list_array+i:NULL);
}

opc_error_t opcQNameLevelCleanup(opcQNameLevel_t *list_array, opc_uint32_t *list_items, opc_uint32_t level, opc_uint32_t *max_level) {
    opc_uint32_t i=0;
    for(opc_uint32_t j=0;j<*list_items;j++) {
        if (list_array[j].level>=level) {
            OPC_ASSERT(list_array[j].level==level); // cleanup should be called for every level...
            if (NULL!=list_array[j].ln) xmlFree(list_array[j].ln);
            // list_array[j].ns is managed by ther parser...
        } else {
            if (NULL!=max_level && list_array[j].level>*max_level) *max_level=list_array[j].level;
            list_array[i++]=list_array[j];
        }
    }
    *list_items=i;
    return OPC_ERROR_NONE;
}

opc_error_t opcQNameLevelPush(opcQNameLevel_t **list_array, opc_uint32_t *list_items, opcQNameLevel_t *item) {
    opc_error_t ret=OPC_ERROR_NONE;
    if (NULL!=(ensureItem((void**)list_array, *list_items, sizeof(opcQNameLevel_t)))) {
        (*list_array)[*list_items]=*item;
        (*list_items)++;
    } else {
        ret=OPC_ERROR_MEMORY;
    }
    return ret;
}

opc_bool_t opcQNameLevelPopIfMatch(opcQNameLevel_t *list_array, opc_uint32_t *list_items, const xmlChar *ns, const xmlChar *ln, opc_uint32_t level) {
    opc_bool_t ret=*list_items>0 && list_array[(*list_items)-1].level==level;
    if (ret) {
        OPC_ASSERT(0==xmlStrcmp(list_array[(*list_items)-1].ln, ln) && 0==xmlStrcmp(list_array[(*list_items)-1].ns, ns));
        OPC_ASSERT(*list_items>0);
        if (NULL!=list_array[(*list_items)-1].ln) xmlFree(list_array[(*list_items)-1].ln);
        (*list_items)--;
    }
    return ret;
}


const xmlChar *opcRelationTypeFirst(opcContainer *container) {
    if (container->relationtype_items>0) {
        return container->relationtype_array[0].type;
    } else {
        return NULL;
    }
}

const xmlChar *opcRelationTypeNext(opcContainer *container, const xmlChar *type) {
    opcContainerRelationType* t=opcContainerInsertRelationType(container, type, OPC_FALSE);
    if (NULL!=t && t>=container->relationtype_array && t+1<container->relationtype_array+container->relationtype_items) {
        return (t+1)->type;
    } else {
        return NULL;
    }
}

const xmlChar *opcExternalTargetFirst(opcContainer *container) {
    if (container->externalrelation_items>0) {
        return container->externalrelation_array[0].target;
    } else {
        return NULL;
    }
}

const xmlChar *opcExternalTargetNext(opcContainer *container, const xmlChar *target) {
    opcContainerExternalRelation*e=insertExternalRelation(container, target, OPC_FALSE);
    if (NULL!=e && e>=container->externalrelation_array && e+1<container->externalrelation_array+container->externalrelation_items) {
        return (e+1)->target;
    } else {
        return NULL;
    }
}
