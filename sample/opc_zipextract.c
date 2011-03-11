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


static opc_error_t dumpStream(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment, FILE *out) {
    opc_error_t err=OPC_ERROR_NONE;
    opcZipInflateState inflateState;
    OPC_ENSURE(OPC_ERROR_NONE==opcZipInitInflateState(&rawBuffer->state, segment, &inflateState));
    opc_uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
    opc_uint32_t len=0;
    opc_uint32_t crc=0;
    while((len=opcZipRawReadFileData(zip, rawBuffer, &inflateState, buf, sizeof(buf)))>0) {
        crc=crc32(crc, buf, len);
        if (NULL!=out) fwrite(buf, sizeof(puint8_t), len, out);
    }
    OPC_ENSURE(OPC_ERROR_NONE==opcZipRawReadDataDescriptor(zip, rawBuffer, segment));
    OPC_ASSERT(crc==segment->crc32);
    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupInflateState(&rawBuffer->state, segment, &inflateState));
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
                opcZipRawBuffer rawBuffer;
                OPC_ENSURE(OPC_ERROR_NONE==opcZipInitRawBuffer(zip, &rawBuffer));
                opcZipSegment segment;
                xmlChar *name=NULL;
                opc_uint32_t segment_number;
                opc_bool_t last_segment;
                while(opcZipRawReadLocalFile(zip, &rawBuffer, &segment, &name, &segment_number, &last_segment, NULL)) {
                    if (0==xmlStrcmp(_X(argv[2]), name)) {
                        OPC_ENSURE(OPC_ERROR_NONE==dumpStream(zip, &rawBuffer, &segment, stdout));
                    } else {
                        OPC_ENSURE(OPC_ERROR_NONE==opcZipRawSkipFileData(zip, &rawBuffer, &segment));
                        OPC_ENSURE(OPC_ERROR_NONE==opcZipRawReadDataDescriptor(zip, &rawBuffer, &segment));
                    }
                    opcZipCleanupSegment(&segment);
                    xmlFree(name);
                }
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

