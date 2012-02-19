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
    Dump some meta information from an Office document using the opc/xmlreader.h APIs.

    Ussage:
    opc_xml2 FILENAME

    Sample:
    opc_xml2 OOXMLI1.docx
*/

#include <opc/opc.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

const char PROP_NS[]="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties";

typedef struct {
    xmlChar *application;
    int words;
    int lines;
    int pages;
} AppDocProps;

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (OPC_ERROR_NONE==opcInitLibrary() && 2==argc) {
        opcContainer *c=NULL;
        if (NULL!=(c=opcContainerOpen(_X(argv[1]), OPC_OPEN_READ_ONLY, NULL, NULL))) {
            AppDocProps props;
            memset(&props, 0, sizeof(props));
            mceTextReader_t reader;
            if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &reader, _X("docProps/app.xml"), NULL, 0, 0)) {
                mce_start_document(&reader) {
                    mce_start_element(&reader, _X(PROP_NS), _X("Properties")) {
                        mce_start_children (&reader) {
                            mce_start_element(&reader, _X(PROP_NS), _X("Application")) {
                                if (props.application!=NULL) {
                                    xmlFree(props.application);
                                    props.application=NULL;
                                }
                                mce_skip_attributes(&reader);
                                mce_start_children(&reader) {
                                    mce_start_text(&reader) {
                                        props.application=xmlStrdup(xmlTextReaderConstValue(reader.reader));
                                    } mce_end_text(&reader);
                                } mce_end_children(&reader);
                            } mce_end_element(&reader);
                            mce_start_element(&reader, _X(PROP_NS), _X("Words")) {
                                props.words=0;
                                mce_skip_attributes(&reader);
                                mce_start_children(&reader) {
                                    mce_start_text(&reader) {
                                        props.words=atoi((char*)xmlTextReaderConstValue(reader.reader));
                                    } mce_end_text(&reader);
                                } mce_end_children(&reader);
                            } mce_end_element(&reader);
                            mce_start_element(&reader, _X(PROP_NS), _X("Lines")) {
                                props.lines=0;
                                mce_skip_attributes(&reader);
                                mce_start_children(&reader) {
                                    mce_start_text(&reader) {
                                        props.lines=atoi((char*)xmlTextReaderConstValue(reader.reader));
                                    } mce_end_text(&reader);
                                } mce_end_children(&reader);
                            } mce_end_element(&reader);
                            mce_start_element(&reader, _X(PROP_NS), _X("Pages")) {
                                props.pages=0;
                                mce_skip_attributes(&reader);
                                mce_start_children(&reader) {
                                    mce_start_text(&reader) {
                                        props.pages=atoi((char*)xmlTextReaderConstValue(reader.reader));
                                    } mce_end_text(&reader);
                                } mce_end_children(&reader);
                            } mce_end_element(&reader);
                        } mce_end_children(&reader);
                    } mce_end_element(&reader);
                } mce_end_document(&reader);
                mceTextReaderCleanup(&reader);
            }            
            opcContainerClose(c, OPC_CLOSE_NOW);
            printf("application: %s\n", props.application);
            printf("words: %i\n", props.words);
            printf("lines: %i\n", props.lines);
            printf("pages: %i\n", props.pages);
            xmlFree(props.application);
        } else {
            printf("ERROR: file \"%s\" could not be opened.\n", argv[1]);
        }
        opcFreeLibrary();
    } else if (2==argc) {
        printf("ERROR: initialization of libopc failed.\n");    
    } else {
        printf("opc_extract FILENAME.\n\n");
        printf("Sample: opc_extract test.docx word/document.xml\n");
    }
#ifdef WIN32
    OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
    return 0;
}

