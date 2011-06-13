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
/*
    Extract all text of an Word document as HTML.

    Ussage:
    opc_text FILENAME

    Sample:
    opc_text OOXMLI1.docx
*/


#include <opc/opc.h>
#include <stdio.h>
#include <time.h>


static void dumpText(mceTextReader_t *reader) {
    mce_skip_attributes(reader);
    mce_start_children(reader) {
        mce_start_element(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("t")) {
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_start_text(reader) {
                    for(const xmlChar *txt=xmlTextReaderConstValue(reader->reader);0!=*txt;txt++) {
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
                } mce_end_text(reader);
            } mce_end_children(reader);
        } mce_end_element(reader);
        mce_start_element(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("p")) {
            printf("<p>");
            dumpText(reader);
            printf("</p>\n");
        } mce_end_element(reader);
        mce_start_element(reader, NULL, NULL) {
            dumpText(reader);
        } mce_end_element(reader);
    } mce_end_children(reader);
}

int main( int argc, const char* argv[] )
{
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        mceTextReader_t reader;
        if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &reader, _X("/word/document.xml"), NULL, 0, 0)) {
            mce_start_document(&reader) {
                mce_start_element(&reader, NULL, NULL) {
                    printf("<html>\n");
                    printf("<head>\n");
                    printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
                    printf("</head>\n");
                    printf("<body>\n");
                    dumpText(&reader);
                    printf("<body>\n");
                    printf("</html>\n");
                } mce_end_element(&reader);
            } mce_end_document(&reader);
            mceTextReaderCleanup(&reader);
        }
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
    opcFreeLibrary();
    return 0;
}

