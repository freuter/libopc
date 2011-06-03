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

#include <stdio.h>
#include <time.h>
#include <mce/textreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>


static int  xmlOutputWrite(void * context, const char * buffer, int len) {
    FILE *out=(FILE*)context;
    return fwrite(buffer, sizeof(char), len, out);
}

static int xmlOutputClose(void * context) {
    return 0;
}


int main( int argc, const char* argv[] )
{
    int ret=-1;
    time_t start_time=time(NULL);
    FILE *file=NULL;
    xmlTextWriter *writer=NULL;
    int writer_indent=0;
    pbool_t reader_mce=PTRUE;
    const char *fileName=NULL;
    for(int i=1;i<argc;i++) {
        if ((0==xmlStrcmp(_X("--understands"), _X(argv[i])) || 0==xmlStrcmp(_X("-u"), _X(argv[i]))) && i+1<argc) {
            const xmlChar *ns=_X(argv[++i]);
        } else if ((0==xmlStrcmp(_X("--out"), _X(argv[i])) || 0==xmlStrcmp(_X("--out"), _X(argv[i]))) && i+1<argc && NULL==file) {
            const char *filename=argv[++i];
            file=fopen(filename, "w");
        } else if (0==xmlStrcmp(_X("--indent"), _X(argv[i]))) {
            writer_indent=1;
        } else if (0==xmlStrcmp(_X("--raw"), _X(argv[i]))) {
            reader_mce=PFALSE;
        } else if (NULL==fileName) {
            fileName=argv[i];
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
    if (NULL==fileName || NULL==writer) {
        printf("mcepp [--understands NAMESPACE] [--out FILENAME] [--indent] [--raw] [FILENAME | - ]\n\n");
        printf("Sample: mcepp sample.xml\n");
    } else {
        xmlInitParser();
        xmlTextWriterSetIndent(writer, writer_indent);
        mceTextReader_t mceTextReader;
        mceTextReaderInit(&mceTextReader, ('-'==fileName[0] && 0==fileName[1]?xmlReaderForFd(0, NULL, NULL, 0):xmlReaderForFile(fileName, NULL, 0)));
        for(int i=1;i<argc;i++) {
            if (0==xmlStrcmp(_X("--understands"), _X(argv[i])) || 0==xmlStrcmp(_X("-u"), _X(argv[i])) && i+1<argc) {
                const xmlChar *ns=_X(argv[++i]);
                mceTextReaderUnderstandsNamespace(&mceTextReader, ns);
            }
        }

        if (-1==mceTextReaderDump(&mceTextReader, writer, PFALSE)) {
            ret=mceTextReaderGetError(&mceTextReader);
        } else {
            ret=0;
        }
        mceTextReaderCleanup(&mceTextReader);
        xmlCleanupParser();
    }
    if (NULL!=writer) xmlFreeTextWriter(writer);
    if (NULL!=file) fclose(file);
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return ret;
}
