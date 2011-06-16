#include <opc/opc.h>
#include <stdio.h>
#include <time.h>


static pbool_t specialChar(int ch) {
    return ch<'0' || (ch>'9' && ch<'A') || (ch>'Z' && ch<'a' && '_'!=ch) || ch>'z';
}

xmlChar *readSourceLine(FILE *f) {
    xmlChar *ret=NULL;
    int len=0;
    int ch=0;
    while(EOF!=(ch=fgetc(f)) && '\n'!=ch) {
        ret=(xmlChar *)xmlRealloc(ret, len+2); // +2 => reserve trailing "0" too
        if (NULL!=ret) {
            pbool_t ws=(ch==' ' || ch=='\t' || ch=='\r' || ch=='\n');
            if (ws) {
                pbool_t preceeding_ws=(len==0 && ws);
                if (len>0 && !specialChar(ret[len-1])) {
                    ret[len++]=' ';
                }
            } else {
                if (len>0 && specialChar(ch) && ret[len-1]==' ') {
                    len--;
                }
                ret[len++]=ch;
            }
            ret[len]=0;
        }
    }
    if ('\n'==ch && 0==len) {
        ret=(xmlChar *)xmlRealloc(ret, len+1);
        if (NULL!=ret) {
            ret[len]=0;
        }
    }
    return ret;
}

typedef enum LINE_TOKEN_ID {
    LINE_TOKEN_ID_DEF,
    LINE_TOKEN_ID_REF,
    LINE_TOKEN_ID_START_DOCUMENT,
    LINE_TOKEN_ID_END_DOCUMENT,
    LINE_TOKEN_ID_START_ELEMENT,
    LINE_TOKEN_ID_END_ELEMENT,
    LINE_TOKEN_ID_START_CHILDREN,
    LINE_TOKEN_ID_END_CHILDREN,
    LINE_TOKEN_ID_SKIP_CHILDREN,
    LINE_TOKEN_ID_START_ATTRIBUTES,
    LINE_TOKEN_ID_END_ATTRIBUTES,
    LINE_TOKEN_ID_SKIP_ATTRIBUTES,
    LINE_TOKEN_ID_START_CHOICE,
    LINE_TOKEN_ID_END_CHOICE,
    LINE_TOKEN_ID_MATCH_ELEMENT,
    LINE_TOKEN_ID_MATCH_ATTRIBUTE,
    LINE_TOKEN_ID_START_TEXT,
    LINE_TOKEN_ID_END_TEXT,
    LINE_TOKEN_ID_START_ATTRIBUTE,
    LINE_TOKEN_ID_END_ATTRIBUTE,
    LINE_TOKEN_ID_INVALID,
    LINE_TOKEN_ID_EOF
} LineTokenId_t;

typedef struct LINE_TOKEN_DECLARATION {
    LineTokenId_t id;
    char name[50];
} LineTokenDeclaration;

static LineTokenDeclaration s_token[] = {
    { LINE_TOKEN_ID_DEF, "mce_def"}, 
    { LINE_TOKEN_ID_REF, "mce_ref"}, 
    { LINE_TOKEN_ID_START_DOCUMENT, "mce_start_document" },
    { LINE_TOKEN_ID_END_DOCUMENT, "mce_end_document" },
    { LINE_TOKEN_ID_START_ELEMENT, "mce_start_element" },
    { LINE_TOKEN_ID_END_ELEMENT, "mce_end_element" },
    { LINE_TOKEN_ID_START_CHILDREN, "mce_start_children" },
    { LINE_TOKEN_ID_END_CHILDREN, "mce_end_children" },
    { LINE_TOKEN_ID_SKIP_CHILDREN, "mce_skip_children" },
    { LINE_TOKEN_ID_START_ATTRIBUTES, "mce_start_attributes" },
    { LINE_TOKEN_ID_END_ATTRIBUTES, "mce_end_attributes" },
    { LINE_TOKEN_ID_SKIP_ATTRIBUTES, "mce_skip_children" },
    { LINE_TOKEN_ID_START_CHOICE, "mce_start_choice" },
    { LINE_TOKEN_ID_END_CHOICE, "mce_end_choice" },
    { LINE_TOKEN_ID_MATCH_ELEMENT, "mce_match_element" },
    { LINE_TOKEN_ID_MATCH_ATTRIBUTE, "mce_match_attribute" },
    { LINE_TOKEN_ID_START_TEXT, "mce_start_text" },
    { LINE_TOKEN_ID_END_TEXT, "mce_end_text" },
    { LINE_TOKEN_ID_START_ATTRIBUTE, "mce_start_attribute" },
    { LINE_TOKEN_ID_END_ATTRIBUTE, "mce_end_attribute" },
    { LINE_TOKEN_ID_INVALID, "" },
    { LINE_TOKEN_ID_EOF, "" }
};

