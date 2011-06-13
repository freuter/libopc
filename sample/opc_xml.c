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
    Dump an XML part using the non-MCE opcXmlReader.

    Ussage:
    opc_xml FILENAME PARTNAME

    Sample:
    opc_xml OOXMLI1.docx "word/document.xml"
*/

#include <opc/opc.h>
#include <stdio.h>
#include <time.h>

static void dumpElement(mceTextReader_t *reader) {
    xmlChar *ln=xmlStrdup(xmlTextReaderLocalName(reader->reader));
    printf("<%s>\n", ln);
    mce_start_attributes(reader) {
    } mce_end_attributes(reader);
    mce_start_children(reader) {
        mce_start_element(reader, NULL, NULL) {
            dumpElement(reader);
        } mce_end_element(reader);
        mce_start_text(reader) {
        } mce_end_text(reader);
    } mce_end_children(reader);
    printf("</%s>\n", ln);
    xmlFree(ln);
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    if (OPC_ERROR_NONE==opcInitLibrary() && 3==argc) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL))) {
            opcContainerDump(c, stdout);
            mceTextReader_t reader;
            if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &reader,  _X(argv[2]), NULL, 0, 0)) {
                mce_start_document(&reader) {
                    mce_start_element(&reader, NULL, NULL) {
                        dumpElement(&reader);
                    } mce_end_element(&reader);
                } mce_end_document(&reader);
                mceTextReaderCleanup(&reader);
            } else {
                printf("ERROR: part \"%s\" could not be opened in \"%s\" for XML reading.\n", argv[2], argv[1]);
            }
            opcContainerClose(c, OPC_CLOSE_NOW);
        } else {
            printf("ERROR: file \"%s\" could not be opened.\n", argv[1]);
        }
        opcFreeLibrary();
    } else if (2==argc) {
        printf("ERROR: initialization of libopc failed.\n");    
    } else {
        printf("opc_extract FILENAME.\n\n");
        printf("Sample: opc_extract test.docx word/document.xml\n");
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return 0;
}
