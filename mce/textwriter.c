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
#include <mce/textwriter.h>

mceTextWriter *mceTextWriterCreateIO(xmlOutputWriteCallback iowrite, xmlOutputCloseCallback  ioclose, void *ioctx, xmlCharEncodingHandlerPtr encoder) {
    mceTextWriter *w=(mceTextWriter*)xmlMalloc(sizeof(mceTextWriter));
    if (NULL!=w) {
        memset(w, 0, sizeof(*w));
        xmlOutputBufferPtr out=xmlOutputBufferCreateIO(iowrite, ioclose, ioctx, encoder);
        w->writer=xmlNewTextWriter(out);
        if (NULL==w->writer) {
            // creation failed
            xmlOutputBufferClose(out);
            xmlFree(w);
            w=NULL;
        }
    }
    return w;
}

int mceTextWriterFree(mceTextWriter *w) {
    int ret=0;
    if (NULL!=w) {
        xmlFreeTextWriter(w->writer);
        if (NULL!=w->registered_array.list_array) xmlFree(w->registered_array.list_array);
        xmlFree(w);
        ret=1;
    }
    return ret;
}

int mceTextWriterStartDocument(mceTextWriter *w) {
    int ret=0;
    ret=xmlTextWriterStartDocument(w->writer, NULL, NULL, NULL);
    PASSERT(0==w->level);
    return ret;
}

int mceTextWriterEndDocument(mceTextWriter *w) {
    int ret=0;
    PASSERT(0==w->level);
    mceQNameLevelCleanup(&w->registered_array, w->level);
    ret=xmlTextWriterEndDocument(w->writer);
    return ret;
}

int mceTextWriterStartElement(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln) {
    int ret=0;
    PASSERT(w->level>=w->registered_array.max_level);
    mceQNameLevel_t* qName=mceQNameLevelLookup(&w->registered_array, ns, NULL, PTRUE);
    if (NULL!=qName) {
        if (NULL==qName->ln) {
            ret=xmlTextWriterStartElement(w->writer, ln);
        } else {
            ret=xmlTextWriterStartElementNS(w->writer, qName->ln, ln, (w->level==w->registered_array.max_level?ns:NULL));
        }
        if (w->level==w->registered_array.max_level) {
            // register new namespaces
            for(puint32_t i=0;i<w->registered_array.list_items;i++) {
                if (w->registered_array.list_array[i].level==w->level && w->registered_array.list_array[i].ns!=ns) {
                    xmlTextWriterWriteAttributeNS(w->writer, 
                                                  _X("xmlns"), 
                                                  w->registered_array.list_array[i].ln, 
                                                  NULL, 
                                                  w->registered_array.list_array[i].ns);
                }
            }
        }


    } else {
        // namespace not registered => not good!
        PASSERT(0==ret);
    }
    w->level++;
    return ret;
}

int mceTextWriterEndElement(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln) {
    int ret=0;
    ret=xmlTextWriterEndElement(w->writer);
    mceQNameLevelCleanup(&w->registered_array, w->level);
    PASSERT(w->level>0);
    w->level--;
    return ret;
}

int mceTextWriterWriteString(mceTextWriter *w, const xmlChar *content) {
    int ret=0;
    ret=xmlTextWriterWriteString(w->writer, content);
    return ret;
}

const xmlChar *mceTextWriterRegisterNamespace(mceTextWriter *w, const xmlChar *ns, const xmlChar *prefix, int flags) {
    mceQNameLevelAdd(&w->registered_array, ns, prefix, w->level);
    mceQNameLevel_t *ret=mceQNameLevelLookup(&w->registered_array, ns, prefix, PFALSE);
    PASSERT(NULL!=ret); // not inserted? why?
    return (NULL!=ret?ret->ns:NULL);
}

int mceTextWriterProcessContent(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln) {
    return 0;
}

int mceTextWriterAttributeF(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln, const char *value, ...) {
    return 0;
}

int mceTextWriterStartAlternateContent(mceTextWriter *w) {
    return 0;
}

int mceTextWriterEndAlternateContent(mceTextWriter *w) {
    return 0;
}

int mceTextWriterStartChoice(mceTextWriter *w, const xmlChar *ns) {
    return 0;
}

int mceTextWriterEndChoice(mceTextWriter *w) {
    return 0;
}

int mceTextWriterStartFallback(mceTextWriter *w) {
    return 0;
}

int mceTextWriterEndFallback(mceTextWriter *w) {
    return 0;
}
