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
/** @file mce/textwriter.h
 
 */
#include <mce/config.h>

#ifndef MCE_TEXTWRITER_H
#define MCE_TEXTWRITER_H

#ifdef __cplusplus
extern "C" {
#endif    

#include <opc/opc.h>
	
#define MCE_DEFAULT 0x0
#define MCE_IGNORABLE 0x1
#define MCE_MUSTUNDERSTAND 0x2
	
	typedef struct MCE_TEXTWRITER_STRUCT mceTextWriter;

	mceTextWriter *mceTextWriterOpen(opcPart *part);
	int mceTextWriterClose();
	int mceTextWriterStartDocument();
	int mceTextWriterEndDocument();
	int mceTextWriterStartElement(const xmlChar *ns, const xmlChar *ln);
	int mceTextWriterEndElement();
	const xmlChar *mceTextWriterRegisterNamespace(const xmlChar *ns, const xmlChar *prefix, int flags); 	
	int mceTextWriterProcessContent(const xmlChar *ns, const xmlChar *ln);
	int mceTextWriterAttributeF(const xmlChar *ns, const xmlChar *ln, const char *value, ...);
	int mceTextWriterStartAlternateContent();
	int mceTextWriterEndAlternateContent();
	int mceTextWriterStartChoice(const xmlChar *ns);
	int mceTextWriterEndChoice();
	int mceTextWriterStartFallback();
	int mceTextWriterEndFallback();
	
#ifdef __cplusplus
} /* extern "C" */
#endif    
		
#endif /* MCE_TEXTWRITER_H */
