/**
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
#include <stdio.h>
#include <string.h>

int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		char buf[500];
		memset(buf, 'X', sizeof(buf));
		opcZip *zip=opcZipOpenFile(_X("sample5.zip"), OPC_ZIP_WRITE | OPC_ZIP_STREAM);
		opcZipWriteStart(zip);
		
		opcZipPartInfo spine_0;
		opcZipWriteOpenPart(zip, &spine_0, _X("spine.xml/[0].piece"), 0);
		opcZipWritePartData(zip, &spine_0, buf, 100);
		opcZipWriteClosePart(zip, &spine_0, NULL);

		opcZipPartInfo page0_0;
		opcZipWriteOpenPart(zip, &page0_0, _X("pages/page0.xml/[0].piece"), 0);
		opcZipWritePartData(zip, &page0_0, buf, 100);
		opcZipWriteClosePart(zip, &page0_0, NULL);

		opcZipPartInfo page0_1;
		opcZipWriteOpenPart(zip, &page0_1, _X("pages/page0.xml/[1].last.piece"), 0);
		opcZipWriteClosePart(zip, &page0_1, NULL);
		
		opcZipPartInfo spine_1;
		opcZipWriteOpenPart(zip, &spine_1, _X("spine.xml/[1].piece"), 0);
		opcZipWritePartData(zip, &spine_1, buf, 500);
		opcZipWriteClosePart(zip, &spine_1, NULL);
		
		opcZipPartInfo page1_0;
		opcZipWriteOpenPart(zip, &page1_0, _X("pages/page1.xml/[0].piece"), 0);
		opcZipWritePartData(zip, &page1_0, buf, 100);
		opcZipWriteClosePart(zip, &page1_0, NULL);
		
		opcZipPartInfo page1_1;
		opcZipWriteOpenPart(zip, &page1_1, _X("pages/page1.xml/[1].last.piece"), 0);
		opcZipWriteClosePart(zip, &page1_1, NULL);
		
		opcZipPartInfo spine_2;
		opcZipWriteOpenPart(zip, &spine_2, _X("spine.xml/[2].piece"), 0);
		opcZipWritePartData(zip, &spine_2, buf, 500);
		opcZipWriteClosePart(zip, &spine_2, NULL);
		
		opcZipPartInfo spine_3;
		opcZipWriteOpenPart(zip, &spine_3, _X("spine.xml/[3].last.piece"), 0);
		opcZipWriteClosePart(zip, &spine_3, NULL);
		
		opcZipWriteStartDirectory(zip);
		opcZipWriteDirectoryEntry(zip, &spine_0);
		opcZipWriteDirectoryEntry(zip, &page0_0);
		opcZipWriteDirectoryEntry(zip, &page0_1);							 
		opcZipWriteDirectoryEntry(zip, &spine_1);
		opcZipWriteDirectoryEntry(zip, &page1_0);
		opcZipWriteDirectoryEntry(zip, &page1_1);
		opcZipWriteDirectoryEntry(zip, &spine_2);
		opcZipWriteDirectoryEntry(zip, &spine_3);
		opcZipWriteEndDirectory(zip);
		opcZipClose(zip);
		opcZipCleanupPartInfo(&spine_0);
		opcZipCleanupPartInfo(&spine_1);
		opcZipCleanupPartInfo(&spine_2);
		opcZipCleanupPartInfo(&spine_3);
		opcZipCleanupPartInfo(&page0_0);		
		opcZipCleanupPartInfo(&page0_1);		
		opcZipCleanupPartInfo(&page1_0);		
		opcZipCleanupPartInfo(&page1_1);		
		opcFreeLibrary();
	}
	return 0;	
}

