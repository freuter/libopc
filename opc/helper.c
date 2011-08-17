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
#include <opc/helper.h>

static puint32_t opcHelperEncodeFilename(const xmlChar *name, char *buf, int buf_len, opc_bool_t rels_segment) {
    int name_len=xmlStrlen(name);
    int len=name_len;
    int buf_ofs=0;
    int rels_ofs=name_len;
    if (rels_segment) {
        while(rels_ofs>0 && name[--rels_ofs]!='/'); // find last "/"
        if (name[rels_ofs]!='/') {
            buf_ofs+=snprintf(buf+buf_ofs, buf_len-buf_ofs, "_rels/");
        }
    }
    const xmlChar *rel_ch=name+rels_ofs;
    int ch=0;
    while(name_len>0 && 0!=(ch=xmlGetUTF8Char(name, &len)) && len>0 && len<=name_len && (NULL==buf || buf_ofs<buf_len)) {
        switch(ch) {
        case '/':
            if (name==rel_ch) {
                buf_ofs+=snprintf(buf+buf_ofs, buf_len-buf_ofs, "/_rels");
            }
        case ':':
        case '@':
        case '-':
        case '.':
        case '_':
        case '~':
        case '!':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case ';':
        case '=':
        case '[': // for internal use only
        case ']': // for internal use only
            if (NULL!=buf && buf_ofs<buf_len) buf[buf_ofs]=ch; buf_ofs++;
            break;
        default:
            if ((ch>='A' && ch<='Z') || (ch>='a' && ch<='z') || (ch>='0' && ch<='9')) {
                if (NULL!=buf) buf[buf_ofs]=ch; buf_ofs++;
            } else if (NULL==buf || (buf_ofs+3<=buf_len && 3==snprintf(buf+buf_ofs, 3, "%%%02X", ch))) {
                buf_ofs+=3;
            } else {
                buf_ofs=0; buf_len=0; // indicate error
            }
        }
        name_len-=len;
        name+=len;
    }
    if (rels_segment) {
        buf_ofs+=snprintf(buf+buf_ofs, buf_len-buf_ofs, ".rels");
    }
    return buf_ofs;
}

static puint32_t opcHelperFilenameAppendPiece(opc_uint32_t segment_number, opc_bool_t last_segment, char *buf, int buf_ofs, int buf_len) {
    if (buf_ofs>0) { // only append to non-empty filenames
        int len=0;
        if (0==segment_number && last_segment) {
            // only one segment, do not append anything...
        } else if (last_segment) {
            OPC_ASSERT(segment_number>0); // we have a last segment
            if (NULL==buf || (buf_ofs<buf_len && (len=snprintf(buf+buf_ofs, buf_len-buf_ofs, "/[%i].last.piece", segment_number))>14)) {
                buf_ofs+=len;
                buf_len-=len;
            }
        } else {
            OPC_ASSERT(!last_segment); // we have a segment with more to come...
            if (NULL==buf || (buf_ofs<buf_len && (len=snprintf(buf+buf_ofs, buf_len-buf_ofs, "/[%i].piece", segment_number))>9)) {
                buf_ofs+=len;
                buf_len-=len;
            }
        }
    }
    return buf_ofs;
}




opc_uint16_t opcHelperAssembleSegmentName(char *out, opc_uint16_t out_size, const xmlChar *name, opc_uint32_t segment_number, opc_uint32_t next_segment_id, opc_bool_t rels_segment, opc_uint16_t *out_max) {
    opc_uint16_t out_length=opcHelperEncodeFilename(name, out, out_size, rels_segment); 
    opc_uint16_t const _out_max=out_length+24; // file+|/[4294967296].last.piece|=file+24
                                               // _rels/file.rels=|_rels/|+|.rels|=6+5=11
    out_length=opcHelperFilenameAppendPiece(segment_number, next_segment_id, out, out_length, out_size);
    OPC_ASSERT(out_length<=out_size);
    OPC_ASSERT(out_length<=_out_max);
    if (NULL!=out_max) *out_max=_out_max;
    return out_length;
}


opc_error_t opcHelperSplitFilename(opc_uint8_t *filename, opc_uint32_t filename_length, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment) {
    opc_error_t ret=OPC_ERROR_STREAM;
    if (NULL!=segment_number) *segment_number=0;
    if (NULL!=last_segment) *last_segment=OPC_TRUE;
    if (NULL!=rel_segment) *rel_segment=OPC_FALSE;
    if (filename_length>7 // "].piece"  suffix
        && filename[filename_length-7]==']'
        && filename[filename_length-6]=='.'
        && filename[filename_length-5]=='p'
        && filename[filename_length-4]=='i'
        && filename[filename_length-3]=='e'
        && filename[filename_length-2]=='c'
        && filename[filename_length-1]=='e') {
        opc_uint32_t i=filename_length-7;
        filename[i--]='\0';
        while(i>0 && filename[i]>='0' && filename[i]<='9') {
            i--;
        }
        if (i>2 && filename[i-2]=='/' && filename[i-1]=='[' && '\0'!=filename[i]) {
            if (NULL!=segment_number) *segment_number=atoi((char*)(filename+i));
            if (NULL!=last_segment) *last_segment=OPC_FALSE;
            filename[i-2]='\0';
            ret=OPC_ERROR_NONE;
        }
    } else if (filename_length>12 // "].last.piece"  suffix
        && filename[filename_length-12]==']'
        && filename[filename_length-11]=='.'
        && filename[filename_length-10]=='l'
        && filename[filename_length-9]=='a'
        && filename[filename_length-8]=='s'
        && filename[filename_length-7]=='t'
        && filename[filename_length-6]=='.'
        && filename[filename_length-5]=='p'
        && filename[filename_length-4]=='i'
        && filename[filename_length-3]=='e'
        && filename[filename_length-2]=='c'
        && filename[filename_length-1]=='e') {
        opc_uint32_t i=filename_length-12;
        filename[i--]='\0';
        while(i>0 && filename[i]>='0' && filename[i]<='9') {
            i--;
        }
        if (i>2 && filename[i-2]=='/' &&  filename[i-1]=='[' && '\0'!=filename[i]) {
            if (NULL!=segment_number) *segment_number=atoi((char*)(filename+i));
            if (NULL!=last_segment) *last_segment=OPC_TRUE;
            filename[i-2]='\0';
            ret=OPC_ERROR_NONE;
        }
    } else if (filename_length>5 // ".rels"  suffix
        && filename[filename_length-5]=='.'
        && filename[filename_length-4]=='r'
        && filename[filename_length-3]=='e'
        && filename[filename_length-2]=='l'
        && filename[filename_length-1]=='s') {
        opc_uint32_t i=filename_length-5;
        while(i>0 && filename[i-1]!='/') i--;
        //"_rels/"
        if (i>=6
            && filename[i-6]=='_' 
            && filename[i-5]=='r' 
            && filename[i-4]=='e' 
            && filename[i-3]=='l' 
            && filename[i-2]=='s' 
            && filename[i-1]=='/') {
            opc_uint32_t j=i;
            for(;j<filename_length-5;j++) {
                filename[j-6]=filename[j];
            }
            filename[j-6]=0;
            if (NULL!=rel_segment) *rel_segment=OPC_TRUE;
        }
        ret=OPC_ERROR_NONE;
    } else {
        ret=OPC_ERROR_NONE;
    }
    return ret;
}