typedef struct LINE_TOKEN_INSTANCE {
    LineTokenId_t id;
    char param[3][255];
} LineTokenInstance;

typedef struct SOURCE_QNAME {
    xmlChar *p;
    xmlChar *ns;
} SourceQName;


typedef struct SCHEMA_DECLARATION {
    xmlChar *ns;
    xmlChar *ln;
    puint32_t def_id;
} SchemaDeclaration;


typedef struct SCHEMA_DEFINITION {
    xmlChar *name; // if definition has a name
    puint32_t max_occurs;
    SchemaDeclaration *attr_array;
    puint32_t attr_len;
    SchemaDeclaration *decl_array;
    puint32_t decl_len;
} SchemaDefinition;

typedef struct SOURCE_CONTEXT {
    FILE *f;
    SourceQName *binding_array;
    puint32_t binding_len;
    SchemaDefinition *def_array;
    puint32_t def_len;
    SchemaDeclaration *decl_array;
    puint32_t decl_len;
    pbool_t error_flag;
} SourceContext;


static void addBinding(SourceContext *sc, const xmlChar *p, const xmlChar *ns) {
    puint32_t i=0; 
    while(i<sc->binding_len && 0!=xmlStrcmp(sc->binding_array[i].p, p)) i++;
    if (i==sc->binding_len) {
        sc->binding_array=(SourceQName*)xmlRealloc(sc->binding_array, (sc->binding_len+1)*sizeof(SourceQName));
        i=sc->binding_len++;
        memset(sc->binding_array+i, 0, sizeof(sc->binding_array[i]));
        sc->binding_array[i].p=xmlStrdup(p);
    } else {
        OPC_ASSERT(0==xmlStrcmp(sc->binding_array[i].p, p));
    }
    if (NULL!=sc->binding_array && i<sc->binding_len) {
        if (NULL!=sc->binding_array[i].ns) xmlFree(sc->binding_array[i].ns);
        sc->binding_array[i].ns=xmlStrdup(ns);
    }
}

static xmlChar *getBinding(SourceContext *sc, const xmlChar *p) {
    puint32_t i=0; 
    while(i<sc->binding_len && 0!=xmlStrcmp(sc->binding_array[i].p, p)) i++;
    if (i<sc->binding_len) {
        return sc->binding_array[i].ns;
    } else {
        return NULL;
    }
}

static puint32_t addDef(SourceContext *sc, xmlChar *def) {
    sc->def_array=(SchemaDefinition*)xmlRealloc(sc->def_array, (sc->def_len+1)*sizeof(SchemaDefinition));
    if (NULL!=sc->def_array) {
        puint32_t i=sc->def_len++;
        memset(&sc->def_array[i], 0, sizeof(sc->def_array[i]));
        if (NULL!=def) {
            sc->def_array[i].name=xmlStrdup(def);
        }
        return i;
    } else {
        return -1;
    }
}

static puint32_t findDef(SourceContext *sc, xmlChar *def) {
    for(puint32_t i=0;i<sc->def_len;i++) {
        if (NULL!=sc->def_array[i].name && 0==xmlStrcmp(sc->def_array[i].name, def)) {
            return i;
        }
    }
    return -1;
}

static void freeDecls(SchemaDeclaration **decl_array, puint32_t *decl_len);
static void freeDef(SchemaDefinition *def) {
    if (NULL!=def->name) xmlFree(def->name);
    freeDecls(&def->decl_array, &def->decl_len);
}


