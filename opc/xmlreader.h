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
/** @file opc/xmlreader.h
 
 */
#include <opc/config.h>
#include <libxml/xmlreader.h>

#ifndef OPC_XMLREADER_H
#define OPC_XMLREADER_H

#ifdef __cplusplus
extern "C" {
#endif    

    typedef struct OPC_CONTAINER_INPUTSTREAM_STRUCT opcXmlReader;

    opcXmlReader *opcXmlReaderOpen(opcContainer *c, opcPart part);
    opc_error_t opcXmlReaderClose(opcXmlReader *reader);

    const xmlChar *opcXmlReaderLocalName(opcXmlReader *reader);
    const xmlChar *opcXmlReaderConstNamespaceUri(opcXmlReader *reader);
    const xmlChar *opcXmlReaderConstPrefix(opcXmlReader *reader);
    const xmlChar *opcXmlReaderConstValue(opcXmlReader *reader);

    void opcXmlReaderStartDocument(opcXmlReader *reader);
    void opcXmlReaderEndDocument(opcXmlReader *reader);

    opc_bool_t opcXmlReaderStartElement(opcXmlReader *reader, xmlChar *ns, xmlChar *ln);
    opc_bool_t opcXmlReaderStartAttribute(opcXmlReader *reader, xmlChar *ns, xmlChar *ln);
    opc_bool_t opcXmlReaderSkipElement(opcXmlReader *reader, xmlChar *ns, xmlChar *ln);
    opc_bool_t opcXmlReaderStartText(opcXmlReader *reader);

    opc_bool_t opcXmlReaderStartAttributes(opcXmlReader *reader);
    opc_bool_t opcXmlReaderEndAttributes(opcXmlReader *reader);

    opc_bool_t opcXmlReaderStartChildren(opcXmlReader *reader);
    opc_bool_t opcXmlReaderEndChildren(opcXmlReader *reader);


#define opc_xml_start_document(reader) opcXmlReaderStartDocument(reader);
#define opc_xml_end_document(reader) opcXmlReaderEndDocument(reader)
#define opc_xml_element(reader, ns, ln) if (opcXmlReaderStartElement(reader, ns, ln)) 
#define opc_xml_start_children(reader) if (opcXmlReaderStartChildren(reader)) { do 
#define opc_xml_end_children(reader) while(!opcXmlReaderEndChildren(reader)); }
#define opc_xml_start_attributes(reader) if (opcXmlReaderStartAttributes(reader)) { do 
#define opc_xml_end_attributes(reader) while(!opcXmlReaderEndAttributes(reader)); }
#define opc_xml_attribute(reader, ns, ln) if (opcXmlReaderStartAttribute(reader, ns, ln))
#define opc_xml_text(reader) if (opcXmlReaderStartText(reader))
#define opc_xml_const_value(reader) opcXmlReaderConstValue(reader)

#define opc_xml_error_guard_start(reader) if (OPC_ERROR_NONE==reader->error) do 
#define opc_xml_error_guard_end(reader)  while(0)
#if defined(__GNUC__)
#define opc_xml_error(reader, guard, err, msg, ...) if (guard) { reader->error=(err); fprintf(stderr, msg, ##__VA_ARGS__ );  continue; }
#else
#define opc_xml_error(reader, guard, err, msg, ...) if (guard) { reader->error=(err); fprintf(stderr, msg, __VA_ARGS__ );  continue; }
#endif
#define opc_xml_error_strict opc_xml_error
#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_XMLREADER_H */
