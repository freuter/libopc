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

int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		opcZip *zip=opcZipOpenFile(_X("sample.zip"), OPC_ZIP_WRITE);
		opcZipWriteStart(zip);
		opcZipPartInfo part1;
		opcZipWriteOpenPart(zip, &part1, _X("sample.txt"), 0);
		opcZipWritePartData(zip, &part1, "Hello World", 11);
		opcZipWriteClosePart(zip, &part1, NULL);
		opcZipPartInfo part2;
		opcZipWriteOpenPart(zip, &part2, _X("more.txt"), 0);
		opcZipWritePartData(zip, &part2, "Hello OPC", 9);
		opcZipWriteClosePart(zip, &part2, NULL);
		opcZipWriteEnd(zip);
		opcZipWriteStartDirectory(zip);
		opcZipWriteDirectoryEntry(zip, &part1);
		opcZipWriteDirectoryEntry(zip, &part2);
		opcZipWriteEndDirectory(zip);
		opcZipClose(zip);
		opcZipCleanupPartInfo(&part1);
		opcZipCleanupPartInfo(&part2);
		opcFreeLibrary();
	}
	return 0;	
}

