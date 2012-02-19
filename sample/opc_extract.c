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
    Extract a part from an OPC containers.

    Ussage:
    opc_extract FILENAME PARTNAME

    Sample:
    opc_extract OOXMLI1.docx "word/document.xml"
*/


#include <opc/opc.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif
#ifdef WIN32
#include <crtdbg.h>
#endif

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef WIN32
    _setmode( _fileno( stdout ), _O_BINARY ); // make sure LF are not translated to CR LF on windows...
#endif
    if (OPC_ERROR_NONE==opcInitLibrary() && 3==argc) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL))) {
            opcPart part=OPC_PART_INVALID;
            if ((part=opcPartFind(c, _X(argv[2]), NULL, 0))!=OPC_PART_INVALID) {
                opcContainerInputStream *stream=NULL;
                if ((stream=opcContainerOpenInputStream(c, part))!=NULL){
                    opc_uint8_t buf[100];
                    opc_uint32_t len=0;
                    while((len=opcContainerReadInputStream(stream, buf, sizeof(buf)))>0) {
                        fwrite(buf, sizeof(opc_uint8_t), len, stdout);
                    }
                    opcContainerCloseInputStream(stream);
                } else {
                    fprintf(stderr, "ERROR: part \"%s\" could not be opened for reading.\n", argv[2]);
                }
            } else {
                fprintf(stderr, "ERROR: part \"%s\" could not be opened in \"%s\".\n", argv[2], argv[1]);
            }
            opcContainerClose(c, OPC_CLOSE_NOW);
        } else {
            fprintf(stderr, "ERROR: file \"%s\" could not be opened.\n", argv[1]);
        }
        opcFreeLibrary();
    } else if (2==argc) {
        fprintf(stderr, "ERROR: initialization of libopc failed.\n");    
    } else {
        fprintf(stderr, "opc_extract FILENAME PART\n\n");
        fprintf(stderr, "Sample: opc_extract test.docx word/document.xml\n");
    }
#ifdef WIN32
    OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
    return 0;
}
