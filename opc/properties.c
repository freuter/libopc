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
#include <opc/properties.h>
#include <libxml/xmlmemory.h>
#include <opc/xmlreader.h>
#include <mce/textreader.h>
#include <mce/textwriter.h>

opc_error_t opcCorePropertiesInit(opcProperties_t *cp) {
    opc_bzero_mem(cp, sizeof(*cp));
    return OPC_ERROR_NONE;
}

static void opcDCSimpleTypeFree(opcDCSimpleType_t *v) {
    if (NULL!=v) {
        if (NULL!=v->lang) xmlFree(v->lang);
        if (NULL!=v->str) xmlFree(v->str);
    }
}

opc_error_t opcCorePropertiesCleanup(opcProperties_t *cp) {
    if (NULL!=cp->category) xmlFree(cp->category);
    if (NULL!=cp->contentStatus) xmlFree(cp->contentStatus);
    if (NULL!=cp->created) xmlFree(cp->created);
    opcDCSimpleTypeFree(&cp->creator);
    opcDCSimpleTypeFree(&cp->description);
    opcDCSimpleTypeFree(&cp->identifier);
    for(opc_uint32_t i=0;i<cp->keyword_items;i++) {
        opcDCSimpleTypeFree(&cp->keyword_array[i]);
    }
    if (NULL!=cp->keyword_array) {
        xmlFree(cp->keyword_array);
    }
    opcDCSimpleTypeFree(&cp->language);
    if (NULL!=cp->lastModifiedBy) xmlFree(cp->lastModifiedBy);
    if (NULL!=cp->lastPrinted) xmlFree(cp->lastPrinted);
    if (NULL!=cp->modified) xmlFree(cp->modified);
    if (NULL!=cp->revision) xmlFree(cp->revision);
    opcDCSimpleTypeFree(&cp->subject);
    opcDCSimpleTypeFree(&cp->title);
    if (NULL!=cp->version) xmlFree(cp->version);
    return OPC_ERROR_NONE;
}

opc_error_t opcCorePropertiesSetString(xmlChar **prop, const xmlChar *str) {
    if (NULL!=prop) {
        if (NULL!=*prop) xmlFree(*prop);
        *prop=xmlStrdup(str);
    }
    return OPC_ERROR_NONE;
}

opc_error_t opcCorePropertiesSetStringLang(opcDCSimpleType_t *prop, const xmlChar *str, const xmlChar *lang) {
    if (NULL!=prop) {
        if (NULL!=prop->str) xmlFree(prop->str);
        if (NULL!=prop->lang) xmlFree(prop->lang);
        prop->str=(NULL!=str?xmlStrdup(str):NULL);
        prop->lang=(NULL!=lang?xmlStrdup(lang):NULL);
    }
    return OPC_ERROR_NONE;
}

static const char cp_ns[]="http://schemas.openxmlformats.org/package/2006/metadata/core-properties";
static const char dc_ns[]="http://purl.org/dc/elements/1.1/";
static const char dcterms_ns[]="http://purl.org/dc/terms/";
static const char xml_ns[]="http://www.w3.org/XML/1998/namespace";
static const char xsi_ns[]="http://www.w3.org/2001/XMLSchema-instance";

static void _opcCorePropertiesAddKeywords(opcProperties_t *cp, const xmlChar *keywords, const xmlChar *lang) {
    int ofs=0;
    do {
        while(0!=keywords[ofs] && (' '==keywords[ofs] || '\t'==keywords[ofs] || '\r'==keywords[ofs] || '\n'==keywords[ofs])) ofs++;
        int start=ofs;
        while(0!=keywords[ofs] && ';'!=keywords[ofs]) ofs++;
        int end=ofs;
        while(end>start && (' '==keywords[end-1] || '\t'==keywords[end-1] || '\r'==keywords[end-1] || '\n'==keywords[end-1])) end--;
        if (end>start) {
            cp->keyword_array=(opcDCSimpleType_t*)xmlRealloc(cp->keyword_array, (cp->keyword_items+1)*sizeof(opcDCSimpleType_t));
            cp->keyword_array[cp->keyword_items].lang=(NULL!=lang?xmlStrdup(lang):NULL);
            cp->keyword_array[cp->keyword_items].str=xmlStrndup(keywords+start, end-start);
            cp->keyword_items++;
        }
        ofs++;
    } while(keywords[ofs-1]!=0);
}

