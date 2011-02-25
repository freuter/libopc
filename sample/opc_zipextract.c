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
#include <libxml/xmlstring.h>
#include <time.h>


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

static opc_error_t partInfoCallback(void *callbackCtx, opcZip *zip) {
    opcZipPartInfo partInfo;
    opc_error_t err=OPC_ERROR_NONE;
    xmlChar *filename=(xmlChar*)callbackCtx;
    if (OPC_ERROR_NONE==(err=opcZipInitPartInfo(zip, &partInfo))) {
        if (NULL==filename) {
            printf("%s\n", partInfo.partName);
        } else if (xmlStrcmp(filename, partInfo.partName)==0) {
            if (OPC_ERROR_NONE==(err=dumpStream(&partInfo, zip, stdout))) {
                err=opcZipConsumedPartInCallback(zip, &partInfo);
            }
        }
        if (OPC_ERROR_NONE==err) {
            err=opcZipCleanupPartInfo(&partInfo);
        }

    }
    return err;
}

int main( int argc, const char* argv[] )
{
    if (argc<2 || argc>3) {
        printf("opc_zipextract ZIP-FILE [PART-NAME]\n");
        return 1;
    } else {
        time_t start_time=time(NULL);
        opc_error_t err=OPC_ERROR_NONE;
        if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
            opcZip *zip=opcZipOpenFile(_X(argv[1]), OPC_ZIP_READ);
            if (NULL!=zip) {
                err=opZipScan(zip, argc==3?_X(argv[2]):NULL, partInfoCallback);
            } else {
                err=OPC_ERROR_STREAM;
            }
        }
        if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
        if (OPC_ERROR_NONE!=err) {
            fprintf(stderr, "*ERROR: %s => %i\n", argv[1], err);
        }
        time_t end_time=time(NULL);
        fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
        return (OPC_ERROR_NONE==err?0:3);	
    }
}

