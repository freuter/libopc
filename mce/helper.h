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
/** @file mce/helper.h
 
 */
#include <mce/config.h>

#ifndef MCE_HELPER_H
#define MCE_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct MCE_QNAME_LEVEL {
        xmlChar *ns;
        xmlChar *ln;
        puint32_t level;
        puint32_t alternatecontent_handled : 1;
    } mceQNameLevel_t;

    typedef struct MCE_QNAME_LEVEL_ARRAY {
        mceQNameLevel_t *list_array;
        puint32_t list_items;
        puint32_t max_level;
    } mceQNameLevelArray_t;

    typedef struct MCE_CONTEXT {
        mceQNameLevelArray_t ignored_array;
        mceQNameLevelArray_t understands_array;
        mceQNameLevelArray_t skip_array;
        mceQNameLevelArray_t processcontent_array;
    } mceCtx_t;

    pbool_t mceQNameLevelAdd(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level);
    mceQNameLevel_t* mceQNameLevelLookup(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, pbool_t ignore_ln);
    pbool_t mceQNameLevelCleanup(mceQNameLevelArray_t *qname_level_array, puint32_t level);
    pbool_t mceQNameLevelPush(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level);
    pbool_t mceQNameLevelPopIfMatch(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level);

    pbool_t mceCtxInit(mceCtx_t *ctx);
    pbool_t mceCtxCleanup(mceCtx_t *ctx);
    pbool_t mceCtxUnderstandsNamespace(mceCtx_t *ctx, const xmlChar *ns);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCE_HELPER_H */
