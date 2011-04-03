/**
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
#include <stdio.h>
#include <time.h>

static void dumpXml(opcXmlReader *reader, int level) {
    if (opcXmlReaderStartElement(reader, NULL, NULL)) {
        xmlChar *ln=xmlStrdup(opcXmlReaderLocalName(reader));
        xmlChar *prefix=(NULL!=opcXmlReaderConstPrefix(reader)?xmlStrdup(opcXmlReaderConstPrefix(reader)):NULL);
        if (NULL==prefix) {
            printf("<%s", ln);
        } else {
            printf("<%s:%s", prefix, ln);
        }
        if (opcXmlReaderStartAttributes(reader)) {
            do {
                if (opcXmlReaderStartAttribute(reader, _X("http://www.w3.org/2000/xmlns/"), NULL)) {
                    printf(" xmlns:%s=\"%s\"", opcXmlReaderLocalName(reader), opcXmlReaderConstValue(reader));
                } else if (opcXmlReaderStartAttribute(reader, NULL, NULL)) {
                    const xmlChar *p=opcXmlReaderConstPrefix(reader);
                    if (NULL==p) {
                        printf(" %s=\"%s\"", opcXmlReaderLocalName(reader), opcXmlReaderConstValue(reader));
                    } else {
                        printf(" %s:%s=\"%s\"", p, opcXmlReaderLocalName(reader), opcXmlReaderConstValue(reader));
                    }
                }
            } while (!opcXmlReaderEndAttributes(reader));
        }
        printf(">");
        if (opcXmlReaderStartChildren(reader)) {
            do {
                dumpXml(reader, level+1);
            } while (!opcXmlReaderEndChildren(reader));
        }
        if (NULL==prefix) {
            printf("</%s>", ln);
        } else {
            printf("</%s:%s>", prefix, ln);
            xmlFree(prefix);
        }
        xmlFree(ln);
    } else if (opcXmlReaderStartText(reader)) {
        const xmlChar *txt=opcXmlReaderConstValue(reader);
        for(const xmlChar *txt=opcXmlReaderConstValue(reader);0!=*txt;txt++) {
            switch(*txt) {
                case '<': 
                    printf("&lt;");
                    break;
                case '>': 
                    printf("&gt;");
                    break;
                case '&': 
                    printf("&amp;");
                    break;
                default:
                    putc(*txt, stdout);
                    break;
            }
        }
        
    }
}


int main( int argc, const char* argv[] )
{
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        opcPart part=opcPartOpen(c, _X(argv[2]), NULL, 0);
        if (OPC_PART_INVALID!=part) {
            const xmlChar *type=opcPartGetType(c, part);
            opc_uint32_t type_len=xmlStrlen(type);
            opc_bool_t is_xml=NULL!=type && type_len>=3 && 'x'==type[type_len-3] && 'm'==type[type_len-2] && 'l'==type[type_len-1];
            fprintf(stderr, "type=%s is_xml=%i\n", type, is_xml);
            if (is_xml) {
                opcXmlReader *reader=opcXmlReaderOpen(c, part, NULL, 0, 0);
                if (NULL!=reader) {
                    opcXmlReaderStartDocument(reader);
                    dumpXml(reader, 0);
                    opcXmlReaderEndDocument(reader);
                    opcXmlReaderClose(reader);
                }
            } else  {
                opcContainerInputStream *stream=opcContainerOpenInputStream(c, part);
                if (NULL!=stream) {
                    opc_uint32_t ret=0;
                    opc_uint8_t buf[100];
                    while((ret=opcContainerReadInputStream(stream, buf, sizeof(buf)))>0) {
                        fwrite(buf, sizeof(opc_uint8_t), ret, stdout);
                    }
                    opcContainerCloseInputStream(stream);
                }
            }
        }
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
    opcFreeLibrary();
    return 0;
}

