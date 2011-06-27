#include <mce/helper.h>
#include <libxml/xmlmemory.h>

static pbool_t mceQNameLevelLookupEx(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, puint32_t *pos, pbool_t ignore_ln) {
    puint32_t i=0;
    puint32_t j=qname_level_set->list_items;
    while(i<j) {
        puint32_t m=i+(j-i)/2;
        PASSERT(i<=m && m<j);
        mceQNameLevel_t *q2=&qname_level_set->list_array[m];
        int const ns_cmp=(NULL==ns?(NULL==q2->ns?0:-1):(NULL==q2->ns?+1:xmlStrcmp(ns, q2->ns)));
        int const cmp=(ignore_ln?ns_cmp:(0==ns_cmp?xmlStrcmp(ln, q2->ln):ns_cmp));
        if (cmp<0) { j=m; } else if (cmp>0) { i=m+1; } else { *pos=m; return PTRUE; }
    }
    PASSERT(i==j); 
    *pos=i;
    return PFALSE;
}

mceQNameLevel_t* mceQNameLevelLookup(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, pbool_t ignore_ln) {
    puint32_t pos=0;
    return (mceQNameLevelLookupEx(qname_level_set, ns, ln, &pos, ignore_ln)?qname_level_set->list_array+pos:NULL);
}

pbool_t mceQNameLevelAdd(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, puint32_t level) {
    puint32_t i=0;
    pbool_t ret=PFALSE;
    if (!mceQNameLevelLookupEx(qname_level_set, ns, ln, &i, PFALSE)) {
        mceQNameLevel_t *new_list_array=NULL;
        if (NULL!=(new_list_array=(mceQNameLevel_t *)xmlRealloc(qname_level_set->list_array, (1+qname_level_set->list_items)*sizeof(*qname_level_set->list_array)))) {
            qname_level_set->list_array=new_list_array;
            for (puint32_t k=qname_level_set->list_items;k>i;k--) {
                qname_level_set->list_array[k]=qname_level_set->list_array[k-1];
            }
            qname_level_set->list_items++;
            PASSERT(i>=0 && i<qname_level_set->list_items);
            memset(&qname_level_set->list_array[i], 0, sizeof(qname_level_set->list_array[i]));
            qname_level_set->list_array[i].level=level;
            qname_level_set->list_array[i].ln=(NULL!=ln?xmlStrdup(ln):NULL);
            qname_level_set->list_array[i].ns=xmlStrdup(ns);
            if (qname_level_set->max_level<level) qname_level_set->max_level=level;
            ret=PTRUE;
        }
    } else {
        ret=PTRUE;
    }
    return ret;
}

pbool_t mceQNameLevelCleanup(mceQNameLevelSet_t *qname_level_set, puint32_t level) {
    if (qname_level_set->max_level>=level) {
        puint32_t i=0;
        for(puint32_t j=0;j<qname_level_set->list_items;j++) {
            if (qname_level_set->list_array[j].level>=level) {
                PASSERT(qname_level_set->list_array[j].level==level); // cleanup should be called for every level...
                if (NULL!=qname_level_set->list_array[j].ln) xmlFree(qname_level_set->list_array[j].ln);
                if (NULL!=qname_level_set->list_array[j].ns) xmlFree(qname_level_set->list_array[j].ns);
            } else {
                if (qname_level_set->list_array[j].level>qname_level_set->max_level) {
                    qname_level_set->max_level=qname_level_set->list_array[j].level;
                }
                qname_level_set->list_array[i++]=qname_level_set->list_array[j];
            }
        }
        qname_level_set->list_items=i;
    }
    PASSERT(0==level || qname_level_set->max_level<level);
    return PTRUE;
}


pbool_t mceSkipStackPush(mceSkipStack_t *skip_stack, puint32_t level_start, puint32_t level_end, mceSkipState_t state) {
    pbool_t ret=PFALSE;
    mceSkipItem_t *new_stack_array=NULL;
    if (NULL!=(new_stack_array=(mceSkipItem_t *)xmlRealloc(skip_stack->stack_array, (1+skip_stack->stack_items)*sizeof(*skip_stack->stack_array)))) {
        skip_stack->stack_array=new_stack_array;
        memset(&skip_stack->stack_array[skip_stack->stack_items], 0, sizeof(skip_stack->stack_array[skip_stack->stack_items]));
        skip_stack->stack_array[skip_stack->stack_items].level_start=level_start;
        skip_stack->stack_array[skip_stack->stack_items].level_end=level_end;
        skip_stack->stack_array[skip_stack->stack_items].state=state;
        skip_stack->stack_items++;
        ret=PTRUE;
    }
    return ret;
}

