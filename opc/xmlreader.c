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


opcXmlReader* opcXmlReaderOpenEx(opcContainer *container, opc_uint32_t segment_id, const char * URL, const char * encoding, int options) {
    opcContainerInputStream* stream=opcContainerOpenInputStreamEx(container, segment_id);
    if (NULL!=stream) {
        OPC_ASSERT(NULL==stream->reader);
        stream->reader=xmlReaderForIO((xmlInputReadCallback)opcContainerReadInputStream, (xmlInputCloseCallback)opcContainerCloseInputStream, stream, URL, encoding, options);
        return stream;
    } else {
        return NULL;
    }
}

opc_error_t opcXmlReaderClose(opcXmlReader *reader) {
    opc_error_t ret=OPC_ERROR_NONE;
    if (NULL!=reader && NULL!=reader->reader) {
        if (0!=xmlTextReaderClose(reader->reader) && OPC_ERROR_NONE==ret) {
            ret=OPC_ERROR_STREAM;
        }
        OPC_ASSERT(NULL==reader->segmentInputStream);        
    } else {
        ret=OPC_ERROR_STREAM;
    }
    return ret;
}


void opcXmlReaderStartDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
        if(1!=xmlTextReaderNext(reader->reader)) {
            reader->error=OPC_ERROR_XML;
        }
    }
}

void opcXmlReaderEndDocument(opcXmlReader *reader) {
    if (OPC_ERROR_NONE==reader->error) {
//        printf("%i %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader));
        if(XML_READER_TYPE_NONE!=xmlTextReaderNodeType(reader->reader)) {
            reader->error=OPC_ERROR_XML;
        }
    }
}

const xmlChar *opcXmlReaderLocalName(opcXmlReader *reader) {
    return xmlTextReaderConstLocalName(reader->reader);
}

const xmlChar *opcXmlReaderConstValue(opcXmlReader *reader) {
    return xmlTextReaderConstValue(reader->reader);
}

opc_bool_t opcXmlReaderStartElement(opcXmlReader *reader, xmlChar *ns, xmlChar *ln) {
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
            if(1==xmlTextReaderRead(reader->reader)) {
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
            OPC_ENSURE(-1!=xmlTextReaderRead(reader->reader));
            reader->reader_consume_element=0;
            reader->reader_element_handled=1;
            return OPC_TRUE;
        } else {
            if (reader->reader_consume_element) {
                if (!reader->reader_element_handled) {
                    printf("UNHANDLED %i %s\n", xmlTextReaderNodeType(reader->reader), xmlTextReaderConstLocalName(reader->reader));
                }
                OPC_ENSURE(-1!=xmlTextReaderNext(reader->reader));
            }
            reader->reader_consume_element=1;
            reader->reader_element_handled=0;
            return OPC_FALSE;
        }
    } else {
        return OPC_TRUE;
    }
}



opcXmlReader *opcXmlReaderOpen(opcContainer *c, opcPart part) {
    OPC_ASSERT(part>=0 && part<c->part_items);
    opcContainerPart *cp=&c->part_array[part];
    OPC_ASSERT(cp->first_segment_id>=0 && cp->first_segment_id<c->segment_items);
    return opcXmlReaderOpenEx(c, cp->first_segment_id, NULL, NULL, 0);
}

