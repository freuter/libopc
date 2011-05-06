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

opcXmlReader* opcXmlReaderOpenEx(opcContainer *container, const xmlChar *partName, opc_bool_t rels_segment, const char * URL, const char * encoding, int options) {
    opcContainerInputStream* stream=opcContainerOpenInputStreamEx(container, partName, rels_segment);
    if (NULL!=stream) {
        OPC_ASSERT(NULL==stream->reader);
        stream->reader=xmlReaderForIO((xmlInputReadCallback)opcContainerReadInputStream, 
                                      (xmlInputCloseCallback)opcContainerCloseInputStream, 
                                      stream, URL, encoding, options);
        return stream;
    } else {
        return NULL;
    }
}

opcXmlReader* opcXmlReaderOpen(opcContainer *container, const xmlChar *partName, const char * URL, const char * encoding, int options) {
    return opcXmlReaderOpenEx(container, partName, OPC_FALSE, URL, encoding, options);
}

opc_error_t opcXmlReaderClose(opcXmlReader *reader) {
    opc_error_t ret=OPC_ERROR_NONE;
    if (NULL!=reader && NULL!=reader->reader) {
        if (0!=xmlTextReaderClose(reader->reader) && OPC_ERROR_NONE==ret) {
            ret=OPC_ERROR_STREAM;
        }
        // WARNING: reader is not longer valid here, since xmlTextReaderClose call stream close which will kill reader
    } else {
        ret=OPC_ERROR_STREAM;
    }
    return ret;
}

static xmlChar *xmlStrDupArray(const xmlChar *value) {
    opc_uint32_t len=xmlStrlen(value);
    xmlChar *ret=(xmlChar *)xmlMalloc((2+len)*sizeof(xmlChar));
    opc_uint32_t j=0;
    for(opc_uint32_t i=0;i<len;i++) {
        while(i<len && (value[i]==' ' || value[i]=='\t' || value[i]=='\r' || value[i]=='\n')) i++; // skip preceeding spaces
        while(i<len && value[i]!=' ' && value[i]!='\t' && value[i]!='\r' && value[i]!='\n') ret[j++]=value[i++];
        ret[j++]=0;
    }
    ret[j]=0;
    return ret;
}

