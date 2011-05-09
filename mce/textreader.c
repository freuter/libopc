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
#include <mce/textreader.h>

#if 0

mceTextReader *mceTextReaderOpen(opcPart *part) {
	return NULL;
}

int mceTextReaderRead(mceTextReader *reader) {
	return 0;
}

int mceTextReaderClose(mceTextReader *reader) {
	return 0;
}

int mceTextReaderNodeType(mceTextReader *reader) {
	return 0;
}

int mceTextReaderIsEmptyElement(mceTextReader *reader) {
	return 0;
}

const xmlChar *mceTextReaderLocalName(mceTextReader *reader) {
	return NULL;
}

int mceTextReaderUnderstands(const xmlChar *ns) {
	return 0;
}

#endif

int mceTextReaderInit(mceTextReader_t *mceTextReader, xmlTextReaderPtr reader) {
    memset(mceTextReader, 0, sizeof(*mceTextReader));
    mceCtxInit(&mceTextReader->mceCtx);
    mceTextReader->reader=reader;
    return 0;
}

int mceTextReaderCleanup(mceTextReader_t *mceTextReader) {
    mceCtxCleanup(&mceTextReader->mceCtx);
    xmlTextReaderClose(mceTextReader->reader);
    xmlFreeTextReader(mceTextReader->reader);
    return 0;
}

static xmlChar *xmlStrDupArray(const xmlChar *value) {
    opc_uint32_t len=xmlStrlen(value);
    xmlChar *ret=(xmlChar *)xmlMalloc((2+len)*sizeof(xmlChar));
    opc_uint32_t j=0;
    for(opc_uint32_t i=0;i<len;i++) {
        while(i<len && (value[i]==' ' || value[i]=='\t' || value[i]=='\r' || value[i]=='\n')) i++; // skip preceeding spaces
        while(i<len && value[i]!=' ' && value[i]!='\t' && value[i]!='\r' && value[i]!='\n') ret[j++]=value[i++];
        ret[j++]=0;
    }
    ret[j]=0;
    return ret;
}

