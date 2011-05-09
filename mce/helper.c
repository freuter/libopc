#include <mce/helper.h>
#include <libxml/xmlmemory.h>

static pbool_t mceQNameLevelLookupEx(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t *pos) {
    puint32_t i=0;
    puint32_t j=qname_level_array->list_items;
    while(i<j) {
        puint32_t m=i+(j-i)/2;
        PASSERT(i<=m && m<j);
        mceQNameLevel_t *q2=&qname_level_array->list_array[m];
        int const ns_cmp=(NULL==ns?(NULL==q2->ns?0:-1):(NULL==q2->ns?+1:xmlStrcmp(ns, q2->ns)));
        int const cmp=(0==ns_cmp?xmlStrcmp(ln, q2->ln):ns_cmp);
        if (cmp<0) { j=m; } else if (cmp>0) { i=m+1; } else { *pos=m; return PTRUE; }
    }
    PASSERT(i==j); 
    *pos=i;
    return PFALSE;
}

mceQNameLevel_t* mceQNameLevelLookup(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln) {
    puint32_t pos=0;
    return (mceQNameLevelLookupEx(qname_level_array, ns, ln, &pos)?qname_level_array->list_array+pos:NULL);

}

pbool_t mceQNameLevelAdd(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level) {
    puint32_t i=0;
    pbool_t ret=PFALSE;
    if (!mceQNameLevelLookupEx(qname_level_array, ns, ln, &i)) {
        mceQNameLevel_t *new_list_array=NULL;
        if (NULL!=(new_list_array=(mceQNameLevel_t *)xmlRealloc(qname_level_array->list_array, (1+qname_level_array->list_items)*sizeof(*qname_level_array->list_array)))) {
            qname_level_array->list_array=new_list_array;
            for (puint32_t k=qname_level_array->list_items;k>i;k--) {
                qname_level_array->list_array[k]=qname_level_array->list_array[k-1];
            }
            qname_level_array->list_items++;
            PASSERT(i>=0 && i<qname_level_array->list_items);
            memset(&qname_level_array->list_array[i], 0, sizeof(qname_level_array->list_array[i]));
            qname_level_array->list_array[i].level=level;
            qname_level_array->list_array[i].ln=(NULL!=ln?xmlStrdup(ln):NULL);
            qname_level_array->list_array[i].ns=xmlStrdup(ns);
            if (qname_level_array->max_level<level) qname_level_array->max_level=level;
            ret=PTRUE;
        }
    } else {
        ret=PTRUE;
    }
    return ret;
}

pbool_t mceQNameLevelCleanup(mceQNameLevelArray_t *qname_level_array, puint32_t level) {
    if (qname_level_array->max_level>=level) {
        puint32_t i=0;
        for(puint32_t j=0;j<qname_level_array->list_items;j++) {
            if (qname_level_array->list_array[j].level>=level) {
                PASSERT(qname_level_array->list_array[j].level==level); // cleanup should be called for every level...
                if (NULL!=qname_level_array->list_array[j].ln) xmlFree(qname_level_array->list_array[j].ln);
                if (NULL!=qname_level_array->list_array[j].ns) xmlFree(qname_level_array->list_array[j].ns);
            } else {
                if (qname_level_array->list_array[j].level>qname_level_array->max_level) {
                    qname_level_array->max_level=qname_level_array->list_array[j].level;
                }
                qname_level_array->list_array[i++]=qname_level_array->list_array[j];
            }
        }
        qname_level_array->list_items=i;
    }
    PASSERT(0==level || qname_level_array->max_level<level);
    return PTRUE;
}


pbool_t mceQNameLevelPush(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level) {
    pbool_t ret=PFALSE;
    mceQNameLevel_t *new_list_array=NULL;
    if (NULL!=(new_list_array=(mceQNameLevel_t *)xmlRealloc(qname_level_array->list_array, (1+qname_level_array->list_items)*sizeof(*qname_level_array->list_array)))) {
        qname_level_array->list_array=new_list_array;
        memset(&qname_level_array->list_array[qname_level_array->list_items], 0, sizeof(qname_level_array->list_array[qname_level_array->list_items]));
        qname_level_array->list_array[qname_level_array->list_items].level=level;
        qname_level_array->list_array[qname_level_array->list_items].ln=(NULL!=ln?xmlStrdup(ln):NULL);
        qname_level_array->list_array[qname_level_array->list_items].ns=(NULL!=ns?xmlStrdup(ns):NULL);
        qname_level_array->list_items++;
        ret=PTRUE;
    }
    return ret;
}

pbool_t mceQNameLevelPopIfMatch(mceQNameLevelArray_t *qname_level_array, const xmlChar *ns, const xmlChar *ln, puint32_t level) {
    pbool_t match=qname_level_array->list_items>0 && qname_level_array->list_array[qname_level_array->list_items-1].level==level;
    if (match) {
        PASSERT(0==xmlStrcmp(qname_level_array->list_array[qname_level_array->list_items-1].ln, ln) 
             && 0==xmlStrcmp(qname_level_array->list_array[qname_level_array->list_items-1].ns, ns));
        PASSERT(qname_level_array->list_items>0);
        if (NULL!=qname_level_array->list_array[qname_level_array->list_items-1].ln) xmlFree(qname_level_array->list_array[qname_level_array->list_items-1].ln);
        if (NULL!=qname_level_array->list_array[qname_level_array->list_items-1].ns) xmlFree(qname_level_array->list_array[qname_level_array->list_items-1].ns);
        qname_level_array->list_items--;
    }
    return match;
}

pbool_t mceCtxInit(mceCtx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    return PTRUE;
}

pbool_t mceCtxCleanup(mceCtx_t *ctx) {

    PASSERT(0==ctx->ignored_array.list_items);
    PENSURE(mceQNameLevelCleanup(&ctx->understands_array, 0));
    PASSERT(0==ctx->skip_array.list_items);
    PASSERT(0==ctx->processcontent_array.list_items);

    if (NULL!=ctx->ignored_array.list_array) xmlFree(ctx->ignored_array.list_array);
    if (NULL!=ctx->understands_array.list_array) xmlFree(ctx->understands_array.list_array);
    if (NULL!=ctx->skip_array.list_array) xmlFree(ctx->skip_array.list_array);
    if (NULL!=ctx->processcontent_array.list_array) xmlFree(ctx->processcontent_array.list_array);

    return PTRUE;
}

pbool_t mceCtxUnderstandsNamespace(mceCtx_t *ctx, const xmlChar *ns) {
    return mceQNameLevelAdd(&ctx->understands_array, ns, NULL, 0);
}