static SchemaDeclaration* addDecl(SchemaDeclaration **decl_array, puint32_t *decl_len, const xmlChar *ns, const xmlChar *ln) {
    *decl_array=(SchemaDeclaration *)xmlRealloc(*decl_array, (1+*decl_len)*sizeof(SchemaDeclaration));
    if (NULL!=*decl_array) {
        memset((*decl_array)+(*decl_len), 0, sizeof((*decl_array)[(*decl_len)]));
        (*decl_array)[(*decl_len)].def_id=-1;
        if (NULL!=ns) (*decl_array)[(*decl_len)].ns=xmlStrdup(ns);
        if (NULL!=ln) (*decl_array)[(*decl_len)].ln=xmlStrdup(ln);
        return (*decl_array)+(*decl_len)++;
    } else {
        return NULL;
    }
}

static void freeDecls(SchemaDeclaration **decl_array, puint32_t *decl_len) {
    if (NULL!=*decl_array) {
        for(puint32_t j=0;j<*decl_len;j++) {
            if (NULL!=(*decl_array)[j].ln) xmlFree((*decl_array)[j].ln);
            if (NULL!=(*decl_array)[j].ns) xmlFree((*decl_array)[j].ns);
        }
        xmlFree(*decl_array);
    }
}

static void readParam(SourceContext *sc, xmlChar *line, int *ofs, char *param, int param_max) {
    int j=0;
    while(line[*ofs]!=0 && line[*ofs]!=',') {
        if (j+2<param_max) {
            if ('('==line[*ofs]) {
                j=0;
            }
            if ('\"'==line[*ofs] || !specialChar(line[*ofs])) {
                param[j++]=line[*ofs];
            }
            param[j]=0;
        }
        (*ofs)++;
    }
    if (j>1 && '\"'==param[0] && '\"'==param[j-1]) {
        for(int i=0;i<j-2;i++) {
            param[i]=param[i+1];
        }
        param[j-2]=0;
    } else if (NULL!=sc) {
        xmlChar *v=getBinding(sc, _X(param));
        if (NULL!=v && xmlStrlen(v)+1<param_max) {
            strcpy(param, (const char *)v);
        } else {
            printf("ERROR: can not resolve %s\n", param);
        }
    }
}

static int findStringInLine(xmlChar *line, const xmlChar *str) {
    int i=0;
    int str_len=xmlStrlen(str);
    while(0!=line[i] && 0!=xmlStrncmp(line+i, str, str_len)) {
        i++;
    }
    if (0==xmlStrncmp(line+i, str, str_len)) {
        return i;
    } else {
        return -1;
    }
}

