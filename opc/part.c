/*
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
#include "internal.h"


static opcPart opcPartOpenEx(opcContainer *container, 
                    const xmlChar *absolutePath, 
                    const xmlChar *type,                         
                    int flags,
                    opc_bool_t create_part) {
    opcContainerPart *part=opcContainerInsertPart(container, (absolutePath[0]=='/'?absolutePath+1:absolutePath), create_part);
    if (NULL!=part) {
        if (create_part && NULL==part->type) {
            opcContainerType *ct=insertType(container, type, OPC_TRUE);
            OPC_ASSERT(NULL!=ct && 0==xmlStrcmp(ct->type, type));
            part->type=ct->type;
        }
        return part->name;
    } else {
        return OPC_PART_INVALID;
    }
}

opcPart opcPartFind(opcContainer *container, 
                    const xmlChar *absolutePath, 
                    const xmlChar *type,
                    int flags) {
    return opcPartOpenEx(container, absolutePath, type, flags, OPC_FALSE);
}

opcPart opcPartCreate(opcContainer *container, 
                    const xmlChar *absolutePath, 
                    const xmlChar *type,
                    int flags) {
    return opcPartOpenEx(container, absolutePath, type, flags, OPC_TRUE);
}

const xmlChar *opcPartGetType(opcContainer *c,  opcPart part) {
    return opcPartGetTypeEx(c, part, OPC_FALSE);
}

const xmlChar *opcPartGetTypeEx(opcContainer *c, opcPart part, opc_bool_t override_only) {
   OPC_ASSERT(OPC_PART_INVALID!=part);
    opcContainerPart *cp=(OPC_PART_INVALID!=part?opcContainerInsertPart(c, part, OPC_FALSE):NULL);
    const xmlChar *type=(NULL!=cp?cp->type:NULL);
    if (NULL==type && NULL!=cp && !override_only) {
        xmlChar *name=cp->name;
        int l=(NULL!=name?xmlStrlen(name):0);
        while(l>0 && name[l]!='.') l--;
        if (l>0) { 
            l++;
            opcContainerExtension *ct=opcContainerInsertExtension(c, name+l, OPC_FALSE);
            type=(NULL!=ct?ct->type:NULL);
        } else {
            type=NULL; // no extension
        }
    }
    return type;
}


opc_error_t opcPartDelete(opcContainer *container, const xmlChar *absolutePath) {
    return opcContainerDeletePart(container, (absolutePath[0]=='/'?absolutePath+1:absolutePath));
}


opcPart opcPartGetFirst(opcContainer *container) {
    return (NULL!=container && container->part_items>0?container->part_array[0].name:OPC_PART_INVALID);
}

opcPart opcPartGetNext(opcContainer *container, opcPart part) {
    opcContainerPart *cp=(NULL!=container?opcContainerInsertPart(container, part, OPC_FALSE):NULL);
    if (NULL!=cp) {
        do { cp++; } while(cp<container->part_array+container->part_items && -1==cp->first_segment_id);
    }
    return (NULL!=cp && cp<container->part_array+container->part_items?cp->name:NULL);
}

opc_ofs_t opcPartGetSize(opcContainer *c, opcPart part) {
   OPC_ASSERT(OPC_PART_INVALID!=part);
    opcContainerPart *cp=(OPC_PART_INVALID!=part?opcContainerInsertPart(c, part, OPC_FALSE):NULL);
    if (NULL!=cp && cp->first_segment_id>=0 && cp->first_segment_id<c->storage->segment_items) {
        return c->storage->segment_array[cp->first_segment_id].uncompressed_size;
    } else {
        return 0;
    }
}