static char ns_mce[]="http://schemas.openxmlformats.org/markup-compatibility/2006";
int mceTextReaderPostprocess(xmlTextReader *reader, mceCtx_t *ctx, int ret) {
    pbool_t done=PFALSE;
    while (1==ret && !done) {
        done=OPC_TRUE;
        if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader)) {
            if (1==xmlTextReaderHasAttributes(reader)) {
                if (1==xmlTextReaderMoveToFirstAttribute(reader)) {
                    do {
                        if (0==xmlStrcmp(_X("Ignorable"), xmlTextReaderConstLocalName(reader)) &&
                            0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                                xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader));
                                for(xmlChar *prefix=v;*prefix!=0;prefix+=1+xmlStrlen(prefix)) {
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader, prefix);
                                    if (NULL!=ns_ && NULL==mceQNameLevelLookup(&ctx->understands_array, ns_, NULL)) {
                                        PASSERT(xmlTextReaderDepth(reader)>0); // remove one, since we need the element and not the attribute level
                                        PENSURE(mceQNameLevelAdd(&ctx->ignored_array, ns_, NULL, xmlTextReaderDepth(reader)-1));
                                    }
                                }
                                xmlFree(v);
                        } else if (0==xmlStrcmp(_X("ProcessContent"), xmlTextReaderConstLocalName(reader)) &&
                                   0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                                xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader));
                                xmlChar *qname=v;
                                for(int qname_len=0;(qname_len=xmlStrlen(qname))>0;qname+=qname_len+1) {
                                    int prefix=0; while(qname[prefix]!=':' && qname[prefix]!=0) prefix++;
                                    OPC_ASSERT(prefix<=qname_len);
                                    int ln=(prefix<qname_len?prefix+1:0);
                                    if (prefix<qname_len) {
                                        qname[prefix]=0;
                                        prefix=0;
                                    };
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader, qname+prefix);
                                    if (NULL!=ns_ && NULL==mceQNameLevelLookup(&ctx->understands_array, ns_, NULL)) {
                                        PASSERT(xmlTextReaderDepth(reader)>0); // remove one, since we need the element and not the attribute level
                                        PENSURE(mceQNameLevelAdd(&ctx->processcontent_array, ns_, xmlStrdup(qname+ln), xmlTextReaderDepth(reader)-1));

                                    }
                                }
                                xmlFree(v);
                        }


                    } while (1==xmlTextReaderMoveToNextAttribute(reader));
                }
                OPC_ENSURE(1==xmlTextReaderMoveToElement(reader));
            }
            if (1==xmlTextReaderHasAttributes(reader)) {
                xmlAttrPtr remove=NULL;
                if (1==xmlTextReaderMoveToFirstAttribute(reader)) {
                    do {
                        if (NULL!=remove) {
                            xmlRemoveProp(remove); remove=NULL;
                        }
                        if (0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                            OPC_ASSERT(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader)->type);
                            remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader);
                        } else if (NULL!=mceQNameLevelLookup(&ctx->ignored_array,
                                                             xmlTextReaderConstNamespaceUri(reader), 
                                                             NULL)) {
                            OPC_ASSERT(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader)->type);
                            remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader);
                        }
                    } while (1==xmlTextReaderMoveToNextAttribute(reader));
                }
                OPC_ENSURE(1==xmlTextReaderMoveToElement(reader));
                if (NULL!=remove) {
                    xmlRemoveProp(remove); remove=NULL;
                }
                OPC_ASSERT(NULL==remove);
            }
            opc_bool_t ignore=OPC_FALSE;
            opc_bool_t process_content=OPC_FALSE;
            if (0==xmlStrcmp(_X("AlternateContent"), xmlTextReaderConstLocalName(reader)) &&
                0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                    ignore=OPC_TRUE;
                    process_content=OPC_TRUE;
            } else if (0==xmlStrcmp(_X("Choice"), xmlTextReaderConstLocalName(reader)) &&
                       0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                    xmlChar *req=NULL;
                    if (1==xmlTextReaderMoveToAttribute(reader, _X("Requires"))) {
                        req=xmlStrDupArray(xmlTextReaderConstValue(reader));
                        OPC_ASSERT(1==xmlTextReaderMoveToElement(reader));
                    } else if (1==xmlTextReaderMoveToAttributeNs(reader, _X("Requires"), _X(ns_mce))) {
                        req=xmlStrDupArray(xmlTextReaderConstValue(reader));
                        OPC_ASSERT(1==xmlTextReaderMoveToElement(reader));
                    }
                    if (NULL!=req) {
                        ignore=OPC_TRUE;
                        const xmlChar *top_ln=(ctx->skip_array.list_items>0?ctx->skip_array.list_array[ctx->skip_array.list_items-1].ln:NULL);
                        const xmlChar *top_ns=(ctx->skip_array.list_items>0?ctx->skip_array.list_array[ctx->skip_array.list_items-1].ns:NULL);
                        OPC_ASSERT(NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns));
                        if (NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns)) {
                            if (0==ctx->skip_array.list_array[ctx->skip_array.list_items-1].alternatecontent_handled) {
                                process_content=OPC_TRUE;
                                for(xmlChar *prefix=req;*prefix!=0;prefix+=1+xmlStrlen(prefix)) {
                                    xmlChar *ns_=xmlTextReaderLookupNamespace(reader, prefix);
                                    OPC_ASSERT(NULL!=ns_);
                                    process_content&=(NULL!=ns_ && NULL!=mceQNameLevelLookup(&ctx->understands_array, ns_, NULL));
                                }
                                ctx->skip_array.list_array[ctx->skip_array.list_items-1].alternatecontent_handled=(process_content?1:0);
                            } else {
                                OPC_ASSERT(OPC_FALSE==process_content);
                            }
                        }
                        OPC_ASSERT(NULL!=req); xmlFree(req);
                    }
            } else if (0==xmlStrcmp(_X("Fallback"), xmlTextReaderConstLocalName(reader)) &&
                       0==xmlStrcmp(_X(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                    ignore=OPC_TRUE;
                        const xmlChar *top_ln=(ctx->skip_array.list_items>0?ctx->skip_array.list_array[ctx->skip_array.list_items-1].ln:NULL);
                        const xmlChar *top_ns=(ctx->skip_array.list_items>0?ctx->skip_array.list_array[ctx->skip_array.list_items-1].ns:NULL);
                    OPC_ASSERT(NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns));
                    if (NULL!=top_ln && NULL!=top_ns && 0==xmlStrcmp(_X("AlternateContent"), top_ln) && 0==xmlStrcmp(_X(ns_mce), top_ns)) {
                        process_content=(1!=ctx->skip_array.list_array[ctx->skip_array.list_items-1].alternatecontent_handled);
                    }
            } else {
                ignore=NULL!=mceQNameLevelLookup(&ctx->ignored_array, xmlTextReaderConstNamespaceUri(reader), NULL);
            }
            if (1==xmlTextReaderIsEmptyElement(reader)) {
                PENSURE(mceQNameLevelCleanup(&ctx->ignored_array, xmlTextReaderDepth(reader)));
                PENSURE(mceQNameLevelCleanup(&ctx->processcontent_array, xmlTextReaderDepth(reader)));
                if (ignore) {
                    PENSURE(-1!=(ret=xmlTextReaderRead(reader))); // consume empty element
                    done=OPC_FALSE;
                }
            } else {
                if (ignore) {
                    if (!process_content) {
                        process_content=NULL!=mceQNameLevelLookup(&ctx->processcontent_array, 
                                                                  xmlTextReaderConstNamespaceUri(reader), 
                                                                  xmlTextReaderLocalName(reader));
                    }
                    if (process_content) {
                        PENSURE(mceQNameLevelPush(&ctx->skip_array, 
                                                  xmlTextReaderConstNamespaceUri(reader), 
                                                  xmlTextReaderConstLocalName(reader), 
                                                  xmlTextReaderDepth(reader)));
                        PENSURE(-1!=(ret=xmlTextReaderRead(reader))); // consume start element
                        done=PFALSE;
                    } else {
                        PENSURE(mceQNameLevelCleanup(&ctx->ignored_array, xmlTextReaderDepth(reader)));
                        PENSURE(mceQNameLevelCleanup(&ctx->processcontent_array, xmlTextReaderDepth(reader)));
                        OPC_ENSURE(-1!=(ret=xmlTextReaderNext(reader))); // consume whole element
                        done=PFALSE;
                    }
                }
            }
        } else if (XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(reader)) {
            PENSURE(mceQNameLevelCleanup(&ctx->ignored_array, xmlTextReaderDepth(reader)));
            PENSURE(mceQNameLevelCleanup(&ctx->processcontent_array, xmlTextReaderDepth(reader)));
            if (mceQNameLevelPopIfMatch(&ctx->skip_array,
                                        xmlTextReaderConstNamespaceUri(reader), 
                                        xmlTextReaderConstLocalName(reader),
                                        xmlTextReaderDepth(reader))) {
                PENSURE(-1!=(ret=xmlTextReaderRead(reader))); // consume end element
                done=OPC_FALSE;
            }
        }
    }
    if (0) {
        printf("mce: %i[%i] %s %s", xmlTextReaderNodeType(reader), xmlTextReaderIsEmptyElement(reader), xmlTextReaderConstNamespaceUri(reader), xmlTextReaderConstLocalName(reader));
        printf("<");
        for(opc_uint32_t i=0;i<ctx->skip_array.list_items;i++) {
            printf("(%i;%s;%s)", ctx->skip_array.list_array[i].level, ctx->skip_array.list_array[i].ln, ctx->skip_array.list_array[i].ns);
            if (i+1<ctx->skip_array.list_items) printf(" ");
        }
        printf(">");
        printf("\n");
    }
    return ret;
}


