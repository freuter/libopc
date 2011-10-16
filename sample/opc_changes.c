#include <opc/opc.h>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

typedef void (paragraph_callback_t)(void *callback_ctx, int level, xmlChar *modeTxt, xmlChar *parTxt);

typedef struct CHANGE_MODE {
    opc_bool_t deleted;
    xmlChar *mode;
} changemode_t;

static void initMode(changemode_t *mode) {
    opc_bzero_mem(mode, sizeof(*mode));
}

static void cleanupMode(changemode_t *mode) {
    if (NULL!=mode->mode) {
        xmlFree(mode->mode);
    }
    opc_bzero_mem(mode, sizeof(*mode));
}

typedef struct PARSER_CONTEXT {
    xmlChar *modeTxt;
    opc_bool_t deleted;
    xmlChar *parTxt;
    void *callback_ctx;
    paragraph_callback_t *callback_fct;
} context_t;

static void flush(context_t *ctx, int level) {
    if (NULL!=ctx->callback_fct) ctx->callback_fct(ctx->callback_ctx, level, ctx->modeTxt, ctx->parTxt);
    if (NULL!=ctx->modeTxt) xmlFree(ctx->modeTxt); ctx->modeTxt=NULL;
    if (NULL!=ctx->parTxt) xmlFree(ctx->parTxt); ctx->parTxt=NULL;
}

static void cleanup(context_t *ctx) {
    if (NULL!=ctx->modeTxt) xmlFree(ctx->modeTxt); ctx->modeTxt=NULL;
    if (NULL!=ctx->parTxt) xmlFree(ctx->parTxt); ctx->parTxt=NULL;
}

static void text(context_t *ctx, const xmlChar *text, changemode_t *textMode) {
    if (NULL!=textMode) {
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, textMode->mode);
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, _X(": \""));
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, text);
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, _X("\"\n"));
    }
    if (NULL!=textMode && textMode->deleted) {
        if (!ctx->deleted) {
            ctx->parTxt=xmlStrcat(ctx->parTxt, _X("[]"));
        }
        ctx->deleted=OPC_TRUE;
    } else {
        ctx->parTxt=xmlStrcat(ctx->parTxt, text);
        ctx->deleted=OPC_FALSE;
    }
}

static void par(context_t *ctx, int level, changemode_t *parMode, changemode_t *cellMode, changemode_t *rowMode) {
    if (NULL!=rowMode && NULL!=rowMode->mode) {
        xmlChar *modeTxt=NULL;;
        modeTxt=xmlStrcat(modeTxt, rowMode->mode);
        modeTxt=xmlStrcat(modeTxt, _X(": row mark\n"));
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, modeTxt);
        xmlFree(modeTxt);
    }
    if (NULL!=parMode && NULL!=parMode->mode) {
        xmlChar *modeTxt=NULL;;
        modeTxt=xmlStrcat(modeTxt, parMode->mode);
        modeTxt=xmlStrcat(modeTxt, _X(": paragraph mark\n"));
        ctx->modeTxt=xmlStrcat(ctx->modeTxt, modeTxt);
        xmlFree(modeTxt);
    }
    if (NULL!=parMode &&  parMode->deleted) {
        if (!ctx->deleted) {
            ctx->parTxt=xmlStrcat(ctx->parTxt, _X("[]"));
        }
        ctx->deleted=OPC_TRUE;
    } else {
        ctx->parTxt=xmlStrcat(ctx->parTxt, _X("\n"));
        ctx->deleted=OPC_FALSE;
        flush(ctx, level);
    }
}


static char ns_w[]="http://schemas.openxmlformats.org/wordprocessingml/2006/main";

