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


/*
    This example shows how to use the low level zip functions.
*/

typedef struct MYPART_STRUCT {
    xmlChar *name;
    opc_uint32_t first_segment_id;
    opc_uint32_t last_segment_id;
    opc_uint32_t segment_count;
} myPart;

typedef struct MYSEGMENT_STRUCT {
    opcZipSegment zipSegment;
    opc_uint32_t next_segment_id;
} mySegment;

typedef struct MYCONTAINER_STRUCT {
    myPart *part_array;
    opc_uint32_t part_items;
    mySegment *segment_array;
    opc_uint32_t segment_items;
    opcZip *zip;
    opc_uint32_t         active_write_id;  // NEEDED
    opcZipRawState       zip_write_state;  // FOR
    opcZipSegmentBuffer *zip_write_buffer; // WRITE
} myContainer;

static void* ensureItem(void **array_, puint32_t items, puint32_t item_size) {
    *array_=xmlRealloc(*array_, (items+1)*item_size);
    return *array_;
}

static myPart* ensurePart(myContainer *container) {
    return ((myPart*)ensureItem((void**)&container->part_array, container->part_items, sizeof(myPart)))+container->part_items;
}

static mySegment* ensureSegment(myContainer *container) {
    return ((mySegment*)ensureItem((void**)&container->segment_array, container->segment_items, sizeof(mySegment)))+container->segment_items;
}

static void myContainerInit(myContainer *container) {
    opc_bzero_mem(container, sizeof(*container));
    container->active_write_id=-1;
}

static opc_error_t _myContainerWriteCentralDir(myContainer *container) {
    for(puint32_t i=0;i<container->part_items;i++) {
        myPart *part=&container->part_array[i];
        puint32_t segment_id=part->first_segment_id;
        puint32_t segment_number=0;
        while(-1!=segment_id) {
            OPC_ASSERT(segment_id>=0 && segment_id<container->segment_items);
            mySegment *segment=&container->segment_array[segment_id];
            OPC_ENSURE(OPC_ERROR_NONE==opcZipUpdateHeader(container->zip, &container->zip_write_state, &segment->zipSegment, part->name, segment_number, -1==segment->next_segment_id));
            segment_id=segment->next_segment_id;
            segment_number++;
        }
        OPC_ASSERT(segment_number==part->segment_count);
    }

    opc_ofs_t central_dir_start_ofs=container->zip_write_state.buf_pos;
    opc_uint32_t total=0;
    for(puint32_t i=0;i<container->part_items;i++) {
        myPart *part=&container->part_array[i];
        OPC_ASSERT(part->first_segment_id>=0 && part->first_segment_id<container->segment_items);
        OPC_ASSERT(part->last_segment_id>=0 && part->last_segment_id<container->segment_items);
        puint32_t segment_id=part->first_segment_id;
        puint32_t segment_number=0;
        while(-1!=segment_id) {
            OPC_ASSERT(segment_id>=0 && segment_id<container->segment_items);
            mySegment *segment=&container->segment_array[segment_id];
            OPC_ENSURE(OPC_ERROR_NONE==opcZipWriteCentralDirectory(container->zip, &container->zip_write_state, &segment->zipSegment, part->name, segment_number, -1==segment->next_segment_id));
            OPC_ASSERT(-1!=segment->next_segment_id || part->last_segment_id==segment_id);
            segment_id=segment->next_segment_id;
            total++;
            segment_number++;
        }
        OPC_ASSERT(segment_number==part->segment_count);
    }
    OPC_ASSERT(container->segment_items==total);
    OPC_ENSURE(OPC_ERROR_NONE==opcZipWriteEndOfCentralDirectory(container->zip, &container->zip_write_state, central_dir_start_ofs, total));
    return container->zip_write_state.err;
}


static void myContainerCleanup(myContainer *container) {
    _myContainerWriteCentralDir(container);
    for(puint32_t i=0;i<container->part_items;i++) {
        xmlFree(container->part_array[i].name);
    }
    for(puint32_t i=0;i<container->segment_items;i++) {
        opcZipCleanupSegment(&container->segment_array[i].zipSegment);
    }
    if (NULL!=container->part_array) xmlFree(container->part_array);
    if (NULL!=container->segment_array) xmlFree(container->segment_array);
    opcZipClose(container->zip);
}

