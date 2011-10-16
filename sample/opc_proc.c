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
    if (OPC_ERROR_NONE==opcInitLibrary() && argc>1) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_WRITE, NULL, NULL))) {
            pbool_t closed=PFALSE;
            for(puint32_t i=2;i<argc;i++) {
                if (xmlStrcmp(_X(argv[i]), _X("--dump"))==0) {
                    opcContainerDump(c, stdout);
                } else if (xmlStrcmp(_X(argv[i]), _X("--create"))==0 && i+4<argc) {
                    const xmlChar *part_name=_X(argv[i+1]);
                    const xmlChar *part_type=_X(argv[i+2]);
                    const puint32_t part_flags=atoi(argv[i+3]);
                    if (xmlStrcasecmp(part_type, _X("NULL"))==0) {
                        part_type=NULL;
                    }
                    opcPart part=opcPartCreate(c, part_name, part_type, part_flags);
                    if (OPC_PART_INVALID!=part) {
                        opcContainerOutputStream* stream = opcContainerCreateOutputStream(c, part, OPC_COMPRESSIONOPTION_NORMAL);
                        if (stream != NULL) {
                            const char *filename=argv[i+4];
                            FILE *in=fopen(filename, "rb");
                            if (NULL!=in) {
                                int ret=0;
                                opc_uint8_t buf[100];
                                while((ret=fread(buf, sizeof(opc_uint8_t), sizeof(buf), in))>0) {
                                    opcContainerWriteOutputStream(stream, buf, ret);
                                }
                                fclose(in);
                            }
                            opcContainerCloseOutputStream(stream);
                        }
                    }
                    i+=4;
                } else if (xmlStrcmp(_X(argv[i]), _X("--delete"))==0 && i+1<argc) {
                    const xmlChar *part_name=_X(argv[i+1]);
                    OPC_ENSURE(OPC_ERROR_NONE==opcPartDelete(c, part_name)); 
                    i+=1;
                } else {
                    printf("ERROR: unknown command \"%s\".\n", argv[i]);
                    return 3;
                }
            }
            if (!closed) {
                opcContainerClose(c, OPC_CLOSE_NOW);
            }
        } else {
            printf("ERROR: \"%s\" could not be opened.\n", argv[1]);
            err=OPC_ERROR_STREAM;
        }
        opcFreeLibrary();
    } else if (2==argc) {
        printf("ERROR: initialization of libopc failed.\n");    
        err=OPC_ERROR_STREAM;
    } else {
        printf("opc_proc FILENAME [COMMANDS].\n\n");
        printf("Sample: opc_proc test.docx --dump\n");
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}

