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
/** @file opc/relation.h
 
 */
#include <opc/config.h>

#ifndef OPC_RELATION_H
#define OPC_RELATION_H

#ifdef __cplusplus
extern "C" {
#endif    

    typedef opc_uint32_t opcRelation;
#define OPC_RELATION_INVALID (-1)

    opcRelation opcRelationCreate(opcContainer *container, opcPart part, const xmlChar *relationId, const xmlChar *mimeType);
    opc_error_t opcRelationRelease(opcContainer *container, opcPart part, opcRelation relation);
    int opcRelationDelete(opcContainer *container, opcPart part, const xmlChar *relationId);

    opcRelation opcRelationFirst(opcContainer *container, opcPart part);
    opcRelation opcRelationNext(opcContainer *container, opcPart part, opcRelation relation);
    
    opcPart opcRelationGetInternalTarget(opcContainer *container, opcPart part, opcRelation relation);
    const xmlChar *opcRelationGetExternalTarget(opcContainer *container, opcPart part, opcRelation relation);
    const xmlChar *opcRelationGetType(opcContainer *container, opcPart part, opcRelation relation);

    opcRelation opcRelationFind(opcContainer *container, opcPart part, const xmlChar *relationId, const xmlChar *mimeType);

    void opcRelationGetInformation(opcContainer *container, opcPart part, opcRelation relation, const xmlChar **prefix, opc_uint32_t *counter, const xmlChar **type);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_RELATION_H */
