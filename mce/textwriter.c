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

mceTextWriter *mceTextWriterOpen(opcPart *part) {
	return NULL;
}

int mceTextWriterClose() {
	return 0;
}

int mceTextWriterStartDocument() {
	return 0;
}

int mceTextWriterEndDocument() {
	return 0;
}

int mceTextWriterStartElement(const xmlChar *ns, const xmlChar *ln) {
	return 0;
}

int mceTextWriterEndElement() {
	return 0;
}

const xmlChar *mceTextWriterRegisterNamespace(const xmlChar *ns, const xmlChar *prefix, int flags) {
	return 0;
}

int mceTextWriterProcessContent(const xmlChar *ns, const xmlChar *ln) {
	return 0;
}

int mceTextWriterAttributeF(const xmlChar *ns, const xmlChar *ln, const char *value, ...) {
	return 0;
}

int mceTextWriterStartAlternateContent() {
	return 0;
}

int mceTextWriterEndAlternateContent() {
	return 0;
}

int mceTextWriterStartChoice(const xmlChar *ns) {
	return 0;
}

int mceTextWriterEndChoice() {
	return 0;
}

int mceTextWriterStartFallback() {
	return 0;
}

int mceTextWriterEndFallback() {
	return 0;
}
