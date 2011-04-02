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
        printf("</p>\n");
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

int main( int argc, const char* argv[] )
{
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        opcPart part=opcPartOpen(c, _X("/word/document.xml"), NULL, 0);
        if (OPC_PART_INVALID!=part) {
                opcXmlReader *reader=opcXmlReaderOpen(c, part, NULL, 0, 0);
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
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
    opcFreeLibrary();
    return 0;
}