static void dumpText(context_t *ctx, mceTextReader_t *reader, int level, changemode_t *textMode, changemode_t *parMode, changemode_t *cellMode, changemode_t *rowMode, changemode_t *prop_mode);
static void dumpChildren(context_t *ctx, mceTextReader_t *reader, int level, changemode_t *textMode, changemode_t *parMode, changemode_t *cellMode, changemode_t *rowMode, changemode_t *prop_mode) {
    mce_start_children(reader) {
        mce_match_element(reader, NULL, NULL) {
            dumpText(ctx, reader, level, textMode, parMode, cellMode, rowMode, prop_mode);
        }
        mce_match_text(reader) {
            dumpText(ctx, reader, level, textMode, parMode, cellMode, rowMode, prop_mode);
        }
    } mce_end_children(reader);
}


static void dumpText(context_t *ctx, mceTextReader_t *reader, int level, changemode_t *textMode, changemode_t *parMode, changemode_t *cellMode, changemode_t *rowMode, changemode_t *prop_mode) {
    mce_start_choice(reader) {
        mce_start_element(reader, _X(ns_w), _X("t")) {
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_start_text(reader) {
                    text(ctx, xmlTextReaderConstValue(reader->reader), textMode);
                } mce_end_text(reader);
            } mce_end_children(reader);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("delText")) {
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_start_text(reader) {
                    OPC_ASSERT(NULL!=textMode && textMode->deleted);
                    text(ctx, xmlTextReaderConstValue(reader->reader), textMode);
                } mce_end_text(reader);
            } mce_end_children(reader);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("ins")) {
            changemode_t ins_props;
            initMode(&ins_props);
            ins_props.deleted=0;
            ins_props.mode=xmlStrdup(_X("Insertion by "));
            mce_start_attributes(reader) {
                mce_start_attribute(reader, _X(ns_w), _X("author")) {
                    ins_props.mode=xmlStrcat(ins_props.mode, xmlTextReaderConstValue(reader->reader));
                } mce_end_attribute(reader);
            } mce_end_attributes(reader);
            if (NULL!=prop_mode) {
                prop_mode->deleted=ins_props.deleted;
                prop_mode->mode=xmlStrdup(ins_props.mode);
            }
            dumpChildren(ctx, reader, level, &ins_props, parMode, cellMode, rowMode, prop_mode);
            cleanupMode(&ins_props);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("moveTo")) {
            changemode_t ins_props;
            initMode(&ins_props);
            ins_props.deleted=0;
            ins_props.mode=xmlStrdup(_X("Insertion by "));
            mce_start_attributes(reader) {
                mce_start_attribute(reader, _X(ns_w), _X("author")) {
                    ins_props.mode=xmlStrcat(ins_props.mode, xmlTextReaderConstValue(reader->reader));
                } mce_end_attribute(reader);
            } mce_end_attributes(reader);
            if (NULL!=prop_mode) {
                prop_mode->deleted=ins_props.deleted;
                prop_mode->mode=xmlStrdup(ins_props.mode);
            }
            dumpChildren(ctx, reader, level, &ins_props, parMode, cellMode, rowMode, prop_mode);
            cleanupMode(&ins_props);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("del")) {
            changemode_t del_props;
            initMode(&del_props);
            del_props.deleted=1;
            del_props.mode=xmlStrdup(_X("Deletion by "));
            mce_start_attributes(reader) {
                mce_start_attribute(reader, _X(ns_w), _X("author")) {
                    del_props.mode=xmlStrcat(del_props.mode, xmlTextReaderConstValue(reader->reader));
                } mce_end_attribute(reader);
            } mce_end_attributes(reader);
            if (NULL!=prop_mode) {
                prop_mode->deleted=del_props.deleted;
                prop_mode->mode=xmlStrdup(del_props.mode);
            }
            dumpChildren(ctx, reader, level, &del_props, parMode, cellMode, rowMode, prop_mode);
            cleanupMode(&del_props);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("moveFrom")) {
            changemode_t del_props;
            initMode(&del_props);
            del_props.deleted=1;
            del_props.mode=xmlStrdup(_X("Deletion by "));
            mce_start_attributes(reader) {
                mce_start_attribute(reader, _X(ns_w), _X("author")) {
                    del_props.mode=xmlStrcat(del_props.mode, xmlTextReaderConstValue(reader->reader));
                } mce_end_attribute(reader);
            } mce_end_attributes(reader);
            if (NULL!=prop_mode) {
                prop_mode->deleted=del_props.deleted;
                prop_mode->mode=xmlStrdup(del_props.mode);
            }
            dumpChildren(ctx, reader, level, &del_props, parMode, cellMode, rowMode, prop_mode);
            cleanupMode(&del_props);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("p")) {
            changemode_t p_props;
            initMode(&p_props);
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_match_element(reader, _X(ns_w), _X("pPr")) {
                    dumpText(ctx, reader, level, textMode, parMode, cellMode, rowMode, &p_props);
                };
                mce_match_element(reader, NULL, NULL) {
                    dumpText(ctx, reader, level, textMode, &p_props, cellMode, rowMode, NULL);
                };
            } mce_end_children(reader);
            par(ctx, level, &p_props, cellMode, rowMode);
            cleanupMode(&p_props);
        } mce_end_element(reader);
        mce_start_element(reader, _X(ns_w), _X("tr")) {
            changemode_t tr_props;
            initMode(&tr_props);
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_match_element(reader, _X(ns_w), _X("trPr")) {
                    dumpText(ctx, reader, level+1, textMode, parMode, cellMode, rowMode, &tr_props);
                };
                mce_match_element(reader, NULL, NULL) {
                    dumpText(ctx, reader, level+1, textMode, parMode, cellMode, &tr_props, NULL);
                };
            } mce_end_children(reader);
            cleanupMode(&tr_props);
        } mce_end_element(reader);
        mce_start_element(reader, NULL, NULL) {
            mce_skip_attributes(reader);
            dumpChildren(ctx, reader, level, textMode, parMode, cellMode, rowMode, prop_mode);
        } mce_end_element(reader);
        mce_start_text(reader) {
        } mce_end_text(reader);
    } mce_end_choice(reader);
}