void mceSkipStackPop(mceSkipStack_t *skip_stack) {
    PASSERT(skip_stack->stack_items>0);
    skip_stack->stack_items--;
}

mceSkipItem_t *mceSkipStackTop(mceSkipStack_t *skip_stack) {
    return NULL!=skip_stack->stack_array && skip_stack->stack_items>0 ? &skip_stack->stack_array[skip_stack->stack_items-1] : NULL;
}

pbool_t mceSkipStackSkip(mceSkipStack_t *skip_stack, puint32_t level) {
    return NULL!=skip_stack->stack_array && skip_stack->stack_items>0 
        && level>=skip_stack->stack_array[skip_stack->stack_items-1].level_start 
        && level<skip_stack->stack_array[skip_stack->stack_items-1].level_end;
}

pbool_t mceCtxInit(mceCtx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    mceCtxSuspendProcessing(ctx, _X("http://schemas.openxmlformats.org/presentationml/2006/main"), _X("extLst"));
    return PTRUE;
}

pbool_t mceCtxCleanup(mceCtx_t *ctx) {

    PASSERT(ctx->error!=MCE_ERROR_NONE || 0==ctx->ignorable_set.list_items);
    PENSURE(mceQNameLevelCleanup(&ctx->ignorable_set, 0));
    PENSURE(mceQNameLevelCleanup(&ctx->understands_set, 0));
    PASSERT(ctx->error!=MCE_ERROR_NONE || 0==ctx->skip_stack.stack_items);
    while (NULL!=mceSkipStackTop(&ctx->skip_stack)) mceSkipStackPop(&ctx->skip_stack);
    PASSERT(ctx->error!=MCE_ERROR_NONE || 0==ctx->processcontent_set.list_items);
    PENSURE(mceQNameLevelCleanup(&ctx->processcontent_set, 0));
    PENSURE(mceQNameLevelCleanup(&ctx->suspended_set, 0));
    PASSERT(ctx->error!=MCE_ERROR_NONE || 0==ctx->suspended_level);
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
    PENSURE(mceQNameLevelCleanup(&ctx->subsume_namespace_set, 0));
    PENSURE(mceQNameLevelCleanup(&ctx->subsume_exclude_set, 0));
    PENSURE(mceQNameLevelCleanup(&ctx->subsume_prefix_set, 0));
#endif
    
    if (NULL!=ctx->ignorable_set.list_array) xmlFree(ctx->ignorable_set.list_array);
    if (NULL!=ctx->understands_set.list_array) xmlFree(ctx->understands_set.list_array);
    if (NULL!=ctx->skip_stack.stack_array) xmlFree(ctx->skip_stack.stack_array);
    if (NULL!=ctx->processcontent_set.list_array) xmlFree(ctx->processcontent_set.list_array);
    if (NULL!=ctx->suspended_set.list_array) xmlFree(ctx->suspended_set.list_array);
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
    if (NULL!=ctx->subsume_namespace_set.list_array) xmlFree(ctx->subsume_namespace_set.list_array);
    if (NULL!=ctx->subsume_exclude_set.list_array) xmlFree(ctx->subsume_exclude_set.list_array);
    if (NULL!=ctx->subsume_prefix_set.list_array) xmlFree(ctx->subsume_prefix_set.list_array);
#endif
    return PTRUE;
}

pbool_t mceCtxUnderstandsNamespace(mceCtx_t *ctx, const xmlChar *ns) {
    return mceQNameLevelAdd(&ctx->understands_set, ns, NULL, 0);
}

pbool_t mceCtxSuspendProcessing(mceCtx_t *ctx, const xmlChar *ns, const xmlChar *ln) {
    return mceQNameLevelAdd(&ctx->suspended_set, ns, ln, 0);
}


#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
pbool_t mceCtxSubsumeNamespace(mceCtx_t *ctx, const xmlChar *prefix_new, const xmlChar *ns_new, const xmlChar *ns_old) {
    return mceQNameLevelAdd(&ctx->subsume_namespace_set, ns_old, ns_new, 0)
        && mceQNameLevelAdd(&ctx->subsume_prefix_set, ns_new, prefix_new, 0);
}
#endif