LineTokenId_t readLineToken(SourceContext *sc, LineTokenInstance* ti) {
    static const char ns_decl[]="static const char ";
    memset(ti, 0, sizeof(*ti));
    ti->id=LINE_TOKEN_ID_INVALID;
    while(!sc->error_flag) {
        xmlChar *line=readSourceLine(sc->f);
        if (NULL!=line) {
            int ofs=-1;
            int mce_def_ofs=-1;
            if ('#'==line[0] && 'd'==line[1] && 'e'==line[2] && 'f'==line[3] && 'i'==line[4] && 'n'==line[5] && 'e'==line[6]) {
                // filter out #define s
            } else if (-1!=(mce_def_ofs=findStringInLine(line, _X("mce_def ")))) {
                ti->id=LINE_TOKEN_ID_DEF;
                int j=0;
                ofs=mce_def_ofs+xmlStrlen(_X("mce_def "));
                while(line[ofs]!=0 && !specialChar(line[ofs])) {
                    if (j+2<sizeof(ti->param[0])) {
                        ti->param[0][j++]=line[ofs];
                        ti->param[0][j]=0;
                    }
                    ofs++;
                }
            } else if (-1!=(mce_def_ofs=findStringInLine(line, _X("mce_ref(")))) {
                ti->id=LINE_TOKEN_ID_REF;
                int j=0;
                ofs=mce_def_ofs+xmlStrlen(_X("mce_ref("));
                while(line[ofs]!=0 && !specialChar(line[ofs])) {
                    if (j+2<sizeof(ti->param[0])) {
                        ti->param[0][j++]=line[ofs];
                        ti->param[0][j]=0;
                    }
                    ofs++;
                }
            } else {
                if ('m'==line[0] && 'c'==line[1] && 'e'==line[2] && '_'==line[3]) {
                    ofs=0;
                } else if ('}'==line[0] && 'm'==line[1] && 'c'==line[2] && 'e'==line[3] && '_'==line[4]) {
                    ofs=1;
                }
                if (ofs>=0) {
                    int t=0; while (LINE_TOKEN_ID_INVALID!=s_token[t].id && 0!=xmlStrncmp(_X(s_token[t].name), line+ofs, xmlStrlen(_X(s_token[t].name)))) t++;
                    if (LINE_TOKEN_ID_INVALID!=s_token[t].id) {
                        ti->id=s_token[t].id;
                        PASSERT(0==xmlStrncmp(_X(s_token[ti->id].name), line+ofs, xmlStrlen(_X(s_token[ti->id].name))));
                        ofs+=xmlStrlen(_X(s_token[t].name));
                        readParam(NULL, line, &ofs, ti->param[0], sizeof(ti->param[0]));
                        if (line[ofs]==',') { ofs++; readParam(sc, line, &ofs, ti->param[1], sizeof(ti->param[1])); }
                        if (line[ofs]==',') { ofs++; readParam(sc, line, &ofs, ti->param[2], sizeof(ti->param[2])); }
                    } else {
        //                printf("UNKOWN TOKEN: %s\n", line);
                    }
                } else if (0==xmlStrncmp(_X(ns_decl), line, xmlStrlen(_X(ns_decl)))) {
                    char prefix[50];
                    char ns[255];
                    int ofs=xmlStrlen(_X(ns_decl));
                    int j=0;
                    while(0!=line[ofs] && '['!=line[ofs]) {
                        if (j+2<sizeof(prefix)) {
                            prefix[j++]=line[ofs];
                            prefix[j]=0;
                        }
                        ofs++;
                    }
                    if (line[ofs]=='[') {
                        while(0!=line[ofs] && '\"'!=line[ofs]) ofs++;
                        if ('\"'==line[ofs]) {
                            ofs++;
                            int j=0;
                            while(0!=line[ofs] && '\"'!=line[ofs]) {
                                if (j+2<sizeof(ns)) {
                                    ns[j++]=line[ofs];
                                    ns[j]=0;
                                }
                                ofs++;
                            }
                            if ('\"'==line[ofs]) {
                                addBinding(sc, _X(prefix), _X(ns));
                            }
                        }
                    }
                }
            }
            xmlFree(line);
            if (LINE_TOKEN_ID_INVALID!=ti->id) {
                return ti->id;
            }
        } else {
            ti->id=LINE_TOKEN_ID_EOF;
            return ti->id;
        }
    }
    ti->id=LINE_TOKEN_ID_EOF; // error
    return ti->id;
}

static void errorToken(SourceContext *sc, LineTokenInstance *ti) {
    printf("ERROR %s(%s, %s, %s)\n", s_token[ti->id].name, ti->param[0], ti->param[1], ti->param[2]);
    ti->id=LINE_TOKEN_ID_EOF;
    sc->error_flag=PTRUE;
}

