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



static inline int _opcXmlTextReaderRead(opcXmlReader *reader) {
    return (1==reader->reader_mce?mceTextReaderPostprocess(reader->reader, &reader->mce, xmlTextReaderRead(reader->reader)):xmlTextReaderRead(reader->reader));
}

static inline int _opcXmlTextReaderNext(opcXmlReader *reader) {
    return (1==reader->reader_mce?mceTextReaderPostprocess(reader->reader, &reader->mce, xmlTextReaderNext(reader->reader)):xmlTextReaderNext(reader->reader));
}

opc_error_t opcXmlUnderstandsNamespace(opcXmlReader *reader, const xmlChar *ns) {
    mceCtxUnderstandsNamespace(&reader->mce, ns);
    return OPC_ERROR_NONE;
}

opc_error_t opcXmlSetMCEProcessing(opcXmlReader *reader, opc_bool_t flag) {
    reader->reader_mce=(flag?1:0);
    return OPC_ERROR_NONE;
}

void opcXmlReaderStartDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
        mceCtxInit(&reader->mce);
        if(1!=_opcXmlTextReaderNext(reader)) {
            reader->error=OPC_ERROR_XML;
        }
    }
}

void opcXmlReaderEndDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
//        printf("%i %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader));
        OPC_ASSERT(0==xmlTextReaderDepth(reader->reader));
        mceCtxCleanup(&reader->mce);
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
    return (OPC_ERROR_NONE==reader->error
        && XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader->reader)
        && (ln==NULL || xmlStrEqual(xmlTextReaderConstLocalName(reader->reader), ln))
        && (ns==NULL || xmlStrEqual(xmlTextReaderConstNamespaceUri(reader->reader), ns))
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