static char ns_mce[]="http://schemas.openxmlformats.org/markup-compatibility/2006";
static inline void _opcUpdateMCEContext(opcXmlReader *reader) {
    opc_bool_t done=OPC_FALSE;
    while (!done) {
        done=OPC_TRUE;
        if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader->reader)) {
            if (1==xmlTextReaderHasAttributes(reader->reader)) {
                if (1==xmlTextReaderMoveToFirstAttribute(reader->reader)) {
                    do {
                        if (0==xmlStrcmp(_X("Ignorable"), xmlTextReaderConstLocalName(reader->reader)) &&
                            0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                                xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader->reader));
                                for(xmlChar *prefix=v;*prefix!=0;prefix+=1+xmlStrlen(prefix)) {
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader->reader, prefix);
                                    if (NULL!=ns_ && NULL==opcQNameLevelLookup(reader->understands_array, reader->understands_items, ns_, NULL)) {
                                        OPC_ASSERT(xmlTextReaderDepth(reader->reader)>0); // remove one, since we need the element and not the attribute level
                                        opcQNameLevel_t item;
                                        opc_bzero_mem(&item, sizeof(item));
                                        item.level=xmlTextReaderDepth(reader->reader)-1;
                                        item.ln=NULL;
                                        item.ns=xmlTextReaderConstString(reader->reader, ns_);
                                        OPC_ASSERT(reader->ignored_max_level<=xmlTextReaderDepth(reader->reader)-1);
                                        reader->ignored_max_level=xmlTextReaderDepth(reader->reader)-1;
                                        OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelAdd(&reader->ignored_array, &reader->ignored_items, &item));
                                    }
                                }
                                xmlFree(v);
                        } else if (0==xmlStrcmp(_X("ProcessContent"), xmlTextReaderConstLocalName(reader->reader)) &&
                                   0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                                xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader->reader));
                                xmlChar *qname=v;
                                for(int qname_len=0;(qname_len=xmlStrlen(qname))>0;qname+=qname_len+1) {
                                    int prefix=0; while(qname[prefix]!=':' && qname[prefix]!=0) prefix++;
                                    OPC_ASSERT(prefix<=qname_len);
                                    int ln=(prefix<qname_len?prefix+1:0);
                                    if (prefix<qname_len) {
                                        qname[prefix]=0;
                                        prefix=0;
                                    };
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader->reader, qname+prefix);
                                    if (NULL!=ns_ && NULL==opcQNameLevelLookup(reader->understands_array, reader->understands_items, ns_, NULL)) {
                                        OPC_ASSERT(xmlTextReaderDepth(reader->reader)>0); // remove one, since we need the element and not the attribute level
                                        opcQNameLevel_t item;
                                        opc_bzero_mem(&item, sizeof(item));
                                        item.level=xmlTextReaderDepth(reader->reader)-1;
                                        item.ln=xmlStrdup(qname+ln);
                                        item.ns=xmlTextReaderConstString(reader->reader, ns_);
                                        OPC_ASSERT(reader->processcontent_max_level<=xmlTextReaderDepth(reader->reader)-1);
                                        reader->processcontent_max_level=xmlTextReaderDepth(reader->reader)-1;
                                        OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelAdd(&reader->processcontent_array, &reader->processcontent_items, &item));
                                    }
                                }
                                xmlFree(v);
                        }


                    } while (1==xmlTextReaderMoveToNextAttribute(reader->reader));
                }
                OPC_ENSURE(1==xmlTextReaderMoveToElement(reader->reader));
            }
            if (1==xmlTextReaderHasAttributes(reader->reader)) {
                xmlAttrPtr remove=NULL;
                if (1==xmlTextReaderMoveToFirstAttribute(reader->reader)) {
                    do {
                        if (NULL!=remove) {
                            xmlRemoveProp(remove); remove=NULL;
                        }
                        if (0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                            OPC_ASSERT(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader->reader)->type);
                            remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader->reader);
                        } else if (NULL!=opcQNameLevelLookup(reader->ignored_array, 
                                                             reader->ignored_items, 
                                                             xmlTextReaderConstNamespaceUri(reader->reader), 
                                                             NULL)) {
                            OPC_ASSERT(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader->reader)->type);
                            remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader->reader);
                        }
                    } while (1==xmlTextReaderMoveToNextAttribute(reader->reader));
                }
                OPC_ENSURE(1==xmlTextReaderMoveToElement(reader->reader));
                if (NULL!=remove) {
                    xmlRemoveProp(remove); remove=NULL;
                }
                OPC_ASSERT(NULL==remove);
            }
            opc_bool_t ignore=OPC_FALSE;
            opc_bool_t process_content=OPC_FALSE;
            if (0==xmlStrcmp(_X("AlternateContent"), xmlTextReaderConstLocalName(reader->reader)) &&
                0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                    ignore=OPC_TRUE;
                    process_content=OPC_TRUE;
            } else if (0==xmlStrcmp(_X("Choice"), xmlTextReaderConstLocalName(reader->reader)) &&
                       0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                    xmlChar *req=NULL;
                    if (1==xmlTextReaderMoveToAttribute(reader->reader, _X("Requires"))) {
                        req=xmlStrDupArray(xmlTextReaderConstValue(reader->reader));
                        OPC_ASSERT(1==xmlTextReaderMoveToElement(reader->reader));
                    } else if (1==xmlTextReaderMoveToAttributeNs(reader->reader, _X("Requires"), _X(ns_mce))) {
                        req=xmlStrDupArray(xmlTextReaderConstValue(reader->reader));
                        OPC_ASSERT(1==xmlTextReaderMoveToElement(reader->reader));
                    }
                    if (NULL!=req) {
                        ignore=OPC_TRUE;
                        const xmlChar *top_ln=(reader->skip_end_items>0?reader->skip_end_stack[reader->skip_end_items-1].ln:NULL);
                        const xmlChar *top_ns=(reader->skip_end_items>0?reader->skip_end_stack[reader->skip_end_items-1].ns:NULL);
                        OPC_ASSERT(NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns));
                        if (NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns)) {
                            if (0==reader->skip_end_stack[reader->skip_end_items-1].alternatecontent_handled) {
                                process_content=OPC_TRUE;
                                for(xmlChar *prefix=req;*prefix!=0;prefix+=1+xmlStrlen(prefix)) {
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader->reader, prefix);
                                    OPC_ASSERT(NULL!=ns_);
                                    process_content&=(NULL!=ns_ && NULL!=opcQNameLevelLookup(reader->understands_array, reader->understands_items, ns_, NULL));
                                }
                                reader->skip_end_stack[reader->skip_end_items-1].alternatecontent_handled=(process_content?1:0);
                            } else {
                                OPC_ASSERT(OPC_FALSE==process_content);
                            }
                        }
                        OPC_ASSERT(NULL!=req); xmlFree(req);
                    }
            } else if (0==xmlStrcmp(_X("Fallback"), xmlTextReaderConstLocalName(reader->reader)) &&
                       0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader->reader))) {
                    ignore=OPC_TRUE;
                    const xmlChar *top_ln=(reader->skip_end_items>0?reader->skip_end_stack[reader->skip_end_items-1].ln:NULL);
                    const xmlChar *top_ns=(reader->skip_end_items>0?reader->skip_end_stack[reader->skip_end_items-1].ns:NULL);
                    OPC_ASSERT(NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns));
                    if (NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns)) {
                        process_content=(1!=reader->skip_end_stack[reader->skip_end_items-1].alternatecontent_handled);
                    }
            } else {
                ignore=OPC_ERROR_NONE==reader->error && NULL!=opcQNameLevelLookup(reader->ignored_array, 
                                                                                  reader->ignored_items, 
                                                                                  xmlTextReaderConstNamespaceUri(reader->reader), 
                                                                                  NULL);
            }
            if (1==xmlTextReaderIsEmptyElement(reader->reader)) {
                if (xmlTextReaderDepth(reader->reader)<=reader->ignored_max_level) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->ignored_array, &reader->ignored_items, xmlTextReaderDepth(reader->reader), &reader->ignored_max_level));
                }
                if (xmlTextReaderDepth(reader->reader)<=reader->processcontent_max_level) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->processcontent_array, &reader->processcontent_items, xmlTextReaderDepth(reader->reader), &reader->processcontent_max_level));
                }
                if (ignore) {
                    OPC_ENSURE(1==xmlTextReaderRead(reader->reader)); // consume empty element
                    reader->reader_consume_element=1;
                    reader->reader_element_handled=0;
                    done=OPC_FALSE;
                }
            } else {
                if (ignore) {
                    if (!process_content) {
                        process_content=OPC_ERROR_NONE==reader->error && NULL!=opcQNameLevelLookup(reader->processcontent_array, 
                                                                                                   reader->processcontent_items, 
                                                                                                   xmlTextReaderConstNamespaceUri(reader->reader), 
                                                                                                   xmlTextReaderLocalName(reader->reader));
                    }
                    if (process_content) {
                        opcQNameLevel_t item;
                        opc_bzero_mem(&item, sizeof(item));
                        item.level=xmlTextReaderDepth(reader->reader);
                        item.ln=xmlStrdup(xmlTextReaderConstLocalName(reader->reader));
                        item.ns=xmlTextReaderConstString(reader->reader, xmlTextReaderConstNamespaceUri(reader->reader));
                        OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelPush(&reader->skip_end_stack, &reader->skip_end_items, &item));
                        OPC_ENSURE(1==xmlTextReaderRead(reader->reader)); // consume start element
                        reader->reader_consume_element=1;
                        reader->reader_element_handled=0;
                        done=OPC_FALSE;
                    } else {
                        if (xmlTextReaderDepth(reader->reader)<=reader->ignored_max_level) {
                            OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->ignored_array, &reader->ignored_items, xmlTextReaderDepth(reader->reader), &reader->ignored_max_level));
                        }
                        if (xmlTextReaderDepth(reader->reader)<=reader->processcontent_max_level) {
                            OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->processcontent_array, &reader->processcontent_items, xmlTextReaderDepth(reader->reader), &reader->processcontent_max_level));
                        }
                        OPC_ENSURE(1==xmlTextReaderNext(reader->reader)); // consume whole element
                        reader->reader_consume_element=1;
                        reader->reader_element_handled=0;
                        done=OPC_FALSE;
                    }
                }
            }
        } else if (XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(reader->reader)) {
            if (xmlTextReaderDepth(reader->reader)<=reader->ignored_max_level) {
                OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->ignored_array, &reader->ignored_items, xmlTextReaderDepth(reader->reader), &reader->ignored_max_level));
            }
            if (xmlTextReaderDepth(reader->reader)<=reader->processcontent_max_level) {
                OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->processcontent_array, &reader->processcontent_items, xmlTextReaderDepth(reader->reader), &reader->processcontent_max_level));
            }
            if (opcQNameLevelPopIfMatch(reader->skip_end_stack, 
                                        &reader->skip_end_items, 
                                        xmlTextReaderConstNamespaceUri(reader->reader), 
                                        xmlTextReaderConstLocalName(reader->reader),
                                        xmlTextReaderDepth(reader->reader))) {
                OPC_ENSURE(1==xmlTextReaderRead(reader->reader)); // consume end element
                done=OPC_FALSE;
            }
        }
    }
    if (0) {
        printf("mce: %i[%i] %s %s", xmlTextReaderNodeType(reader->reader), xmlTextReaderIsEmptyElement(reader->reader), xmlTextReaderConstNamespaceUri(reader->reader), xmlTextReaderConstLocalName(reader->reader));
        printf("<");
        for(opc_uint32_t i=0;i<reader->skip_end_items;i++) {
            printf("(%i;%s;%s)", reader->skip_end_stack[i].level, reader->skip_end_stack[i].ln, reader->skip_end_stack[i].ns);
            if (i+1<reader->skip_end_items) printf(" ");
        }
        printf(">");
        printf("\n");
    }
}


