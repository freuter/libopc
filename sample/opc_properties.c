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
    Dump core properties of an OPC container.

    Ussage:
    opc_properties FILENAME

    Sample:
    opc_properties OOXMLI1.docx
*/

#include <opc/opc.h>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

static void printValue(opcDCSimpleType_t *v) {
    printf("\"%s\"", v->str);
    if (NULL!=v->lang) 
        printf("@%s", v->lang);
}

static void printPair(const char *name, opcDCSimpleType_t *value) {
    printf("%s=", name);
    printValue(value);
    printf("\n");
}

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==opcInitLibrary() && 2==argc) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_WRITE, NULL, NULL))) {
            opcProperties_t cp;
            opcCorePropertiesInit(&cp);
            opcCorePropertiesRead(&cp, c);
//            opcCorePropertiesSetString(&cp.category, _X("category"));
//            opcCorePropertiesSetString(&cp.contentStatus, _X("contentStatus"));
//            opcCorePropertiesSetString(&cp.created, _X("2011-04-13T06:24:00Z"));

            if (NULL!=cp.category) printf("category=\"%s\"\n", cp.category);
            if (NULL!=cp.contentStatus) printf("contentStatus=\"%s\"\n", cp.contentStatus);
            if (NULL!=cp.created) printf("created=\"%s\"\n", cp.created);
            if (NULL!=cp.creator.str) printPair("creator", &cp.creator);
            if (NULL!=cp.description.str) printPair("description", &cp.description);
            if (NULL!=cp.identifier.str) printPair("identifier", &cp.identifier);
            if (cp.keyword_items>0) {
                printf("keywords={");
                for(opc_uint32_t i=0;i<cp.keyword_items;i++) {
                    printValue(&cp.keyword_array[i]);
                    if (i+1<cp.keyword_items) printf(", ");
                }
                printf("}\n");
            }
            if (NULL!=cp.language.str) printPair("language", &cp.language);
            if (NULL!=cp.lastModifiedBy) printf("lastModifiedBy=\"%s\"\n", cp.lastModifiedBy);
            if (NULL!=cp.lastPrinted) printf("lastPrinted=\"%s\"\n", cp.lastPrinted);
            if (NULL!=cp.modified) printf("modified=\"%s\"\n", cp.modified);
            if (NULL!=cp.revision) printf("revision=\"%s\"\n", cp.revision);
            if (NULL!=cp.subject.str) printPair("subject", &cp.subject);
            if (NULL!=cp.title.str) printPair("title", &cp.title);
            if (NULL!=cp.version) printf("version=\"%s\"\n", cp.version);
            opcCorePropertiesWrite(&cp, c);
            opcCorePropertiesCleanup(&cp);
            opcContainerClose(c, OPC_CLOSE_NOW);
        } else {
            printf("ERROR: \"%s\" could not be opened.\n", argv[1]);
            err=OPC_ERROR_STREAM;
        }
        opcFreeLibrary();
    } else if (2==argc) {
        printf("ERROR: initialization of libopc failed.\n");    
        err=OPC_ERROR_STREAM;
    } else {
        printf("opc_dump FILENAME.\n\n");
        printf("Sample: opc_properties test.docx\n");
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
#ifdef WIN32
    OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
    return (OPC_ERROR_NONE==err?0:3);
}