static void _opcCorePropertiesReadString(mceTextReader_t *r, xmlChar **v) {
    mce_skip_attributes(r);
    mce_start_children(r) {
        mce_start_text(r) {
            if (NULL!=v && NULL!=*v) xmlFree(*v);
            *v=xmlStrdup(xmlTextReaderConstValue(r->reader));
        } mce_end_text(r);
    } mce_end_children(r);
}

static void _opcCorePropertiesReadSimpleType(mceTextReader_t *r, opcDCSimpleType_t *v) {
    opc_bzero_mem(v, sizeof(*v));
    mce_start_attributes(r) {
        mce_start_attribute(r, _X(xml_ns), _X("lang")) {
            if (NULL==v->lang) {
                v->lang=xmlStrdup(xmlTextReaderConstValue(r->reader));
            }
        } mce_end_attribute(r);
    } mce_end_attributes(r);
    mce_start_children(r) {
        mce_start_text(r) {
            if (NULL==v->str) {
                v->str=xmlStrdup(xmlTextReaderConstValue(r->reader));
            }
        } mce_end_text(r);
    } mce_end_children(r);
}

opc_error_t opcCorePropertiesRead(opcProperties_t *cp, opcContainer *c) {
    mceTextReader_t r;
    if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &r, _X("docProps/core.xml"), NULL, NULL, 0)) {
            mce_start_document(&r) {
                mce_start_element(&r, _X(cp_ns), _X("coreProperties")) {
                    mce_skip_attributes(&r);
                    mce_start_children(&r) {
                        mce_start_element(&r, _X(cp_ns), _X("category")) {
                            _opcCorePropertiesReadString(&r, &cp->category);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("contentStatus")) {
                            _opcCorePropertiesReadString(&r, &cp->contentStatus);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dcterms_ns), _X("created")) {
                            _opcCorePropertiesReadString(&r, &cp->created);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("creator")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->creator);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("description")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->description);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("identifier")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->identifier);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("keywords")) {
                            xmlChar *xml_lang=NULL;
                            mce_start_attributes(&r) {
                                mce_start_attribute(&r, _X(xml_ns), _X("lang")) {
                                    xml_lang=xmlStrdup(xmlTextReaderConstValue(r.reader));
                                } mce_end_attribute(&r);
                            } mce_end_attributes(&r);
                            mce_start_children(&r) {
                                mce_start_element(&r, _X(cp_ns), _X("value")) {
                                    xmlChar *value_xml_lang=NULL;
                                    mce_start_attributes(&r) {
                                        mce_start_attribute(&r, _X(xml_ns), _X("lang")) {
                                            value_xml_lang=xmlStrdup(xmlTextReaderConstValue(r.reader));
                                        } mce_end_attribute(&r);
                                    } mce_end_attributes(&r);
                                    mce_start_children(&r) {
                                        mce_start_text(&r) {
                                            _opcCorePropertiesAddKeywords(cp, xmlTextReaderConstValue(r.reader), value_xml_lang);
                                        } mce_end_text(&r);
                                    } mce_end_children(&r);
                                if (NULL!=value_xml_lang) { xmlFree(value_xml_lang); value_xml_lang=NULL; };
                                } mce_end_element(&r);
                                mce_start_text(&r) {
                                    _opcCorePropertiesAddKeywords(cp, xmlTextReaderConstValue(r.reader), xml_lang);
                                } mce_end_text(&r);
                            } mce_end_children(&r);
                            if (NULL!=xml_lang) { xmlFree(xml_lang); xml_lang=NULL; };
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("language")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->language);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("lastModifiedBy")) {
                            _opcCorePropertiesReadString(&r, &cp->lastModifiedBy);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("lastPrinted")) {
                            _opcCorePropertiesReadString(&r, &cp->lastPrinted);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dcterms_ns), _X("modified")) {
                            _opcCorePropertiesReadString(&r, &cp->modified);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("revision")) {
                            _opcCorePropertiesReadString(&r, &cp->revision);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("subject")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->subject);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(dc_ns), _X("title")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->title);
                        } mce_end_element(&r);
                        mce_start_element(&r, _X(cp_ns), _X("version")) {
                            _opcCorePropertiesReadString(&r, &cp->version);
                        } mce_end_element(&r);
                    } mce_end_children(&r);
                } mce_end_element(&r);
            } mce_end_document(&r);
        mceTextReaderCleanup(&r);
    }
    return OPC_ERROR_NONE;
}