static inline int _opcXmlTextReaderRead(opcXmlReader *reader) {
    int ret=xmlTextReaderRead(reader->reader);
    if (1==ret && 1==reader->reader_mce) _opcUpdateMCEContext(reader);
    return ret;
}

static inline int _opcXmlTextReaderNext(opcXmlReader *reader) {
    int ret=xmlTextReaderNext(reader->reader);
    if (1==ret && 1==reader->reader_mce) _opcUpdateMCEContext(reader);
    return ret;
}

opc_error_t opcXmlUnderstandsNamespace(opcXmlReader *reader, const xmlChar *ns) {
    opcQNameLevel_t item;
    opc_bzero_mem(&item, sizeof(item));
    item.level=0;
    item.ln=NULL;
    item.ns=xmlTextReaderConstString(reader->reader, ns);
    opcQNameLevelAdd(&reader->understands_array, &reader->understands_items, &item);
    return OPC_ERROR_NONE;
}

opc_error_t opcXmlSetMCEProcessing(opcXmlReader *reader, opc_bool_t flag) {
    reader->reader_mce=(flag?1:0);
    return OPC_ERROR_NONE;
}

void opcXmlReaderStartDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
        if(1!=_opcXmlTextReaderNext(reader)) {
            reader->error=OPC_ERROR_XML;
        }
    }
}

void opcXmlReaderEndDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
//        printf("%i %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader));
        OPC_ASSERT(0==xmlTextReaderDepth(reader->reader));
        OPC_ENSURE(OPC_ERROR_NONE==opcQNameLevelCleanup(reader->ignored_array, &reader->ignored_items, xmlTextReaderDepth(reader->reader), &reader->ignored_max_level));
        if(XML_READER_TYPE_NONE!=xmlTextReaderNodeType(reader->reader)) {
            reader->error=OPC_ERROR_XML;
        }
    }
}

const xmlChar *opcXmlReaderLocalName(opcXmlReader *reader) {
    return xmlTextReaderConstLocalName(reader->reader);
}

const xmlChar *opcXmlReaderConstNamespaceUri(opcXmlReader *reader) {
    return xmlTextReaderConstNamespaceUri(reader->reader);
}

const xmlChar *opcXmlReaderConstPrefix(opcXmlReader *reader) {
    return xmlTextReaderConstPrefix(reader->reader);
}


const xmlChar *opcXmlReaderConstValue(opcXmlReader *reader) {
    return xmlTextReaderConstValue(reader->reader);
}

opc_bool_t opcXmlReaderStartElement(opcXmlReader *reader, xmlChar *ns, xmlChar *ln) {
//    printf("%i %s %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader), xmlTextReaderConstNamespaceUri(reader->reader));
    opc_bool_t ignore=OPC_ERROR_NONE==reader->error
                                   && XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader->reader)
                                   && NULL!=opcQNameLevelLookup(reader->ignored_array, reader->ignored_items, xmlTextReaderConstNamespaceUri(reader->reader), NULL);
    return (OPC_ERROR_NONE==reader->error
        && XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader->reader)
        && (ln==NULL || xmlStrEqual(xmlTextReaderConstLocalName(reader->reader), ln))
        && (ns==NULL || xmlStrEqual(xmlTextReaderConstNamespaceUri(reader->reader), ns))
        && !ignore
        && (1==(reader->reader_element_handled=1)));
}

