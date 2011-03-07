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

typedef struct MYPARTSTRUCT {
    opcZipSegment segment;
    opc_uint32_t segment_number;
    opc_bool_t last_segment;
    xmlChar *name;
} myPart;


typedef struct MYCONTAINERSTRUCT {
    myPart *part_array;
    opc_uint32_t part_items;
} myContainer;

myPart* ensurePart(myContainer *container) {
    container->part_array=(myPart *)xmlRealloc(container->part_array, (container->part_items+1)*sizeof(myPart));
    myPart* ret=&container->part_array[container->part_items];
    opc_bzero_mem(ret, sizeof(*ret));
    return ret;
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        for(int i=1;OPC_ERROR_NONE==err && i<argc;i++) {
            opcZip *zip=opcZipOpenFile(_X(argv[i]), OPC_ZIP_READ);
            if (NULL!=zip) {
                myContainer container;
                memset(&container, 0, sizeof(myContainer));
                opcZipRawBuffer rawBuffer;
                OPC_ENSURE(OPC_ERROR_NONE==opcZipInitRawBuffer(zip, &rawBuffer));
                myPart *part=NULL;
                while(NULL!=(part=ensurePart(&container)) && opcZipRawReadLocalFile(zip, &rawBuffer, &part->segment, &part->name, &part->segment_number, &part->last_segment)) {
                    opcZipInflateState inflateState;
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipInitInflateState(&rawBuffer.state, &part->segment, &inflateState));
                    opc_uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
                    opc_uint32_t len=0;
                    opc_uint32_t crc=0;
                    while((len=opcZipRawReadFileData(zip, &rawBuffer, &inflateState, buf, sizeof(buf)))>0) {
                        crc=crc32(crc, buf, len);
                    }
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipRawReadDataDescriptor(zip, &rawBuffer, &part->segment));
                    OPC_ASSERT(crc==part->segment.crc32);
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupInflateState(&rawBuffer.state, &part->segment, &inflateState));
                    printf("%s\n", part->name);
                    part=NULL; container.part_items++;
                }
                opcZipSegment segment;
                xmlChar *name=NULL;
                opc_uint32_t segment_number;
                opc_bool_t last_segment;
                while(opcZipRawReadCentralDirectory(zip, &rawBuffer, &segment, &name, &segment_number, &last_segment)) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupSegment(&segment));
                    xmlFree(name);
                }
                opc_uint16_t central_dir_entries=0;
                OPC_ENSURE(opcZipRawReadEndOfCentralDirectory(zip, &rawBuffer, &central_dir_entries));
                for(opc_uint32_t i=0;i<container.part_items;i++) {
                    opcZipSegmentInputStream *stream=opcZipCreateSegmentInputStream(zip, &container.part_array[i].segment);
                    opc_uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
                    opc_uint32_t len=0;
                    opc_uint32_t crc=0;
                    while((len=opcZipReadSegmentInputStream(zip, stream, buf, sizeof(buf)))>0) {
                        crc=crc32(crc, buf, len);
                    }
                    OPC_ASSERT(crc==container.part_array[i].segment.crc32);
                    printf("%s [%08X]\n", container.part_array[i].name, crc);
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCloseSegmentInputStream(zip, &container.part_array[i].segment, stream));
                }
                for(opc_uint32_t i=0;i<container.part_items;i++) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupSegment(&container.part_array[i].segment));
                    xmlFree(container.part_array[i].name);
                }
            }
            opcZipClose(zip);
        }
        if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
    }
    time_t end_time=time(NULL);
    printf("time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}