void parseText(xmlChar *filename, paragraph_callback_t *callback_fct, void *callback_ctx) {
    opcContainer *c=opcContainerOpen(filename, OPC_OPEN_READ_ONLY, NULL, NULL);
    if (NULL!=c) {
        opcRelation rel=opcRelationFind(c, OPC_PART_INVALID, NULL, _X("http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"));
        if (OPC_RELATION_INVALID!=rel) {
            opcPart main=opcRelationGetInternalTarget(c, OPC_PART_INVALID, rel);
            if (OPC_PART_INVALID!=main) {
                const xmlChar *type=opcPartGetType(c, main);
                if (0==xmlStrcmp(type, _X("application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"))) {
                    mceTextReader_t reader;
                    if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &reader, main, NULL, 0, 0)) {
                        context_t ctx;
                        opc_bzero_mem(&ctx, sizeof(ctx));
                        ctx.callback_fct=callback_fct;
                        ctx.callback_ctx=callback_ctx;
                        mce_start_document(&reader) {
                            mce_match_element(&reader, NULL, NULL) {
                                dumpText(&ctx, &reader, 0, NULL, NULL, NULL, NULL, NULL);
                            };
                        } mce_end_document(&reader);
                        flush(&ctx, 0);
                        cleanup(&ctx);
                    }
                    mceTextReaderCleanup(&reader);
                }
            }
        }
        opcContainerClose(c, OPC_CLOSE_NOW);
    }
}

static void paragraph_callback(void *callback_ctx, int level, xmlChar *modeTxt, xmlChar *parTxt) {
    if (NULL!=modeTxt) {
        fputs((const char *)modeTxt, (FILE*)callback_ctx);
    }
    if (NULL!=parTxt) {
        fputs((const char *)parTxt, (FILE*)callback_ctx);
    }
}

int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    opcInitLibrary();
    parseText(_X(argv[1]), paragraph_callback, stdout);
    opcFreeLibrary();
    return 0;
}
