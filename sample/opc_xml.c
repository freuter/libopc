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

static void dump(opcXmlReader *reader) {
    if (opcXmlReaderStartElement(reader, NULL, NULL)) {
        xmlChar *ln=xmlStrdup(opcXmlReaderLocalName(reader));
        printf("<%s>\n", ln);
        if (opcXmlReaderStartAttributes(reader)) {
            do {

            } while (!opcXmlReaderEndAttributes(reader));
        }
        if (opcXmlReaderStartChildren(reader)) {
            do {
                dump(reader);
            } while (!opcXmlReaderEndChildren(reader));
        }
        printf("</%s>\n", ln);
        xmlFree(ln);
    } else if (opcXmlReaderStartText(reader)) {

    }
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    if (OPC_ERROR_NONE==opcInitLibrary() && 3==argc) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL))) {
            opcContainerDump(c, stdout);
            opcPart part=OPC_PART_INVALID;
            if ((part=opcPartOpen(c, _X(argv[2]), NULL, 0))!=OPC_PART_INVALID) {
                opcXmlReader *reader=NULL;
                if ((reader=opcXmlReaderOpen(c, part, NULL, NULL, 0))!=NULL){
                    opcXmlReaderStartDocument(reader);
                    dump(reader);
                    opcXmlReaderEndDocument(reader);
                    opcXmlReaderClose(reader);
                } else {
                    printf("ERROR: part \"%s\" could not be opened for XML reading.\n", argv[2]);
                }
                opcPartRelease(c, part);
            } else {
                printf("ERROR: part \"%s\" could not be opened in \"%s\".\n", argv[2], argv[1]);
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
