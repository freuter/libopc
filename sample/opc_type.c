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
    Corretly get the type and an Office document.

    Ussage:
    opc_type FILENAME

    Sample:
    opc_type OOXMLI1.docx
*/
#include <opc/opc.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    opcInitLibrary();
    for(int i=1;i<argc;i++) {
        opcContainer *c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL);
        opcRelation rel=opcRelationFind(c, OPC_PART_INVALID, NULL, _X("http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"));
        if (OPC_RELATION_INVALID!=rel) {
            opcPart mainPart=opcRelationGetInternalTarget(c, OPC_PART_INVALID, rel);
            if (OPC_PART_INVALID!=mainPart) {
                const xmlChar *type=opcPartGetType(c, mainPart);
                printf("Office Document Type: %s\n", type);
                if (0==xmlStrcmp(type, _X("application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"))) {
                    printf("WORD Document\n");
                } else if (0==xmlStrcmp(type, _X("application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"))) {
                    printf("POWERPOINT Document\n");
                } else if (0==xmlStrcmp(type, _X("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"))) {
                    printf("EXCEL Document\n");
                }
            }
        }
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
    opcFreeLibrary();
#ifdef WIN32
    OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
    return 0;
}