static opc_error_t _myContainerFinalizeActiveSegment(myContainer *container) {
    if (container->active_write_id>=0 && container->active_write_id<container->part_items) {
        myPart *part=&container->part_array[container->active_write_id];
        OPC_ASSERT(NULL!=container->zip_write_buffer);
        OPC_ASSERT(part->first_segment_id>=0 && part->first_segment_id<container->segment_items); // we should habe a valid segment id
        OPC_ASSERT(part->last_segment_id>=0 && part->last_segment_id<container->segment_items); // we should habe a valid segment id
        OPC_ENSURE(opcZipSegmentBufferClose(container->zip, &container->zip_write_state, container->zip_write_buffer));
        mySegment *segment=&container->segment_array[part->last_segment_id];
        OPC_ENSURE(NULL==opcZipFinalizeSegment(container->zip, &container->zip_write_state, &segment->zipSegment, container->zip_write_buffer));
        container->zip_write_buffer=NULL;
        container->active_write_id=-1;
    }
    return container->zip_write_state.err;
}

static opc_error_t _myContainerEnsureActiveSegment(myContainer *container, puint32_t part_id) {
    OPC_ASSERT(part_id>=0 && part_id<container->part_items);
    if (part_id!=container->active_write_id) { // we only need to append a new segment, if the current part is not active
        OPC_ASSERT(part_id>=0 && part_id<container->part_items);
        myPart *part=&container->part_array[part_id];
        // create new segment...
        mySegment *segment=ensureSegment(container);
        opc_bzero_mem(segment, sizeof(*segment));
        opc_uint32_t segment_id=container->segment_items++;
        segment->next_segment_id=-1;
        if (-1!=part->last_segment_id) {
            container->segment_array[part->last_segment_id].next_segment_id=segment_id;
        }
        part->last_segment_id=segment_id;
        opc_uint32_t segment_number=part->segment_count++;
        if (-1==part->first_segment_id) part->first_segment_id=part->last_segment_id;
        OPC_ASSERT(part->first_segment_id>=0 && part->first_segment_id<container->segment_items);
        OPC_ASSERT(part->last_segment_id>=0 && part->last_segment_id<container->segment_items);

        // activate part...
        _myContainerFinalizeActiveSegment(container);
        OPC_ASSERT(-1==container->active_write_id);
        container->active_write_id=part_id;

        // ensure a write buffer
        OPC_ASSERT(NULL==container->zip_write_buffer); // not "finalized" why??
        opcZipInitRawState(container->zip, &container->zip_write_state);
        container->zip_write_buffer=opcZipCreateSegment(container->zip, 
                                                        &container->zip_write_state, 
                                                        &segment->zipSegment, 
                                                        part->name, segment_number, 8, OPC_DEFAULT_GROWTH_HINT, 0);
        OPC_ASSERT(NULL!=container->zip_write_buffer);  
    }
    OPC_ASSERT(part_id==container->active_write_id);
    return container->zip_write_state.err;
}

static puint32_t myContainerCreatePart(myContainer *container, xmlChar *name) {
    myPart* part=ensurePart(container);
    if (NULL!=part) {
        opc_bzero_mem(part, sizeof(*part));
        part->name=xmlStrdup(name);
        part->first_segment_id=-1;
        part->last_segment_id=-1;
        return container->part_items++;
    } else {
        return -1;
    }
}