static pbool_t parseDeclarations(SourceContext *sc, LineTokenInstance *ti, puint32_t def_id);
static pbool_t parseStartDeclaration(SourceContext *sc, LineTokenInstance *ti, SchemaDeclaration **decl_array, puint32_t *decl_len) {
    if (LINE_TOKEN_ID_START_ELEMENT==ti->id || LINE_TOKEN_ID_START_ATTRIBUTE==ti->id || LINE_TOKEN_ID_START_TEXT==ti->id) {
        LineTokenId_t const end_id=(LINE_TOKEN_ID_START_ELEMENT==ti->id
                                   ?LINE_TOKEN_ID_END_ELEMENT:LINE_TOKEN_ID_START_ATTRIBUTE==ti->id?LINE_TOKEN_ID_END_ATTRIBUTE
                                                                                                   :LINE_TOKEN_ID_START_TEXT==ti->id
                                                                                                   ?LINE_TOKEN_ID_END_TEXT:ti->id);

        SchemaDeclaration *decl=addDecl(decl_array, decl_len, _X(ti->param[1]), _X(ti->param[2]));
        PASSERT(NULL!=decl && -1==decl->def_id);
        readLineToken(sc, ti); // consume start token
        if (LINE_TOKEN_ID_REF==ti->id) {
            decl->def_id=findDef(sc, _X(ti->param[0]));
            if (-1==decl->def_id) {
                decl->def_id=addDef(sc, _X(ti->param[0]));
            }
        } else {
            decl->def_id=addDef(sc, NULL);
        }
        PASSERT(NULL!=decl && -1!=decl->def_id);
        if (decl->def_id>=0 && decl->def_id<sc->def_len) {
            parseDeclarations(sc, ti, decl->def_id); // attributes
            parseDeclarations(sc, ti, decl->def_id); // children
            if (end_id==ti->id) {
                sc->def_array[decl->def_id].max_occurs=PUINT32_MAX;
                readLineToken(sc, ti); // consume end token
                return PTRUE;
            }
        }
    }
    return PFALSE;
}

static pbool_t parseDeclarations(SourceContext *sc, LineTokenInstance *ti, puint32_t def_id) {
    if (LINE_TOKEN_ID_START_CHILDREN==ti->id) {
        readLineToken(sc, ti); // consume start token
        while(parseStartDeclaration(sc, ti, &sc->def_array[def_id].decl_array, &sc->def_array[def_id].decl_len)) {
        }
        if (LINE_TOKEN_ID_END_CHILDREN==ti->id) {
            readLineToken(sc, ti); // consume end token
            return PTRUE;
        } else {
            return PFALSE;
        }
    } else if (LINE_TOKEN_ID_START_ATTRIBUTES==ti->id) {
        readLineToken(sc, ti); // consume start token
        while(parseStartDeclaration(sc, ti, &sc->def_array[def_id].attr_array, &sc->def_array[def_id].attr_len)) {
        }
        if (LINE_TOKEN_ID_END_ATTRIBUTES==ti->id) {
            readLineToken(sc, ti); // consume end token
            return PTRUE;
        } else {
            return PFALSE;
        }
    } else if (LINE_TOKEN_ID_SKIP_CHILDREN==ti->id) {
        readLineToken(sc, ti); // consume token
        return PTRUE;
    } else if (LINE_TOKEN_ID_SKIP_ATTRIBUTES==ti->id) {
        readLineToken(sc, ti); // consume token
        return PTRUE;
    } else if (LINE_TOKEN_ID_REF==ti->id) {
        readLineToken(sc, ti); // consume token
        return PTRUE;
    } else {
        return PFALSE;
    }
}


static pbool_t parseStartDocument(SourceContext *sc, LineTokenInstance *ti) {
    if (LINE_TOKEN_ID_START_DOCUMENT==ti->id) {
        readLineToken(sc, ti); // consume start_document token
        if (parseStartDeclaration(sc, ti, &sc->decl_array, &sc->decl_len)) {
            return (LINE_TOKEN_ID_END_DOCUMENT==ti->id?LINE_TOKEN_ID_INVALID!=readLineToken(sc, ti):PFALSE); // consume end_document
        }
    } 
    return PFALSE;
}

static pbool_t parseDefinition(SourceContext *sc, LineTokenInstance *ti) {
    if (LINE_TOKEN_ID_DEF==ti->id) {
        puint32_t def_id=findDef(sc, _X(ti->param[0]));
        if (-1==def_id) {
            def_id=addDef(sc, _X(ti->param[0]));
        }
        PASSERT(-1!=def_id);
        readLineToken(sc, ti); // consume def token
        if (def_id>=0 && def_id<sc->def_len) {
            parseDeclarations(sc, ti, def_id); // attributes
            parseDeclarations(sc, ti, def_id); // children
            return PTRUE;
        }
    } 
    return PFALSE;
}

