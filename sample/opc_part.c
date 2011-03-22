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

static void dumpXml(opcXmlReader *reader) {
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
                dumpXml(reader);
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
        printf("%s", opcXmlReaderConstValue(reader));
    }
}

static void dumpText(opcXmlReader *reader) {
    if (opcXmlReaderStartElement(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("t"))) {
        if (opcXmlReaderStartAttributes(reader)) {
            do {
            } while (!opcXmlReaderEndAttributes(reader));
        }
        if (opcXmlReaderStartChildren(reader)) {
            do {
                if (opcXmlReaderStartText(reader)) {
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
            } while (!opcXmlReaderEndChildren(reader));
        }
    } else if (opcXmlReaderStartElement(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("p"))) {
        if (opcXmlReaderStartAttributes(reader)) {
            do {
            } while (!opcXmlReaderEndAttributes(reader));
        }
        printf("<p>");
        if (opcXmlReaderStartChildren(reader)) {
            do {
                dumpText(reader);
            } while (!opcXmlReaderEndChildren(reader));
        }
        printf("</p>");
    } else if (opcXmlReaderStartElement(reader, NULL, NULL)) {
        if (opcXmlReaderStartAttributes(reader)) {
            do {
            } while (!opcXmlReaderEndAttributes(reader));
        }
        if (opcXmlReaderStartChildren(reader)) {
            do {
                dumpText(reader);
            } while (!opcXmlReaderEndChildren(reader));
        }
    } else if (opcXmlReaderStartText(reader)) {
    }
}


static void extract(opcContainer *c, opcPart p) {
    opc_uint32_t i=xmlStrlen(p);
    while(i>0 && p[i]!='/') i--;
    if (p[i]=='/') i++;
    FILE *out=fopen((char *)(p+i), "wb");
    if (NULL!=out) {
        opcInputStream *stream=opcInputStreamOpen(c, p);
        if (NULL!=stream) {
            int ret=0;
            char buf[100];
            while((ret=opcInputStreamRead(stream, buf, sizeof(buf)))>0) {
                fwrite(buf, sizeof(char), ret, out);
            }
            opcInputStreamClose(stream);
        }
        fclose(out);
    }
}


int main( int argc, const char* argv[] )
{
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X("OOXMLI4.docx"), OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        {
        for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
            const xmlChar *type=opcPartGetType(c, part);
            if (xmlStrcmp(type, _X("image/jpeg"))==0) {
                extract(c, part);
            } else if (xmlStrcmp(type, _X("image/png"))==0) {
                extract(c, part);
            } else {
                printf("skipped %s of type %s\n", part, type);
            }
        }
        }
        opcPart part=opcPartOpen(c, _X("/word/document.xml"), NULL, 0);
        if (OPC_PART_INVALID!=part) {
            opcInputStream *stream=opcInputStreamOpen(c, part);
            if (NULL!=stream) {
                int ret=0;
                char buf[100];
                while((ret=opcInputStreamRead(stream, buf, sizeof(buf)))>0) {
                    printf("%.*s", ret, buf);
                }
                opcInputStreamClose(stream);
                printf("\n");
            }
            {
                opcXmlReader *reader=opcXmlReaderOpen(c, part);
                if (NULL!=reader) {
                    opcXmlReaderStartDocument(reader);
                    dumpXml(reader);
                    opcXmlReaderEndDocument(reader);
                    opcXmlReaderClose(reader);
                }
            }
            {
                opcXmlReader *reader=opcXmlReaderOpen(c, part);
                if (NULL!=reader) {
                    opcXmlReaderStartDocument(reader);
                    printf("<html>\n");
                    printf("<head>\n");
                    printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
                    printf("</head>\n");
                    printf("<body>\n");
                    dumpText(reader);
                    printf("<body>\n");
                    printf("</html>\n");
                    opcXmlReaderEndDocument(reader);
                    opcXmlReaderClose(reader);
                }
            }
            opcPartRelease(c, part);
        }
        opcContainerClose(c, OPC_CLOSE_NOW);

    }
    opcFreeLibrary();
    return 0;
}