static opc_error_t myContainerWritePart(myContainer *container, opc_uint32_t part_id, opc_uint8_t *data, puint32_t data_len) {
    OPC_ASSERT(part_id>=0 && part_id<container->part_items);
    if (OPC_ERROR_NONE==_myContainerEnsureActiveSegment(container, part_id)) {
        myPart *part=&container->part_array[part_id];
        OPC_ASSERT(NULL!=container->zip_write_buffer);
        OPC_ASSERT(part->first_segment_id>=0 && part->first_segment_id<container->segment_items); // we should habe a valid segment id
        OPC_ASSERT(part->last_segment_id>=0 && part->last_segment_id<container->segment_items); // we should habe a valid segment id
        opc_uint32_t data_ofs=opcZipSegmentBufferWrite(container->zip, 
                                                       &container->zip_write_state, 
                                                       container->zip_write_buffer, 
                                                       data, data_len);
        if (OPC_ERROR_NONE==container->zip_write_state.err && data_ofs<data_len) { // not all data could be written => clearly an error, since segment is active and this should be able to grow!
            container->zip_write_state.err=OPC_ERROR_STREAM;
        }
    }
    return container->zip_write_state.err;
}

static opc_error_t myContainerClosePart(myContainer *container, opc_uint32_t part_id) {
    OPC_ASSERT(part_id>=0 && part_id<container->part_items);
    if (OPC_ERROR_NONE==_myContainerEnsureActiveSegment(container, part_id)) {
        _myContainerFinalizeActiveSegment(container);
    }
    return container->zip_write_state.err;
}


