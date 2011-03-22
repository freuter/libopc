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

opcContainerRelation *opcContainerFindRelationByType(opcContainer *container, opcContainerRelation *relation_array, opc_uint32_t relation_items, const xmlChar *mimeType) {
    for(opc_uint32_t i=0;i<relation_items;i++) {
        if (0==xmlStrcmp(relation_array[i].relation_type, mimeType)) {
            return &relation_array[i];
        }
    }
    return NULL;
}


static opcContainerRelation* _opcRelationFind(opcContainer *container, opcPart part, opcRelation relation) {
    if (OPC_PART_INVALID==part) {
        return opcContainerFindRelation(container, container->relation_array, container->relation_items, relation);
    } else {
        opcContainerPart *cp=opcContainerInsertPart(container, part, OPC_FALSE);
        return (cp!=NULL?opcContainerFindRelation(container, cp->relation_array, cp->relation_items, relation):NULL);
    }
}

opcRelation opcRelationFind(opcContainer *container, opcPart part, const xmlChar *relationId, const xmlChar *mimeType) {
    opcContainerRelation *rel=NULL;
    if (OPC_PART_INVALID==part) {
        if (NULL!=relationId) {
            rel=opcContainerFindRelationById(container, container->relation_array, container->relation_items, relationId);
        } else if (NULL!=mimeType) {
            rel=opcContainerFindRelationByType(container, container->relation_array, container->relation_items, mimeType);
        }
    } else {
        opcContainerPart *cp=opcContainerInsertPart(container, part, OPC_FALSE);
        if (NULL!=cp) {
            if (NULL!=relationId) {
                rel=opcContainerFindRelationById(container, cp->relation_array, cp->relation_items, relationId);
            } else if (NULL!=mimeType) {
                rel=opcContainerFindRelationByType(container, cp->relation_array, cp->relation_items, mimeType);
            }
        }
    }
    return (NULL!=rel?rel->relation_id:OPC_RELATION_INVALID);
}

opcPart opcRelationGetInternalTarget(opcContainer *container, opcPart part, opcRelation relation) {
    opcContainerRelation *rel=_opcRelationFind(container, part, relation);
    if (rel!=NULL && 0==rel->target_mode) {
        return (opcPart)rel->target_ptr;
    } else {
        return NULL;
    }
}

opc_error_t opcRelationRelease(opcContainer *container, opcPart part, opcRelation relation) {
    return OPC_ERROR_NONE;
}

opcRelation opcRelationFirst(opcContainer *container, opcPart part) {
    if (OPC_PART_INVALID==part) {
        return (container->relation_items>0?container->relation_array[0].relation_id:OPC_RELATION_INVALID);
    } else {
        opcContainerPart *cp=opcContainerInsertPart(container, part, OPC_FALSE);
        return (NULL!=cp && cp->relation_items>0?cp->relation_array[0].relation_id:OPC_RELATION_INVALID);
    }
}

opcRelation opcRelationNext(opcContainer *container, opcPart part, opcRelation relation) {
    opcContainerRelation *relation_array=NULL;
    opc_uint32_t relation_items=0;
    if (OPC_PART_INVALID==part) {
        relation_array=container->relation_array;
        relation_items=container->relation_items;
    } else {
        opcContainerPart *cp=opcContainerInsertPart(container, part, OPC_FALSE);
        if (NULL!=cp) {
            relation_array=cp->relation_array;
            relation_items=cp->relation_items;
        }
    }
    if (relation_items>0) {
        opcContainerRelation *rel=opcContainerFindRelation(container, relation_array, relation_items, relation);
        if (rel+1<relation_array+relation_items) {
            return (rel+1)->relation_id;
        } else {
            return OPC_RELATION_INVALID;
        }
    } else {
        return OPC_RELATION_INVALID;
    }
}


void opcRelationGetInformation(opcContainer *container, opcPart part, opcRelation relation, const xmlChar **prefix, opc_uint32_t *counter, const xmlChar **type) {
    opcContainerRelation* rel=NULL;
    if (NULL!=prefix) {
        OPC_ASSERT(OPC_CONTAINER_RELID_PREFIX(relation)>=0 && OPC_CONTAINER_RELID_PREFIX(relation)<container->relprefix_items);
        *prefix=container->relprefix_array[OPC_CONTAINER_RELID_PREFIX(relation)].prefix;
    }
    if (NULL!=counter) {
        *counter=OPC_CONTAINER_RELID_COUNTER(relation);
    }
    
    if (NULL!=type) {
        if (NULL==rel) rel=_opcRelationFind(container, part, relation);
        *type=(NULL!=rel?rel->relation_type:NULL);
    }
}
