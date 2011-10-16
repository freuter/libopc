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
    Extract all images from an OPC container.

    Ussage:
    opc_image FILENAME [OUTPATH]

    The OUTPATH must exist.

    Sample:
    opc_image OOXMLI1.docx C:\Users\flr\Pictures
*/

#include <opc/opc.h>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

static void extract(opcContainer *c, opcPart p, const char *path) {
    char filename[OPC_MAX_PATH];
    opc_uint32_t i=xmlStrlen(p);
    while(i>0 && p[i]!='/') i--;
    if (p[i]=='/') i++;    
    strcpy(filename, path);
    strcat(filename,  (char *)(p+i));    
    FILE *out=fopen(filename, "wb");
    if (NULL!=out) {
        opcContainerInputStream *stream=opcContainerOpenInputStream(c, p);
        if (NULL!=stream) {
            opc_uint32_t  ret=0;
            opc_uint8_t buf[100];
            while((ret=opcContainerReadInputStream(stream, buf, sizeof(buf)))>0) {
                fwrite(buf, sizeof(char), ret, out);
            }
            opcContainerCloseInputStream(stream);
        }
        fclose(out);
    }
}

int main( int argc, const char* argv[] )
{    
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    opcInitLibrary();
    opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
    const char *path=(argc>1?argv[2]:"");
    if (NULL!=c) {
        for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
            const xmlChar *type=opcPartGetType(c, part);
            if (xmlStrcmp(type, _X("image/jpeg"))==0) {
                extract(c, part, path);
            } else if (xmlStrcmp(type, _X("image/png"))==0) {
                extract(c, part, path);
            } else {
                printf("skipped %s of type %s\n", part, type);
            }
        }
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
    opcFreeLibrary();
    return 0;
}

