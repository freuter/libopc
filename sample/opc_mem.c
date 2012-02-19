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
    Dump all information about an OPC container via the memory-based OPC container interface.

    Ussage:
    opc_mem FILENAME

    Sample:
    opc_mem OOXMLI1.docx
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
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==opcInitLibrary() && 2==argc) {
        opcContainer *c=NULL;
        FILE *f=fopen(argv[1], "rb");
        fseek(f, 0, SEEK_END);
        opc_uint32_t data_len=ftell(f);
        fseek(f, 0, SEEK_SET);
        opc_uint8_t *data=(opc_uint8_t *)xmlMalloc(data_len);
        size_t fread_len=fread(data, 1, data_len, f);
        fclose(f);
	if (data_len!=fread_len) {
            printf("ERROR: reading file.\n");
	} else if (NULL!=(c=opcContainerOpenMem(data, data_len, OPC_OPEN_READ_ONLY, NULL))) {
            opcContainerDump(c, stdout);
            opcContainerClose(c, OPC_CLOSE_NOW);
        } else {
            printf("ERROR: \"%s\" could not be opened.\n", argv[1]);
            err=OPC_ERROR_STREAM;
        }
        xmlFree(data);
        opcFreeLibrary();
    } else if (2==argc) {
        printf("ERROR: initialization of libopc failed.\n");    
        err=OPC_ERROR_STREAM;
    } else {
        printf("opc_dump FILENAME.\n\n");
        printf("Sample: opc_dump test.docx\n");
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
#ifdef WIN32
    OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
    return (OPC_ERROR_NONE==err?0:3);	
}

