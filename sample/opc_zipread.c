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
#include <opc/zip.h> 
#include <zlib.h> // for crc32 function

/*
    This module demonstrates the low-level opcZipLoader functionality. You can use this method to get very information about a
    ZIP file. This can also be used to implement streaming-based access to ZIP files.

    Ussage:
    opc_zipread [--verify] [--skip] FILENAME

    * --verify will verify the checksums.
    * --skip will quickly skip the streams.

    Sample:
    opc_zipread --verify OOXMLI1.docx
    opc_zipread --skip OOXMLI1.docx
*/

static opc_bool_t verify_crc = OPC_FALSE;

opc_error_t loadSegment(void *iocontext, 
                        void *userctx, 
                        opcZipSegmentInfo_t *info,
                        opcFileOpenCallback *open, 
                        opcFileReadCallback *read, 
                        opcFileCloseCallback *close, 
                        opcFileSkipCallback *skip) {
    printf("%i: %s%s(%i%s) %i/%i %i/%i...", info->stream_ofs, 
                              info->name, 
                              (info->rels_segment?"(.rels)":""),
                              info->segment_number,
                              (info->last_segment?".last":""),
                              info->compressed_size, info->uncompressed_size,
                              info->min_header_size, info->header_size);
    if (!verify_crc) {
        // enable this to SKIP throught the files FAST.
        OPC_ENSURE(0==skip(iocontext));
        printf("skipped\n");
    } else {
        // enable this to very the CRC checksums
        opc_bool_t ok=OPC_FALSE;
        if (0==open(iocontext)) {
            opc_uint32_t crc=0;
            char buf[OPC_DEFLATE_BUFFER_SIZE];
            int ret=0;
            while((ret=read(iocontext, buf, sizeof(buf)))>0) {
                crc=crc32(crc, (const Bytef*)buf, ret);
            }
            OPC_ENSURE(0==close(iocontext));
            ok=(info->data_crc==crc);
        }
        if (ok) {
            printf("ok\n");
        } else {
            printf("failure\n");
        }
    }
    return OPC_ERROR_NONE;
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        for(int i=1;OPC_ERROR_NONE==err && i<argc;i++) {
            if (xmlStrcasecmp(_X(argv[i]), _X("--verify"))==0) {
                verify_crc=OPC_TRUE;
            } else if (xmlStrcasecmp(_X(argv[i]), _X("--skip"))==0) {
                verify_crc=OPC_FALSE;
            } else {
                opcIO_t io;
                if (OPC_ERROR_NONE==opcFileInitIOFile(&io, _X(argv[i]), OPC_FILE_READ)) {
                    err=opcZipLoader(&io, NULL, loadSegment);
                }
                OPC_ENSURE(OPC_ERROR_NONE==opcFileCleanupIO(&io));
            }
        }
        if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
    }
    time_t end_time=time(NULL);
    fprintf(stderr, "time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}
