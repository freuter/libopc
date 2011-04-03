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
#include <zlib.h> // for crc32 function

opc_error_t loadSegment(void *iocontext, 
                        void *userctx, 
                        opcZipSegmentInfo_t *info,
                        opcFileOpenCallback *open, 
                        opcFileReadCallback *read, 
                        opcFileCloseCallback *close, 
                        opcFileSkipCallback *skip) {
    opcZip *zip=(opcZip*)userctx;
//    OPC_ENSURE(0==skip(iocontext));
    OPC_ENSURE(0==open(iocontext));
    opc_uint32_t crc=0;
    char buf[100];
    int ret=0;
    while((ret=read(iocontext, buf, sizeof(buf)))>0) {
        crc=crc32(crc, (const Bytef*)buf, ret);
    }
    OPC_ASSERT(info->data_crc==crc);
    OPC_ASSERT(ret>=0);
    OPC_ENSURE(0==close(iocontext));
    opcZipLoadSegment(zip, xmlStrdup(info->name), info->rels_segment, info);
    return OPC_ERROR_NONE;
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        if (argc>2) {
            opcIO_t io;
            if (OPC_ERROR_NONE==opcFileInitIOFile(&io, _X(argv[1]), OPC_FILE_READ)) {
                opcZip *zip=opcZipCreate(&io);
                if (NULL!=zip) {
                    err=opcZipLoader(&io, zip, loadSegment);
                    if (OPC_ERROR_NONE==err) {
                        // successfully loaded; dump all segments
                        for(opc_uint32_t segment_id=opcZipGetFirstSegmentId(zip);
                            -1!=segment_id;
                            segment_id=opcZipGetNextSegmentId(zip, segment_id)) {
                            const xmlChar *name=NULL;
                            opc_bool_t rels_segment=OPC_FALSE;
                            opc_uint32_t data_crc=0;
                            OPC_ENSURE(OPC_ERROR_NONE==opcZipGetSegmentInfo(zip, segment_id, &name, &rels_segment, &data_crc));
                            OPC_ASSERT(NULL!=name);
                            if (!rels_segment && 0==xmlStrcmp(name, _X(argv[2]))) {
                                printf("extracting  \"%s\"\n", name);
                                opcZipInputStream *stream=opcZipOpenInputStream(zip, segment_id);
                                if (NULL!=stream) {
                                    opc_uint32_t crc=0;
                                    opc_uint8_t buf[100];
                                    opc_uint32_t ret=0;
                                    while((ret=opcZipReadInputStream(zip, stream , buf, sizeof(buf)))>0) {
//                                        printf("%.*s", ret, buf);
                                        crc=crc32(crc, (const Bytef*)buf, ret);
                                    }
                                    OPC_ASSERT(crc==data_crc);
                                    opcZipCloseInputStream(zip, stream);
                                }
                            } else {
                                printf("skip \"%s\" %s\n", name, (rels_segment?"(.rels)":""));
                            }
                        }

                    }
                    // free names
                    for(opc_uint32_t segment_id=opcZipGetFirstSegmentId(zip);
                        -1!=segment_id;
                        segment_id=opcZipGetNextSegmentId(zip, segment_id)) {
                        const xmlChar *name=NULL;
                        OPC_ENSURE(OPC_ERROR_NONE==opcZipGetSegmentInfo(zip, segment_id, &name, NULL, NULL));
                        OPC_ASSERT(NULL!=name);
                        xmlFree((void*)name);
                    }
                    opcZipClose(zip);
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
