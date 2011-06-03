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
#include <libxml/xmlwriter.h>


static int  xmlOutputWrite(void * context, const char * buffer, int len) {
    FILE *out=(FILE*)context;
    return fwrite(buffer, sizeof(char), len, out);
}

static int xmlOutputClose(void * context) {
    return 0;
}

static void dumpPartsAsJSON(opcContainer *c, int indent) {
    printf("["); if (indent) printf("\n");
    opcPart part=OPC_PART_INVALID;
    opcPart next=OPC_PART_INVALID;
    for(part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=next) {
        next=opcPartGetNext(c, part);
        if (indent) {
            printf("  {\n    \"name\": \"%s\",\n    \"type\":\"%s\"\n  }%s\n", part, opcPartGetType(c, part), (OPC_PART_INVALID==next?"":","));
        } else {
            printf("{\"name\": \"%s\", \"type\":\"%s\"}%s", part, opcPartGetType(c, part), (OPC_PART_INVALID==next?"":","));
        }
    }
    printf("]"); if (indent) printf("\n");
}

int main( int argc, const char* argv[] )
{
    int ret=-1;
    time_t start_time=time(NULL);
    FILE *file=NULL;
    const xmlChar *containerPath8=NULL;
    const xmlChar *partName8=NULL;
    xmlTextWriter *writer=NULL;
    int writer_indent=0;
    opc_bool_t reader_mce=OPC_TRUE;
    for(int i=1;i<argc;i++) {
        if ((0==xmlStrcmp(_X("--understands"), _X(argv[i])) || 0==xmlStrcmp(_X("-u"), _X(argv[i]))) && i+1<argc) {
            const xmlChar *ns=_X(argv[++i]);
        } else if ((0==xmlStrcmp(_X("--out"), _X(argv[i])) || 0==xmlStrcmp(_X("--out"), _X(argv[i]))) && i+1<argc && NULL==file) {
            const char *filename=argv[++i];
            file=fopen(filename, "w");
        } else if (0==xmlStrcmp(_X("--indent"), _X(argv[i]))) {
            writer_indent=1;
        } else if (0==xmlStrcmp(_X("--raw"), _X(argv[i]))) {
            reader_mce=OPC_FALSE;
        } else if (NULL==containerPath8) {
            containerPath8=_X(argv[i]);
        } else if (NULL==partName8) {
            partName8=_X(argv[i]);
        } else {
            fprintf(stderr, "IGNORED: %s\n", argv[i]);
        }
    }
    if (NULL!=file) {
        xmlOutputBuffer *out=xmlOutputBufferCreateIO(xmlOutputWrite, xmlOutputClose, file, NULL);
        if (NULL!=out) {
            writer=xmlNewTextWriter(out);
        }
    } else {
        xmlOutputBuffer *out=xmlOutputBufferCreateIO(xmlOutputWrite, xmlOutputClose, stdout, NULL);
        if (NULL!=out) {
            writer=xmlNewTextWriter(out);
        }
    }
    if (NULL==containerPath8 || NULL==writer) {
        printf("mce_extract FILENAME.\n\n");
        printf("Sample: mce_extract test.docx word/document.xml\n");
    } else if (OPC_ERROR_NONE==opcInitLibrary()) {
        xmlTextWriterSetIndent(writer, writer_indent);
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(containerPath8, OPC_OPEN_READ_ONLY, NULL, NULL))) {
            if (NULL==partName8) {
                dumpPartsAsJSON(c, writer_indent);
            } else {
                opcPart part=OPC_PART_INVALID;
                if ((part=opcPartOpen(c, partName8, NULL, 0))!=OPC_PART_INVALID) {
                    opcXmlReader *reader=NULL;
                    if ((reader=opcXmlReaderOpen(c, part, NULL, NULL, 0))!=NULL) {
                        opcXmlSetMCEProcessing(reader, reader_mce);
                        for(int i=1;i<argc;i++) {
                            if (0==xmlStrcmp(_X("--understands"), _X(argv[i])) || 0==xmlStrcmp(_X("-u"), _X(argv[i])) && i+1<argc) {
                                const xmlChar *ns=_X(argv[++i]);
                                opcXmlUnderstandsNamespace(reader, ns);
                            }
                        }

                        if (-1==mceTextReaderDump(opcXmlReaderGetMceReader(reader), writer, PTRUE)) {
                            ret=mceTextReaderGetError(opcXmlReaderGetMceReader(reader));
                        } else {
                            ret=0;
                        }
                        opcXmlReaderClose(reader);
                    } else {
                        fprintf(stderr, "ERROR: part \"%s\" could not be opened for XML reading.\n", argv[2]);
                    }
                } else {
                    fprintf(stderr, "ERROR: part \"%s\" could not be opened in \"%s\".\n", argv[2], argv[1]);
                }
            }
            opcContainerClose(c, OPC_CLOSE_NOW);
        } else {
            fprintf(stderr, "ERROR: file \"%s\" could not be opened.\n", argv[1]);
        }
        opcFreeLibrary();
    } else {
        fprintf(stderr, "ERROR: initialization of libopc failed.\n");    
    }
    if (NULL!=writer) xmlFreeTextWriter(writer);
    if (NULL!=file) fclose(file);
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return ret;
}
