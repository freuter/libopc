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

	typedef struct OPC_XMLREADER_STRUCT opcXmlReader;

	opcXmlReader *opcXmlReaderOpen(opcPart *part);
	int opcXmlReaderRead(opcXmlReader *reader);
	int opcXmlReaderClose(opcXmlReader *reader);
	int opcXmlReaderNodeType(opcXmlReader *reader);
	int opcXmlReaderIsEmptyElement(opcXmlReader *reader);
	const xmlChar *opcXmlReaderLocalName(opcXmlReader *reader);	
	
#define opc_xml_start_document(c)
#define opc_xml_end_document(c)
#define opc_xml_start_element(c, ns, ln)
#define opc_xml_end_element(c)
#define opc_xml_start_children(c)
#define opc_xml_end_children(c)
#define opc_xml_start_attributes(c)
#define opc_xml_end_attributes(c)
#define opc_xml_attribute(c, ns, ln)
#define opc_xml_start_text(c)
#define opc_xml_end_text(c)
#define opc_xml_text(c)
	xmlChar *opc_xml_const_value(opcXmlReader *reader);

	
#ifdef __cplusplus
} /* extern "C" */
#endif    
		
#endif /* OPC_XMLREADER_H */
