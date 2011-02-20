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
#include <time.h>

typedef struct ZIPPARTSSTRUCT {
	opcZipPartInfo *partInfo_array;
	int partInfo_items;
} ZipParts;

static opc_error_t partInfoCallback(void *callbackCtx, opcZip *zip) {
	ZipParts *zipParts=(ZipParts*)callbackCtx;
	zipParts->partInfo_array=(opcZipPartInfo *)xmlRealloc(zipParts->partInfo_array, (zipParts->partInfo_items+1)*sizeof(opcZipPartInfo));
	if (NULL!=zipParts->partInfo_array && OPC_ERROR_NONE==opcZipInitPartInfo(zip, zipParts->partInfo_array+zipParts->partInfo_items)) {
		zipParts->partInfo_items++;
	}
	return OPC_ERROR_NONE;
}

static opc_error_t dumpStream(opcZipPartInfo *partInfo, opcZip *zip, FILE *out) {
    opc_error_t err=OPC_ERROR_NONE;
	opcZipDeflateStream stream;
	if (OPC_ERROR_NONE==err && OPC_ERROR_NONE==(err=opcZipInitDeflateStream(partInfo, &stream))) {
		if (OPC_ERROR_NONE==err && OPC_ERROR_NONE==(err=opcZipOpenDeflateStream(partInfo, &stream))) {
			int len=0;
			char buf[OPC_DEFLATE_BUFFER_SIZE];
			do {
				len=opcZipReadDeflateStream(zip, &stream, buf, sizeof(buf), &err);
				if (len>0 && NULL!=out) {
					fwrite(buf, sizeof(char), len, out);
				}
			} while (len>0 && OPC_ERROR_NONE==err);
			if (OPC_ERROR_NONE==err) err=opcZipCloseDeflateStream(partInfo, &stream);
		}
	}
	return err;
}



int main( int argc, const char* argv[] )
{
	time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
	if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
		for(int i=1;OPC_ERROR_NONE==err && i<argc;i++) {
			opcZip *zip=opcZipOpenFile(_X(argv[i]), OPC_ZIP_READ);
			if (NULL!=zip) {
				ZipParts zipParts;
				memset(&zipParts, 0, sizeof(zipParts));
				if (OPC_ERROR_NONE==err && OPC_ERROR_NONE==(err=opZipScan(zip, &zipParts, partInfoCallback))) {
					printf("scan ok.\n");
					for(int i=0;i<zipParts.partInfo_items;i++) {
						printf("%s[%li]\n", zipParts.partInfo_array[i].partName, zipParts.partInfo_array[i].stream_ofs);
						dumpStream(&zipParts.partInfo_array[i], zip, NULL);
					}
				}
				for(int i=0;OPC_ERROR_NONE==err && i<zipParts.partInfo_items;i++) {
					err=opcZipCleanupPartInfo(zipParts.partInfo_array+i);
				}
			} else {
                err=OPC_ERROR_STREAM;
            }
            if (OPC_ERROR_NONE!=err) {
                printf("*ERROR: %s => %i\n", argv[i], err);
            }
            opcZipClose(zip);
		}
		if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
	}
	time_t end_time=time(NULL);
	printf("time %.2lfsec\n", difftime(end_time, start_time));
	return (OPC_ERROR_NONE==err?0:3);	
}

