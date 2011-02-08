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

opcZip *opcZipOpenFile(const xmlChar *fileName, int flags) {
	return NULL;
}

int opcZipClose(opcZip *zip) {
	return 0;
}

int opcZipWriteStart(opcZip *zip) {
	return 0;
}

int opcZipWriteEnd(opcZip *zip)  {
	return 0;
}

int opcZipWriteOpenPart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *partName, int growthHint)  {
	return 0;
}

int opcZipWritePartData(opcZip *zip, opcZipPartInfo *partInfo, const char *buf, int bufLen)  {
	return 0;
}

int opcZipWriteClosePart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *newName)  {
	return 0;
}


int opcZipWriteStartDirectory(opcZip *zip) {
	return 0;
}

int opcZipWriteDirectoryEntry(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipWriteEndDirectory(opcZip *zip) {
	return 0;
}

int opcZipReadStart(opcZip *zip) {
	return 0;
}

int opcZipReadPartInfo(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadSkipPart(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadEnd(opcZip *zip) {
	return 0;
}

int opcZipCleanupPartInfo(opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadDataStart(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo) {
	return 0;
}

int opcZipReadData(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo, char *buf, int bufLen) {
	return 0;
}

int opcZipReadDataEnd(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo) {
	return 0;
}

int opcZipReadDirectoryStart(opcZip *zip) {
	return 0;
}

int opcZipReadDirectoryInfo(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadDirectoryEnd(opcZip *zip) {
	return 0;
}

int opcZipIsStreamMode(opcZip *zip) {
	return 0;
}

int opcZipMovePart(opcZip *zip, opcZipPartInfo *partInfo, int delta) {
	return 0;
}

int opcZipCopyPart(opcZip *zip, opcZipPartInfo *partInfo, opcZip *target) {
	return 0;
}

int opcZipSwapPart(opcZip *zip, opcZipPartInfo *partInfo, int minGapSize) {
	return 0;
}

int opcZipGetPhysicalPartSize(opcZipPartInfo *partInfo) {
	return 0;
}