static void _opcCorePropertiesWriteString(mceTextWriter *w, const xmlChar *ns, const xmlChar *name, const xmlChar *value, const xmlChar *type) {
    if (NULL!=value) {
        mceTextWriterStartElement(w, ns, name);
        if (NULL!=type) {
            mceTextWriterAttributeF(w, _X(xsi_ns), _X("type"), (const char *)type);
        }
        mceTextWriterWriteString(w, value);
        mceTextWriterEndElement(w, ns, name);
    }
}

static void _opcCorePropertiesWriteSimpleType(mceTextWriter *w, const xmlChar *ns, const xmlChar *name, const opcDCSimpleType_t*value) {
    if (NULL!=value->str) {
        mceTextWriterStartElement(w, ns, name);
        if (NULL!=value->lang) {
            mceTextWriterAttributeF(w, _X(xml_ns), _X("lang"), (const char *)value->lang);
        }
        mceTextWriterWriteString(w, value->str);
        mceTextWriterEndElement(w, ns, name);
    }
}

opc_error_t opcCorePropertiesWrite(opcProperties_t *cp, opcContainer *c) {
    opcPart part=opcPartCreate(c, _X("docProps/core.xml"), _X("application/vnd.openxmlformats-package.core-properties+xml"), 0);
    if (NULL!=part) {
        mceTextWriter *w=mceTextWriterOpen(c, part, OPC_COMPRESSIONOPTION_SUPERFAST);
        if (NULL!=w) {
            mceTextWriterStartDocument(w);
            mceTextWriterRegisterNamespace(w, _X(cp_ns), _X("cp"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, _X(dc_ns), _X("dc"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, _X(dcterms_ns), _X("dcterms"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, _X(xsi_ns), _X("xsi"), MCE_DEFAULT);
            mceTextWriterStartElement(w, _X(cp_ns), _X("coreProperties"));
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("category"), cp->category, NULL);
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("contentStatus"), cp->contentStatus, NULL);
            _opcCorePropertiesWriteString(w, _X(dcterms_ns), _X("created"), cp->created, _X("dcterms:W3CDTF"));
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("creator"), &cp->creator);
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("description"), &cp->description);
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("identifier"), &cp->identifier);
            if (cp->keyword_items>0) {
                mceTextWriterStartElement(w, _X(cp_ns), _X("keywords"));
                for(opc_uint32_t i=0;i<cp->keyword_items;i++) {
                    _opcCorePropertiesWriteSimpleType(w, _X(cp_ns), _X("value"), &cp->keyword_array[i]);
                    if (i+1<cp->keyword_items) {
                        mceTextWriterWriteString(w, _X(";"));
                    }
                }
                mceTextWriterEndElement(w, _X(cp_ns), _X("keywords"));
            }
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("language"), &cp->language);
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("lastModifiedBy"), cp->lastModifiedBy, NULL);
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("lastPrinted"), cp->lastPrinted, _X("dcterms:W3CDTF"));
            _opcCorePropertiesWriteString(w, _X(dcterms_ns), _X("modified"), cp->modified, _X("dcterms:W3CDTF"));
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("revision"), cp->revision, NULL);
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("subject"), &cp->subject);
            _opcCorePropertiesWriteSimpleType(w, _X(dc_ns), _X("title"), &cp->title);
            _opcCorePropertiesWriteString(w, _X(cp_ns), _X("version"), cp->version, NULL);
            mceTextWriterEndElement(w, _X(cp_ns), _X("coreProperties"));
            mceTextWriterEndDocument(w);
            mceTextWriterFree(w);
        }
    }
    return OPC_ERROR_NONE;
}