int main( int argc, const char* argv[] )
{
    opc_error_t err=OPC_ERROR_NONE;
    if (OPC_ERROR_NONE==(err=opcInitLibrary())) {
        myContainer container;
        myContainerInit(&container);
        container.zip=opcZipOpenFile(_X("sample.zip"), OPC_ZIP_WRITE);
        if (NULL!=container.zip) {
            puint32_t part_hello_id=myContainerCreatePart(&container, _X("hello.txt"));
            char txt[]="Hello World!\n";
            OPC_ENSURE(OPC_ERROR_NONE==myContainerWritePart(&container, part_hello_id, _X(txt), xmlStrlen(_X(txt))));
            if (1) {
                puint32_t part_id=myContainerCreatePart(&container, _X("sample.txt"));
                char txt[]="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris lectus dui, convallis a rutrum aliquet, sagittis eu neque. Vivamus ipsum lorem, luctus vitae dictum a, dignissim eget sem. Maecenas tincidunt velit sed mi venenatis nec egestas eros elementum. Donec sagittis, eros vitae aliquam bibendum, purus quam gravida erat, ut lacinia turpis sapien non tellus. Nunc sit amet nulla eros. Morbi sed risus vitae eros luctus tristique. Aliquam sed facilisis tellus. Donec purus lorem, vestibulum sed placerat at, molestie eu lorem. Nullam semper felis a dui mollis eu fringilla odio pretium. Vivamus nec facilisis arcu. Maecenas cursus ipsum sit amet felis malesuada adipiscing. Ut a lorem et diam adipiscing vulputate sit amet ut libero. In a nunc metus. Praesent porta metus vel neque tincidunt a gravida ante fermentum. Etiam neque lectus, rutrum et hendrerit in, interdum ac tellus. Aliquam et justo augue. Nam egestas justo at lectus congue ornare. Sed cursus iaculis velit id rhoncus. Aliquam tellus est, mollis nec rutrum eget, auctor a libero. Morbi blandit, odio non aliquam fermentum, augue lacus mattis eros, et bibendum quam nisl eu dolor.\nCras hendrerit, nulla in aliquet convallis, nisi justo porttitor felis, sed dignissim enim purus accumsan libero. Aenean pretium, nisi a auctor rhoncus, orci lectus faucibus tellus, ac mollis sapien ligula quis nibh. Donec faucibus ipsum non velit rhoncus eget rhoncus enim varius. Mauris neque quam, consectetur eu convallis et, sollicitudin sit amet nisi. Praesent sit amet diam ac est pulvinar dapibus. Ut sem ligula, lobortis vel pharetra vitae, venenatis vestibulum metus. Ut eget risus at lorem molestie pellentesque. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Sed non lacus vel sapien viverra faucibus. Nullam tellus nulla, interdum non convallis porttitor, tristique ut tellus. Curabitur sollicitudin tincidunt orci. Vestibulum lacinia tortor nec lacus sollicitudin dictum. Etiam vitae lorem in ligula vehicula pharetra. Sed gravida sapien eget orci laoreet rutrum.\nDonec ut lectus ligula. Vivamus eget massa sapien. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Donec a erat eu turpis scelerisque iaculis. Vestibulum vitae lectus id enim varius fringilla ac ut lectus. Aliquam pretium sollicitudin urna vel adipiscing. Vestibulum vel neque eget lectus sodales sodales ut vitae purus. Phasellus tempus est malesuada sapien ultrices non fringilla nibh bibendum. Donec ac purus ante, id accumsan sapien. Cras quis elit eget sapien tincidunt volutpat ut ut neque. Ut volutpat justo a arcu iaculis et vulputate elit ullamcorper. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Etiam laoreet elit eget mi auctor vehicula vel tempus erat. Nunc facilisis dui nec augue iaculis semper non tempor purus. Quisque orci enim, facilisis vitae venenatis et, fringilla ut nulla. Nulla nec purus ac diam auctor elementum in non sem. Integer tincidunt pellentesque faucibus. Quisque facilisis scelerisque eros in tempus.\nMorbi ornare eros at dui dapibus at dapibus arcu venenatis. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Duis a lacus leo, in aliquet tellus. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Mauris tortor erat, tincidunt quis rhoncus pharetra, faucibus vel odio. Aliquam scelerisque lectus ut nulla varius ac dignissim risus iaculis. Suspendisse vestibulum turpis id tortor porttitor in rutrum lectus vestibulum. In auctor, orci auctor iaculis pharetra, urna orci condimentum mi, et lobortis dui leo non dui. Proin faucibus consectetur placerat. Nunc quis libero risus, vitae ultricies erat. Nullam cursus volutpat sapien nec consectetur. Phasellus quam lectus, accumsan sed porta et, suscipit sed nunc. Phasellus urna sem, consectetur et consectetur at, tempor sed sapien. Donec aliquam felis non sem mattis et egestas ipsum ornare. Maecenas nec leo vel neque pharetra tempus. Quisque tempus auctor enim ut dapibus. Duis tincidunt pulvinar eleifend. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\nDuis blandit, felis id ultrices vulputate, est justo tincidunt libero, ac ullamcorper nisl metus a felis. Nunc commodo tempor orci et fringilla. Quisque nec turpis metus, non faucibus purus. Suspendisse dolor tellus, volutpat a venenatis ac, cursus sed est. Nulla ullamcorper tincidunt venenatis. Morbi vulputate aliquet ligula, congue mattis nibh adipiscing accumsan. Maecenas vitae libero et nisi hendrerit malesuada et pellentesque tortor. Morbi at tellus at odio bibendum venenatis. Nulla non odio nec neque posuere elementum. Suspendisse potenti. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Sed hendrerit, purus a tincidunt dignissim, turpis dolor feugiat tellus, nec tempor magna lacus ac felis.";
                for(int i=0;i<1000;i++) {
                    OPC_ENSURE(OPC_ERROR_NONE==myContainerWritePart(&container, part_id, _X(txt), xmlStrlen(_X(txt))));
                }
                OPC_ENSURE(OPC_ERROR_NONE==myContainerClosePart(&container, part_id));
            }
            OPC_ENSURE(OPC_ERROR_NONE==myContainerWritePart(&container, part_hello_id, _X(txt), xmlStrlen(_X(txt))));
            OPC_ENSURE(OPC_ERROR_NONE==myContainerClosePart(&container, part_hello_id));
            if (1) {
                puint32_t part_id=myContainerCreatePart(&container, _X("[Content_Types].xml"));
                char txt[]="<?xml version=\"1.0\" encoding=\"utf-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"txt\" ContentType=\"text/xml\" /></Types>";
                OPC_ENSURE(OPC_ERROR_NONE==myContainerWritePart(&container, part_id, _X(txt), xmlStrlen(_X(txt))));
                OPC_ENSURE(OPC_ERROR_NONE==myContainerClosePart(&container, part_id));
            }
        }
        myContainerCleanup(&container);
        opcFreeLibrary();
    }
    return (OPC_ERROR_NONE==err?0:3);;
}

