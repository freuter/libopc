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
/** @file opc/zip.h
 
 */
#include <opc/config.h>

#ifndef OPC_ZIP_H
#define OPC_ZIP_H

#ifdef __cplusplus
extern "C" {
#endif    

	typedef struct OPC_ZIP_STRUCT opcZip;
	typedef struct OPC_ZIPPARTINFO_STRUCT {
		const xmlChar *partName;
	} opcZipPartInfo;
	typedef struct OPC_ZIPREADINFO_STRUCT {
		
	} opcZipReadInfo;
	
#define OPC_ZIP_READ  0x1
#define OPC_ZIP_WRITE 0x2
#define OPC_ZIP_STREAM  0x4
	
	opcZip *opcZipOpenFile(const xmlChar *fileName, int flags);
	int opcZipClose(opcZip *zip);
	
	int opcZipWriteStart(opcZip *zip);
	int opcZipWriteEnd(opcZip *zip);
	
	int opcZipWriteOpenPart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *partName, int growthHint);
	int opcZipWritePartData(opcZip *zip, opcZipPartInfo *partInfo, const char *buf, int bufLen);
	int opcZipWriteClosePart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *newName);
	
	int opcZipWriteStartDirectory(opcZip *zip);
	int opcZipWriteDirectoryEntry(opcZip *zip, opcZipPartInfo *partInfo);
	int opcZipWriteEndDirectory(opcZip *zip);

	int opcZipReadStart(opcZip *zip);	
	int opcZipReadPartInfo(opcZip *zip, opcZipPartInfo *partInfo);
	int opcZipReadSkipPart(opcZip *zip, opcZipPartInfo *partInfo);
	int opcZipReadEnd(opcZip *zip);	
	
	int opcZipCleanupPartInfo(opcZipPartInfo *partInfo);

	int opcZipReadDataStart(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo);
	int opcZipReadData(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo, char *buf, int bufLen);
	int opcZipReadDataEnd(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo);

	int opcZipReadDirectoryStart(opcZip *zip);	
	int opcZipReadDirectoryInfo(opcZip *zip, opcZipPartInfo *partInfo);
	int opcZipReadDirectoryEnd(opcZip *zip);	
	
	int opcZipIsStreamMode(opcZip *zip);
	
	int opcZipMovePart(opcZip *zip, opcZipPartInfo *partInfo, int delta);
	int opcZipCopyPart(opcZip *zip, opcZipPartInfo *partInfo, opcZip *target);
	int opcZipSwapPart(opcZip *zip, opcZipPartInfo *partInfo, int minGapSize);
	
	int opcZipGetPhysicalPartSize(opcZipPartInfo *partInfo);
#ifdef __cplusplus
} /* extern "C" */
#endif    
		
#endif /* OPC_ZIP_H */
