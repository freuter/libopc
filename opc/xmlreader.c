/*
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
#include "internal.h"

opc_error_t opcXmlReaderOpenEx(opcContainer *container, mceTextReader_t *mceTextReader, const xmlChar *partName, opc_bool_t rels_segment, const char * URL, const char * encoding, int options) {
    opcContainerInputStream* stream=opcContainerOpenInputStreamEx(container, partName, rels_segment);
    if (NULL!=stream) {
        if (0==mceTextReaderInit(mceTextReader, 
                                 xmlReaderForIO((xmlInputReadCallback)opcContainerReadInputStream, 
                                                (xmlInputCloseCallback)opcContainerCloseInputStream, 
                                                stream, URL, encoding, options))) {
            return OPC_ERROR_NONE;
        } else {
            return OPC_ERROR_STREAM;
        }
    } else {
        return OPC_ERROR_STREAM;
    }
}

opc_error_t opcXmlReaderOpen(opcContainer *container, mceTextReader_t *mceTextReader, const xmlChar *partName, const char * URL, const char * encoding, int options) {
    return opcXmlReaderOpenEx(container, mceTextReader, (partName!=NULL && partName[0]=='/'?partName+1:partName), OPC_FALSE, URL, encoding, options);
}

xmlDocPtr opcXmlReaderReadDoc(opcContainer *container, const xmlChar *partName, const char * URL, const char * encoding, int options) {
    opcContainerInputStream* stream=opcContainerOpenInputStreamEx(container, partName, OPC_FALSE);
    if (NULL!=stream) {
        xmlDocPtr doc=xmlReadIO((xmlInputReadCallback)opcContainerReadInputStream, 
                                (xmlInputCloseCallback)opcContainerCloseInputStream, 
                                stream, URL, encoding, options);
        return doc;
    } else {
        return NULL;
    }
}
