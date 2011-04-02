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


/*
    This example shows how to use the low level zip functions.
*/

static opc_error_t addSegment(void *iocontext, 
                       void *userctx, 
                       opcZipSegmentInfo_t *info, 
                       opcFileOpenCallback *open, 
                       opcFileReadCallback *read, 
                       opcFileCloseCallback *close, 
                       opcFileSkipCallback *skip) {
    opcZip *zip=(opcZip *)userctx;
    OPC_ENSURE(0==skip(iocontext));
    OPC_ENSURE(-1!=opcZipLoadSegment(zip, xmlStrdup(info->name), info->rels_segment, info));
    return OPC_ERROR_NONE;
}

int main( int argc, const char* argv[] )
{
    time_t start_time=time(NULL);
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        for(int i=1;OPC_ERROR_NONE==err && i<argc;i++) {
            opcIO_t io;
            if (OPC_ERROR_NONE==opcFileInitIOFile(&io, _X(argv[i]), OPC_FILE_READ | OPC_FILE_WRITE)) {
                opcZip *zip=opcZipCreate(&io);

                if (NULL!=zip) { // import existing segments
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipLoader(&io, zip, addSegment));
                }
                if (1 && NULL!=zip) {
                    static char txt[]="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris lectus dui, convallis a rutrum aliquet, sagittis eu neque. Vivamus ipsum lorem, luctus vitae dictum a, dignissim eget sem. Maecenas tincidunt velit sed mi venenatis nec egestas eros elementum. Donec sagittis, eros vitae aliquam bibendum, purus quam gravida erat, ut lacinia turpis sapien non tellus. Nunc sit amet nulla eros. Morbi sed risus vitae eros luctus tristique. Aliquam sed facilisis tellus. Donec purus lorem, vestibulum sed placerat at, molestie eu lorem. Nullam semper felis a dui mollis eu fringilla odio pretium. Vivamus nec facilisis arcu. Maecenas cursus ipsum sit amet felis malesuada adipiscing. Ut a lorem et diam adipiscing vulputate sit amet ut libero. In a nunc metus. Praesent porta metus vel neque tincidunt a gravida ante fermentum. Etiam neque lectus, rutrum et hendrerit in, interdum ac tellus. Aliquam et justo augue. Nam egestas justo at lectus congue ornare. Sed cursus iaculis velit id rhoncus. Aliquam tellus est, mollis nec rutrum eget, auctor a libero. Morbi blandit, odio non aliquam fermentum, augue lacus mattis eros, et bibendum quam nisl eu dolor.\nCras hendrerit, nulla in aliquet convallis, nisi justo porttitor felis, sed dignissim enim purus accumsan libero. Aenean pretium, nisi a auctor rhoncus, orci lectus faucibus tellus, ac mollis sapien ligula quis nibh. Donec faucibus ipsum non velit rhoncus eget rhoncus enim varius. Mauris neque quam, consectetur eu convallis et, sollicitudin sit amet nisi. Praesent sit amet diam ac est pulvinar dapibus. Ut sem ligula, lobortis vel pharetra vitae, venenatis vestibulum metus. Ut eget risus at lorem molestie pellentesque. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Sed non lacus vel sapien viverra faucibus. Nullam tellus nulla, interdum non convallis porttitor, tristique ut tellus. Curabitur sollicitudin tincidunt orci. Vestibulum lacinia tortor nec lacus sollicitudin dictum. Etiam vitae lorem in ligula vehicula pharetra. Sed gravida sapien eget orci laoreet rutrum.\nDonec ut lectus ligula. Vivamus eget massa sapien. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Donec a erat eu turpis scelerisque iaculis. Vestibulum vitae lectus id enim varius fringilla ac ut lectus. Aliquam pretium sollicitudin urna vel adipiscing. Vestibulum vel neque eget lectus sodales sodales ut vitae purus. Phasellus tempus est malesuada sapien ultrices non fringilla nibh bibendum. Donec ac purus ante, id accumsan sapien. Cras quis elit eget sapien tincidunt volutpat ut ut neque. Ut volutpat justo a arcu iaculis et vulputate elit ullamcorper. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Etiam laoreet elit eget mi auctor vehicula vel tempus erat. Nunc facilisis dui nec augue iaculis semper non tempor purus. Quisque orci enim, facilisis vitae venenatis et, fringilla ut nulla. Nulla nec purus ac diam auctor elementum in non sem. Integer tincidunt pellentesque faucibus. Quisque facilisis scelerisque eros in tempus.\nMorbi ornare eros at dui dapibus at dapibus arcu venenatis. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Duis a lacus leo, in aliquet tellus. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Mauris tortor erat, tincidunt quis rhoncus pharetra, faucibus vel odio. Aliquam scelerisque lectus ut nulla varius ac dignissim risus iaculis. Suspendisse vestibulum turpis id tortor porttitor in rutrum lectus vestibulum. In auctor, orci auctor iaculis pharetra, urna orci condimentum mi, et lobortis dui leo non dui. Proin faucibus consectetur placerat. Nunc quis libero risus, vitae ultricies erat. Nullam cursus volutpat sapien nec consectetur. Phasellus quam lectus, accumsan sed porta et, suscipit sed nunc. Phasellus urna sem, consectetur et consectetur at, tempor sed sapien. Donec aliquam felis non sem mattis et egestas ipsum ornare. Maecenas nec leo vel neque pharetra tempus. Quisque tempus auctor enim ut dapibus. Duis tincidunt pulvinar eleifend. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\nDuis blandit, felis id ultrices vulputate, est justo tincidunt libero, ac ullamcorper nisl metus a felis. Nunc commodo tempor orci et fringilla. Quisque nec turpis metus, non faucibus purus. Suspendisse dolor tellus, volutpat a venenatis ac, cursus sed est. Nulla ullamcorper tincidunt venenatis. Morbi vulputate aliquet ligula, congue mattis nibh adipiscing accumsan. Maecenas vitae libero et nisi hendrerit malesuada et pellentesque tortor. Morbi at tellus at odio bibendum venenatis. Nulla non odio nec neque posuere elementum. Suspendisse potenti. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Sed hendrerit, purus a tincidunt dignissim, turpis dolor feugiat tellus, nec tempor magna lacus ac felis.";
//                    static char txt[]="Hallo Welt";
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
                }
                if (NULL!=zip) { 
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCommit(zip, OPC_FALSE));
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCommit(zip, OPC_TRUE));
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
            }
            OPC_ENSURE(OPC_ERROR_NONE==opcFileCleanupIO(&io));
        }
        if (OPC_ERROR_NONE==err) err=opcFreeLibrary();
    }
    time_t end_time=time(NULL);
    printf("time %.2lfsec\n", difftime(end_time, start_time));
    return (OPC_ERROR_NONE==err?0:3);	
}

