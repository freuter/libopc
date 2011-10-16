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
#include <time.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

/*
    This example shows how to use the low level zip functions.

    Ussage:
    opc_zipread FILENAME [--import] [--delete] [--add] [--commit] [--trim]

    * --import existing streams
    * --delete all streams
    * --add sample streams
    * --commit streams
    * --trim commit and trim streams

    Sample:
    opc_zipread out.zip --delete --import --add --commit
*/

static opc_error_t addSegment(void *iocontext, 
                       void *userctx, 
                       opcZipSegmentInfo_t *info, 
                       opcZipLoaderOpenCallback *open, 
                       opcZipLoaderReadCallback *read, 
                       opcZipLoaderCloseCallback *close, 
                       opcZipLoaderSkipCallback *skip) {
    opcZip *zip=(opcZip *)userctx;
    OPC_ENSURE(0==skip(iocontext));
    OPC_ENSURE(-1!=opcZipLoadSegment(zip, xmlStrdup(info->name), info->rels_segment, info));
    return OPC_ERROR_NONE;
}

static opc_error_t releaseSegment(opcZip *zip, opc_uint32_t segment_id) {
    const xmlChar *name=NULL;
    OPC_ENSURE(OPC_ERROR_NONE==opcZipGetSegmentInfo(zip, segment_id, &name, NULL, NULL));
    OPC_ASSERT(NULL!=name);
    xmlFree((void*)name);
    return OPC_ERROR_NONE;
}


int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        if (argc>0) {
            opcIO_t io;
            if (OPC_ERROR_NONE==opcFileInitIOFile(&io, _X(argv[1]), OPC_FILE_READ | OPC_FILE_WRITE)) {
                opcZip *zip=opcZipCreate(&io);
                if (NULL!=zip) {
                    for(int i=2;i<argc;i++) {
                        if (0==strcmp(argv[i], "--import")) { // import existing segments
                            OPC_ENSURE(OPC_ERROR_NONE==opcZipLoader(&io, zip, addSegment));
                        } else if (0==strcmp(argv[i], "--delete")) { // delete all segments
                            for(opc_uint32_t i=opcZipGetFirstSegmentId(zip);-1!=i;i=opcZipGetNextSegmentId(zip, i)) {
                                opc_uint32_t first_segment=i;
                                opc_uint32_t last_segment=i;
                                OPC_ENSURE(opcZipSegmentDelete(zip, &first_segment, &last_segment, releaseSegment));
                            }
                        } else if (0==strcmp(argv[i], "--add")) { // add sample streams
                            static char txt[]="Hello World!";
                            opc_uint32_t segment1_id=opcZipCreateSegment(zip, xmlStrdup(_X("hello.txt")), OPC_FALSE, 47+5, 0, 0, 0);
                            opc_uint32_t segment2_id=opcZipCreateSegment(zip, xmlStrdup(_X("stream.txt")), OPC_FALSE, 47+5, 0, 8, 6);
                            if (-1!=segment1_id) {
                                opcZipOutputStream *out=opcZipOpenOutputStream(zip, &segment1_id);
                                OPC_ENSURE(opcZipWriteOutputStream(zip, out, _X(txt), strlen(txt))==strlen(txt));
                                OPC_ENSURE(OPC_ERROR_NONE==opcZipCloseOutputStream(zip, out, &segment1_id));
                            }
                            if (-1!=segment2_id) {
                                opcZipOutputStream *out=opcZipOpenOutputStream(zip, &segment2_id);
                                OPC_ENSURE(opcZipWriteOutputStream(zip, out, _X(txt), strlen(txt))==strlen(txt));
                                OPC_ENSURE(OPC_ERROR_NONE==opcZipCloseOutputStream(zip, out, &segment2_id));
                            }
                        } else if (0==strcmp(argv[i], "--commit")) { // commit 
                            OPC_ENSURE(OPC_ERROR_NONE==opcZipCommit(zip, OPC_FALSE));
                        } else if (0==strcmp(argv[i], "--trim")) { // commit and trim
                            OPC_ENSURE(OPC_ERROR_NONE==opcZipCommit(zip, OPC_TRUE));
                        }
                    }
                    opcZipClose(zip, releaseSegment);
                }
                OPC_ENSURE(OPC_ERROR_NONE==opcFileCleanupIO(&io));
            }
        }
        if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
    }
    time_t end_time=time(NULL);
    fprintf(stdout, "time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}

