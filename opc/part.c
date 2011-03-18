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

opcPart opcPartOpen(opcContainer *container, 
                    const xmlChar *absolutePath, 
                    opcType *type,                         
                    int flags) {
    return NULL;
}

const xmlChar *opcPartGetType(opcContainer *c,  opcPart part) {
    OPC_ASSERT(part>=0 && part<c->part_items);
    const xmlChar *type=c->part_array[part].type;
    if (NULL==type) {
        xmlChar *name=c->part_array[part].name;
        int l=(NULL!=name?xmlStrlen(name):0);
        while(l>0 && name[l]!='.') l--;
        if (l>0) { //@TODO has ".rels" the extension "rels", if YES then l>=0
            l++;
            opcContainerExtension *ct=opcContainerInsertExtension(c, name+l, OPC_FALSE);
            type=(NULL!=ct?ct->type:NULL);
        } else {
            type=NULL; // no extension
        }
    }
    return type;
}

int opcPartRelease(opcPart part) {
    return 0;
}

int opcPartDelete(opcContainer *container, const xmlChar *absolutePath) {
    return 0;
}
