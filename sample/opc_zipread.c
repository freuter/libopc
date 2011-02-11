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

typedef struct ZIPPARTSSTRUCT {
	opcZipPartInfo *partInfo_array;
	int partInfo_items;
} ZipParts;

static int partInfoCallback(void *callbackCtx, opcZip *zip) {
	ZipParts *zipParts=(ZipParts*)callbackCtx;
	zipParts->partInfo_array=(opcZipPartInfo *)xmlRealloc(zipParts->partInfo_array, (zipParts->partInfo_items+1)*sizeof(opcZipPartInfo));
	if (NULL!=zipParts->partInfo_array && opcZipInitPartInfo(zip, zipParts->partInfo_array+zipParts->partInfo_items)) {
		zipParts->partInfo_items++;
	}
	return 1;
}

static int dumpStream(opcZipPartInfo *partInfo, opcZip *zip, FILE *out) {
	opcZipDeflateStream stream;
	if (opcZipInitDeflateStream(partInfo, &stream)) {
		if (opcZipOpenDeflateStream(partInfo, &stream)) {
			int len=0;
			char buf[OPC_DEFLATE_BUFFER_SIZE];
			do {
				len=opcZipReadDeflateStream(zip, &stream, buf, sizeof(buf));
				if (len>0 && NULL!=out) {
					fwrite(buf, sizeof(char), len, out);
//					printf("%i\n", len);
//								printf("%4i %.*s\n", len, len, buf);
				}
			} while (len>0);
			opcZipCloseDeflateStream(partInfo, &stream);
		}
	}
	return 1;
}



int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		for(int i=1;i<argc;i++) {
			opcZip *zip=opcZipOpenFile(_X(argv[i]), OPC_ZIP_READ);
			if (NULL!=zip) {
				ZipParts zipParts;
				memset(&zipParts, 0, sizeof(zipParts));
				if (opZipScan(zip, &zipParts, partInfoCallback)) {
					printf("scan ok.\n");
					for(int i=0;i<zipParts.partInfo_items;i++) {
						printf("%s[%li]\n", zipParts.partInfo_array[i].partName, zipParts.partInfo_array[i].stream_ofs);
						dumpStream(&zipParts.partInfo_array[i], zip, NULL);
					}
				}
				for(int i=0;i<zipParts.partInfo_items;i++) {
					opcZipCleanupPartInfo(zipParts.partInfo_array+i);
				}
				opcZipClose(zip);
			}
		}
		opcFreeLibrary();
	}
	return 0;	
}

