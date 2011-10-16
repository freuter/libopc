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
    Dump content of a part using either the binary or the XML interfaces.

    Ussage:
    opc_part FILENAME PARTNAME

    Sample:
    opc_part OOXMLI1.docx "word/document.xml"
*/

#include <opc/opc.h>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        opcPart part=opcPartFind(c, _X(argv[2]), NULL, 0);
        if (OPC_PART_INVALID!=part) {
            const xmlChar *type=opcPartGetType(c, part);
            opc_uint32_t type_len=xmlStrlen(type);
            opc_bool_t is_xml=NULL!=type && type_len>=3 && 'x'==type[type_len-3] && 'm'==type[type_len-2] && 'l'==type[type_len-1];
            fprintf(stderr, "type=%s is_xml=%i\n", type, is_xml);
            if (is_xml) {
                mceTextReader_t reader;
                if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &reader, part, NULL, 0, 0)) {
                    xmlTextWriter *writer=xmlNewTextWriterFile(NULL);
                    xmlTextWriterSetIndent(writer, 1);
                    if (NULL!=writer) {
                        mceTextReaderDump(&reader, writer, 1);
                    }
                    xmlFreeTextWriter(writer);
                    mceTextReaderCleanup(&reader);
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