static void dumpDecl(SourceContext *sc, FILE *out, int indent, SchemaDeclaration *decl);
static void dumpDef(SourceContext *sc, FILE *out, int indent, puint32_t def_id) {
    printf("%*sattrs:\n", indent, "");
    for(puint32_t i=0;i<sc->def_array[def_id].attr_len;i++) {
        dumpDecl(sc, out, indent, &sc->def_array[def_id].attr_array[i]);
    }
    printf("%*schildren:\n", indent, "");
    for(puint32_t i=0;i<sc->def_array[def_id].decl_len;i++) {
        dumpDecl(sc, out, indent, &sc->def_array[def_id].decl_array[i]);
    }
}


static void dumpDecl(SourceContext *sc, FILE *out, int indent, SchemaDeclaration *decl) {
    pbool_t const has_children=(decl->def_id>=0 && decl->def_id<sc->def_len && NULL==sc->def_array[decl->def_id].name && (sc->def_array[decl->def_id].attr_len>0 || sc->def_array[decl->def_id].decl_len>0));
    fprintf(out, "%*s\"%s\" \"%s\" -> \"%s\" %s\n", indent, "", decl->ns, decl->ln, sc->def_array[decl->def_id].name, (has_children?" {":""));
    if (has_children) {
        dumpDef(sc, out, indent+2, decl->def_id);
        fprintf(out, "%*s}\n", indent, "");
    }
}

void dumpGrammar(SourceContext *sc, FILE *out) {
    for(puint32_t i=0;i<sc->def_len;i++) {
        if (NULL!=sc->def_array[i].name) {
            fprintf(out, "def %s {\n", sc->def_array[i].name);
            dumpDef(sc, out, 2, i);
            fprintf(out, "}\n");
        }
    }
    fprintf(out, "start {\n");
    for(puint32_t i=0;i<sc->decl_len;i++) {
        dumpDecl(sc, out, 2, &sc->decl_array[i]);
    }
    fprintf(out, "}\n");
}

void parseSourceCode(SourceContext *sc, xmlChar *filename) {
    sc->f=fopen((const char*)filename, "r");
    if (NULL!=sc->f) {
        LineTokenInstance ti;
        readLineToken(sc, &ti); // read first token
        while(LINE_TOKEN_ID_EOF!=ti.id) {
            if (parseStartDocument(sc, &ti)) {
            } else if (parseDefinition(sc, &ti)) {
            } else {
                errorToken(sc, &ti);
                readLineToken(sc, &ti); // consumer token, try to recover
            }
        }
        fclose(sc->f);
    }
}

void parseSchema(xmlChar *filename) {
    mceTextReader_t reader;
    if (0==mceTextReaderInit(&reader, xmlNewTextReaderFilename((const char*)filename))) {
        mce_start_document(&reader) {

        } mce_end_document(&reader);
        mceTextReaderCleanup(&reader);
    }
}


int main( int argc, const char* argv[] )
{
    opcInitLibrary();
    SourceContext sc;
    memset(&sc, 0, sizeof(sc));
    addBinding(&sc, _X("NULL"), _X("*"));
    for(int i=1;i<argc;i++) {
        parseSourceCode(&sc, _X(argv[i]));
    }
    dumpGrammar(&sc, stdout);
    if (NULL!=sc.binding_array) {
        for(puint32_t i=0;i<sc.binding_len;i++) {
            if (NULL!=sc.binding_array[i].p) xmlFree(sc.binding_array[i].p);
            if (NULL!=sc.binding_array[i].ns) xmlFree(sc.binding_array[i].ns);
        }
        xmlFree(sc.binding_array);
    }
    freeDecls(&sc.decl_array, &sc.decl_len);
    if (NULL!=sc.def_array) {
        for(puint32_t i=0;i<sc.def_len;i++) {
            freeDef(&sc.def_array[i]);
        }
        xmlFree(sc.def_array);
    }
    opcFreeLibrary();
    return 0;
}
