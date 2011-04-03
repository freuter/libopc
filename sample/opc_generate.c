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

static void normalize_name(char *dest, const xmlChar *src, opc_uint32_t len) {
    strncpy(dest, (const char *)src, len);
    dest[len-1]=0;
    for(opc_uint32_t i=0;0!=dest[i];i++) {
        if (dest[i]=='/' || dest[i]=='.') {
            dest[i]='_';
        }
    }
}

static void generate_relations(opcContainer *c, FILE *out, opcPart root) {
    for(opcRelation rel=opcRelationFirst(c, root)
       ;OPC_RELATION_INVALID!=rel
       ;rel=rel=opcRelationNext(c, root, rel)) {
            const xmlChar *prefix=NULL;
            opc_uint32_t counter=-1;
            const xmlChar *type=NULL;
            opcRelationGetInformation(c, root, rel, &prefix, &counter, &type);
            char buf[20]="";
            if (-1!=counter) {
                sprintf(buf, "%i", counter);
            }
            opcPart internal_target=opcRelationGetInternalTarget(c, root, rel);
            if (OPC_PART_INVALID!=internal_target) {
                char part[OPC_MAX_PATH]="";
                normalize_name(part, internal_target, sizeof(part));
                fprintf(out, "     %sopcRelationAdd(c, %s, _X(\"%s%s\"), create_%s(c), _X(\"%s\"));\n", 
                    (OPC_PART_INVALID==root?"":"     "), 
                    (OPC_PART_INVALID==root?"OPC_PART_INVALID":"ret"), 
                    prefix, buf, part, type);
            } else {
                const xmlChar *external_target=opcRelationGetExternalTarget(c, root, rel);
                if (NULL!=external_target) {
                    //@TODO!
                }
            }
    }
}

static void generate_binary_data(opcContainer *c, FILE *out, opcContainerInputStream *in) {
    fprintf(out, "              static opc_uint8_t data[]={\n");
    opc_uint8_t buf[100];
    opc_uint32_t len=0;
    char cont=' ';
    while((len=opcContainerReadInputStream(in, buf, sizeof(buf)))>0) {
        fprintf(out, "              ");
        for(opc_uint32_t i=0;i<len;i++) {
            fprintf(out, "%c 0x%02X", cont, buf[i]);
            cont=',';
        }
        fprintf(out, "\n");
    }
    fprintf(out, "              };\n");
}

static void generate_parts(opcContainer *c, FILE *out) {
    for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
            char norm_part[OPC_MAX_PATH]="";
            normalize_name(norm_part, part, sizeof(norm_part));
            fprintf(out, "static opcPart create_%s(opcContainer *c);\n", norm_part);
    }
    fprintf(out, "\n");
    for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
            char norm_part[OPC_MAX_PATH]="";
            normalize_name(norm_part, part, sizeof(norm_part));
            const xmlChar *override_type=opcPartGetTypeEx(c, part, OPC_TRUE);
            fprintf(out, "static opcPart create_%s(opcContainer *c) {\n", norm_part);
            if (NULL!=override_type) {
                fprintf(out, "     opcPart ret=opcPartOpen(c, _X(\"%s\"), _X(\"%s\"), 0);\n", part, override_type);
            } else {
                fprintf(out, "     opcPart ret=opcPartOpen(c, _X(\"%s\"), NULL, 0);\n", part);
            }
            if (NULL!=override_type) {
                fprintf(out, "     if (OPC_PART_INVALID==ret && OPC_PART_INVALID!=(ret=opcPartCreate(c, _X(\"%s\"), _X(\"%s\"), 0))) {\n", part, override_type);
            } else {
                fprintf(out, "     if (OPC_PART_INVALID==ret && OPC_PART_INVALID!=(ret=opcPartCreate(c, _X(\"%s\"), NULL, 0))) {\n", part);
            }
            fprintf(out, "         //adding content\n");
            fprintf(out, "          opcContainerOutputStream *out=opcContainerCreateOutputStream(c, ret);\n");
            fprintf(out, "          if (NULL!=out) {\n");
            opcContainerInputStream *in=opcContainerOpenInputStream(c, part);
            if (NULL!=in) {
                generate_binary_data(c, out, in);
                opcContainerCloseInputStream(in);
            }
            fprintf(out, "              opcContainerWriteOutputStream(out, (const opc_uint8_t*)data, sizeof(data));\n");
            fprintf(out, "              opcContainerCloseOutputStream(out);\n");
            fprintf(out, "          }\n");

            fprintf(out, "          // adding relations\n");
            generate_relations(c, out, part);
            fprintf(out, "     }\n");
            fprintf(out, "     return ret;\n");
            fprintf(out, "}\n");
            fprintf(out, "\n");
    }
}

static void generate(opcContainer *c, FILE *out) {
    fprintf(out, "#include <opc/opc.h>\n");
    fprintf(out, "\n");
    generate_parts(c, out);
    fprintf(out, "void generate(opcContainer *c, FILE *out) {\n");
    fprintf(out, "     // adding registered extensions\n");
    for(const xmlChar *ext=opcExtensionFirst(c);NULL!=ext;ext=opcExtensionNext(c, ext)) {
        const xmlChar *type=opcExtensionGetType(c, ext);
        fprintf(out, "     opcExtensionRegister(c, _X(\"%s\"), _X(\"%s\"));\n", ext, type);
    }
    fprintf(out, "     // adding root relations\n");
    generate_relations(c, out, OPC_PART_INVALID);
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out, "int main( int argc, const char* argv[] ) {\n");
    fprintf(out, "    if (OPC_ERROR_NONE==opcInitLibrary() && 2==argc) {\n");
    fprintf(out, "         opcContainer *c=NULL;\n");
    fprintf(out, "         if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_WRITE_ONLY, NULL, NULL))) {\n");
    fprintf(out, "              generate(c, stdout);\n");
    fprintf(out, "              opcContainerClose(c, OPC_CLOSE_NOW);\n");
    fprintf(out, "         }\n");
    fprintf(out, "    }\n");
    fprintf(out, "}\n");
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==opcInitLibrary() && argc>=2) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL))) {
            FILE *out=stdout;
            if (argc>=3) {
                out=fopen(argv[2], "w");
            }
            generate(c, out);
            if (stdout!=out) {
                fclose(out);
            }
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
        printf("Sample: opc_dump test.docx\n");
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}