int mceTextReaderRead(mceTextReader_t *mceTextReader) {
    return mceTextReaderPostprocess(mceTextReader->reader, &mceTextReader->mceCtx, xmlTextReaderRead(mceTextReader->reader));
}

int mceTextReaderNext(mceTextReader_t *mceTextReader) {
    return mceTextReaderPostprocess(mceTextReader->reader, &mceTextReader->mceCtx, xmlTextReaderNext(mceTextReader->reader));
}


int mceTextReaderDump(mceTextReader_t *mceTextReader, xmlTextWriter *writer) {
    int ret=-1;
    if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(mceTextReader->reader)) {
        const xmlChar *ns=xmlTextReaderConstNamespaceUri(mceTextReader->reader);
        const xmlChar *ln=xmlTextReaderConstLocalName(mceTextReader->reader);
        if (NULL!=ns) {
            xmlTextWriterStartElementNS(writer, xmlTextReaderConstPrefix(mceTextReader->reader), ln, NULL);
        } else {
            xmlTextWriterStartElement(writer, ln);
        }
        if (xmlTextReaderHasAttributes(mceTextReader->reader)) {
            if (1==xmlTextReaderMoveToFirstAttribute(mceTextReader->reader)) {
                do {
                    static const char ns_xml[]="http://www.w3.org/2000/xmlns/";
                    static const char _xmlns[]="xmlns";
                    const xmlChar *attr_ns=xmlTextReaderConstNamespaceUri(mceTextReader->reader);
                    const xmlChar *attr_ln=xmlTextReaderConstLocalName(mceTextReader->reader);
                    const xmlChar *attr_val=xmlTextReaderConstValue(mceTextReader->reader);
                    if (NULL!=attr_ns && 0==xmlStrcmp(attr_ns, _X(ns_xml))) {
                        if (0==xmlStrcmp(attr_ln, _X(_xmlns))) {
                            xmlTextWriterWriteAttribute(writer, attr_ln, attr_val);
                        } else {
                            xmlTextWriterWriteAttributeNS(writer, _X(_xmlns), attr_ln, NULL, attr_val);
                        }
                    } else {
                        if (NULL!=ns) {
                            xmlTextWriterWriteAttributeNS(writer, xmlTextReaderConstPrefix(mceTextReader->reader), attr_ln, NULL, attr_val);
                        } else {
                            xmlTextWriterWriteAttribute(writer, attr_ln, attr_val);
                        }
                    }
                } while (1==xmlTextReaderMoveToNextAttribute(mceTextReader->reader));
            }
            PENSURE(1==xmlTextReaderMoveToElement(mceTextReader->reader));
        }
        if (!xmlTextReaderIsEmptyElement(mceTextReader->reader)) {
            PENSURE(1==(ret=mceTextReaderRead(mceTextReader))); // read start element
            while (1==ret && XML_READER_TYPE_END_ELEMENT!=xmlTextReaderNodeType(mceTextReader->reader)) {
                ret=mceTextReaderDump(mceTextReader, writer);
            }
            PASSERT(XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(mceTextReader->reader));
            PENSURE(-1!=(ret=mceTextReaderRead(mceTextReader))); // read end element
        } else {
            PENSURE(-1!=(ret=mceTextReaderRead(mceTextReader))); // read empty element
        }
        xmlTextWriterEndElement(writer);
    } else if (XML_READER_TYPE_TEXT==xmlTextReaderNodeType(mceTextReader->reader)) {
        xmlTextWriterWriteString(writer, xmlTextReaderConstValue(mceTextReader->reader));
        PENSURE(1==(ret=mceTextReaderRead(mceTextReader))); // read end element
    } else if (XML_READER_TYPE_SIGNIFICANT_WHITESPACE==xmlTextReaderNodeType(mceTextReader->reader)) {
        xmlTextWriterWriteString(writer, xmlTextReaderConstValue(mceTextReader->reader));
        PENSURE(1==(ret=mceTextReaderRead(mceTextReader))); // read end element
    } else if (XML_READER_TYPE_NONE==xmlTextReaderNodeType(mceTextReader->reader)) {
        ret=mceTextReaderRead(mceTextReader);
        if (1==ret && XML_READER_TYPE_NONE!=xmlTextReaderNodeType(mceTextReader->reader)) {
            xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
            ret=mceTextReaderDump(mceTextReader, writer);
            xmlTextWriterEndDocument(writer);
        }
    } else {
        PENSURE(-1!=(ret=mceTextReaderNext(mceTextReader))); // skip element
    }
    return ret;
}

int mceTextReaderUnderstandsNamespace(mceTextReader_t *mceTextReader, const xmlChar *ns) {
    return (mceCtxUnderstandsNamespace(&mceTextReader->mceCtx, ns)?0:-1);
}