opc_bool_t opcXmlReaderStartAttribute(opcXmlReader *reader, xmlChar *ns, xmlChar *ln) {
    return (OPC_ERROR_NONE==reader->error
        && XML_READER_TYPE_ATTRIBUTE==xmlTextReaderNodeType(reader->reader)
        && (ln==NULL || xmlStrEqual(xmlTextReaderConstLocalName(reader->reader), ln))
        && (ns==NULL || xmlStrEqual(xmlTextReaderConstNamespaceUri(reader->reader), ns))
        && (1==(reader->reader_element_handled=1)));
}

opc_bool_t opcXmlReaderStartText(opcXmlReader *reader) {
    return (OPC_ERROR_NONE==reader->error
        && (XML_READER_TYPE_TEXT==xmlTextReaderNodeType(reader->reader) || XML_READER_TYPE_SIGNIFICANT_WHITESPACE==xmlTextReaderNodeType(reader->reader))
        && (1==(reader->reader_element_handled=1)));
}

opc_bool_t opcXmlReaderStartAttributes(opcXmlReader *reader) {
    return OPC_ERROR_NONE==reader->error
        && (1==xmlTextReaderHasAttributes(reader->reader)) 
        && (1==xmlTextReaderMoveToFirstAttribute(reader->reader));
}

opc_bool_t opcXmlReaderEndAttributes(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
        if (1==xmlTextReaderMoveToNextAttribute(reader->reader)) {
            return OPC_FALSE;
        } else {
            if(1!=xmlTextReaderMoveToElement(reader->reader)) {
                reader->error=OPC_ERROR_XML;
            }
            return OPC_TRUE;
        }
    } else {
        return OPC_TRUE;
    }
}


opc_bool_t opcXmlReaderStartChildren(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
        if (0==xmlTextReaderIsEmptyElement(reader->reader)) {
            if(1==_opcXmlTextReaderRead(reader)) {
                reader->reader_consume_element=1;
                reader->reader_element_handled=0;
                return OPC_TRUE;
            } else {
                reader->error=OPC_ERROR_XML;
                return OPC_FALSE;
            }
        } else {
            reader->reader_consume_element=1;
            reader->reader_element_handled=1;
            return OPC_FALSE;
        }
    } else {
        return OPC_FALSE;
    }
}

opc_bool_t opcXmlReaderEndChildren(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
//        printf("%i %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader));
        if (XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(reader->reader)) {
            OPC_ENSURE(-1!=_opcXmlTextReaderRead(reader));
            reader->reader_consume_element=0;
            reader->reader_element_handled=1;
            return OPC_TRUE;
        } else {
            if (reader->reader_consume_element) {
                if (!reader->reader_element_handled) {
                    fprintf(stderr, "UNHANDLED %i %s \"%s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader), xmlTextReaderConstValue(reader->reader));
                }
                OPC_ENSURE(-1!=_opcXmlTextReaderNext(reader));
            }
            reader->reader_consume_element=1;
            reader->reader_element_handled=0;
            return OPC_FALSE;
        }
    } else {
        return OPC_TRUE;
    }
}



xmlDocPtr opcXmlReaderReadDoc(opcContainer *container, const xmlChar *partName, const char * URL, const char * encoding, int options) {
    opcContainerInputStream* stream=opcContainerOpenInputStreamEx(container, partName, OPC_FALSE);
    if (NULL!=stream) {
        OPC_ASSERT(NULL==stream->reader);
        xmlDocPtr doc=xmlReadIO((xmlInputReadCallback)opcContainerReadInputStream, 
                                (xmlInputCloseCallback)opcContainerCloseInputStream, 
                                stream, URL, encoding, options);
        return doc;
    } else {
        return NULL;
    }
}

