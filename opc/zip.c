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
#include <libxml/xmlmemory.h>
#include <libxml/globals.h>
#include <string.h>
#include <stdio.h>
#define OPC_MAXPATH 512
#include "internal.h"


static inline opc_uint32_t _opcZipFileRead(opcZip *zip, opc_uint8_t *buf, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=zip && zip->_ioread!=NULL && NULL!=buf);
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==zip->state.err) {
        int len=zip->_ioread(zip->iocontext, (char *)buf, buf_len);
        if (ret<0) {
            zip->state.err=OPC_ERROR_STREAM;
        } else {
            ret=(opc_uint32_t)len;
            zip->state.buf_pos+=ret;
        }
    }
    return ret;
}

static inline opc_uint32_t _opcZipFileWrite(opcZip *zip, const opc_uint8_t *buf, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=zip && zip->_iowrite!=NULL && NULL!=buf);
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==zip->state.err) {
        int len=zip->_iowrite(zip->iocontext, (const char *)buf, buf_len);
        if (ret<0) {
            zip->state.err=OPC_ERROR_STREAM;
        } else {
            ret=(opc_uint32_t)len;
            zip->state.buf_pos+=ret;
            if (zip->state.buf_pos>zip->file_size) zip->file_size=zip->state.buf_pos;
        }
    }
    return ret;
}

static inline opc_error_t _opcZipFileClose(opcZip *zip) {
    OPC_ASSERT(NULL!=zip && zip->_ioclose!=NULL);
    opc_uint32_t ret=0;
    if (0!=zip->_ioclose(zip->iocontext)) {
        if (OPC_ERROR_NONE==zip->state.err) {
            zip->state.err=OPC_ERROR_STREAM;
        }
    }
    return zip->state.err;
}

static inline opc_ofs_t _opcZipFileSeek(opcZip *zip, opc_ofs_t ofs, opcZipSeekMode whence) {
    OPC_ASSERT(NULL!=zip && (opcZipSeekSet==whence || opcZipSeekCur==whence || opcZipSeekEnd==whence));
    opc_ofs_t abs=zip->state.buf_pos;
    if (OPC_ERROR_NONE==zip->state.err) {
        switch (whence) {
        case opcZipSeekSet: abs=ofs; break;
        case opcZipSeekCur: abs+=ofs; break;
        case opcZipSeekEnd: abs=zip->file_size-ofs; break;
        }
        if (abs<0 || abs>=zip->file_size) {
            zip->state.err=OPC_ERROR_STREAM;
        } else if (zip->state.buf_pos!=abs) {
            if (NULL!=zip->_ioseek) {
                int _ofs=zip->_ioseek(zip->iocontext, abs);
                if (_ofs!=abs) {
                    zip->state.err=OPC_ERROR_STREAM;
                } else {
                    zip->state.buf_pos=abs;
                }
            } else {
                zip->state.err=OPC_ERROR_SEEK;
            }
        }
    }
    return abs;
}

static inline opc_error_t _opcZipFileGrow(opcZip *zip, opc_uint32_t abs) {
    if (OPC_ERROR_NONE==zip->state.err) {
        if (abs>zip->file_size) {
            zip->file_size=abs; //@TODO add error handling here if e.g. file can not grow because disk is full etc...
        }
    }
    return zip->state.err;
}

static inline opc_error_t _opcZipFileSeekRawState(opcZip *zip, opcZipRawState *rawState, puint32_t new_ofs) {
    if (OPC_ERROR_NONE==rawState->err) {
        rawState->err=zip->state.err;
    }
    if (OPC_ERROR_NONE==rawState->err) {
        if (_opcZipFileSeek(zip, new_ofs, opcZipSeekSet)!=new_ofs) {
            *rawState=zip->state; // indicate an error
        } else {
            rawState->buf_pos=new_ofs; // all OK
        }
    }
    return rawState->err;
}


static int __opcZipFileRead(void *iocontext, char *buffer, int len) {
    return fread(buffer, sizeof(char), len, (FILE*)iocontext);
}

static int __opcZipFileWrite(void *iocontext, const char *buffer, int len) {
    return fwrite(buffer, sizeof(char), len, (FILE*)iocontext);
}

static int __opcZipFileClose(void *iocontext) {
    return fclose((FILE*)iocontext);
}

static opc_ofs_t __opcZipFileSeek(void *iocontext, opc_ofs_t ofs) {
    int ret=fseek((FILE*)iocontext, ofs, SEEK_SET);
    if (ret>=0) {
        return ftell((FILE*)iocontext);
    } else {
        return ret;
    }
}

static opc_uint32_t __opcZipFileLength(void *iocontext) {
    opc_ofs_t current=ftell((FILE*)iocontext);
    OPC_ENSURE(fseek((FILE*)iocontext, 0, SEEK_END)>=0);
    opc_ofs_t length=ftell((FILE*)iocontext);
    OPC_ENSURE(fseek((FILE*)iocontext, current, SEEK_SET)>=0);
    OPC_ASSERT(current==ftell((FILE*)iocontext));
    return length;
}

opcZip *opcZipOpenFile(const xmlChar *fileName, int flags) {
    opcZip *ret=NULL;
    char mode[4];
    int mode_ofs=0;
    if (flags & OPC_ZIP_READ) {
        mode[mode_ofs++]='r';
    }
    if (flags & OPC_ZIP_WRITE) {
        mode[mode_ofs++]='w';
    }
    mode[mode_ofs++]='b';
    mode[mode_ofs++]='\0';
    FILE *file=fopen(_X2C(fileName), mode);
    if (file!=NULL) {
        ret=opcZipOpenIO(__opcZipFileRead, __opcZipFileWrite, __opcZipFileClose, __opcZipFileSeek, file, __opcZipFileLength(file), flags);
    }
    return ret;
}

opcZip *opcZipOpenIO(opcZipReadCallback *ioread,
                     opcZipWriteCallback *iowrite,
                     opcZipCloseCallback *ioclose,
                     opcZipSeekCallback *ioseek,
                     void *iocontext,
                     pofs_t file_size,
                     int flags) {
    opcZip *zip=(opcZip*)xmlMalloc(sizeof(opcZip));
    memset(zip, 0, sizeof(*zip));
    zip->_ioread=ioread;
    zip->_iowrite=iowrite;
    zip->_ioclose=ioclose;
    zip->_ioseek=ioseek;
    zip->iocontext=iocontext;
    zip->file_size=file_size;
    zip->flags=flags;
    zip->append_ofs=0;
    return zip;
}

void opcZipClose(opcZip *zip) {
    if (NULL!=zip) {
        OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileClose(zip));
        xmlFree(zip);
    }
}

static inline opc_uint32_t opcZipRawWrite(opcZip *zip, opcZipRawState *raw, const opc_uint8_t *buffer, opc_uint32_t len) {
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==raw->err) {
        if (OPC_ERROR_NONE==zip->state.err) {
            if (raw->buf_pos==zip->state.buf_pos) {
                ret=_opcZipFileWrite(zip, buffer, len);
                *raw=zip->state;
            } else {
                raw->err=OPC_ERROR_SEEK;
            }
        } else {
            *raw=zip->state;
        }
    }
    return ret;
}

opc_error_t opcZipInitRawState(opcZip *zip, opcZipRawState *rawState) {
    *rawState=zip->state;
    return rawState->err;
}

opc_error_t opcZipInitRawBuffer(opcZip *zip, opcZipRawBuffer *rawBuffer) {
    opc_bzero_mem(rawBuffer, sizeof(*rawBuffer));
    return opcZipInitRawState(zip, &rawBuffer->state);;
}


static inline int opcZipRawWriteU8(opcZip *zip, opcZipRawState *raw, opc_uint8_t val) {
    return opcZipRawWrite(zip, raw, (const opc_uint8_t *)&val, sizeof(opc_uint8_t));
}

static inline int opcZipRawWrite8(opcZip *zip, opcZipRawState *raw, opc_int8_t val) {
    return opcZipRawWriteU8(zip, raw, (opc_uint8_t)val);
}

static inline int opcZipRawWriteZero(opcZip *zip, opcZipRawState *raw, opc_uint32_t count) {
    int ret=0;
    opc_uint8_t val=0;
    //@TODO may speed me up a bit (using fappend...)
    for(opc_uint32_t i=0;i<count;i++) {
        ret+=opcZipRawWriteU8(zip, raw, val);
    }
    return ret;
}


static inline int opcZipRawWriteU16(opcZip *zip, opcZipRawState *raw, opc_uint16_t val) {
    int i=0;
    while(OPC_ERROR_NONE==raw->err && i<sizeof(val) && 1==opcZipRawWriteU8(zip, raw, (opc_uint8_t)(((unsigned)val)>>(i<<3)))) {
        i++;
    }
    return i;
}

static inline int opcZipRawWriteU32(opcZip *zip, opcZipRawState *raw, opc_uint32_t val) {
    int i=0;
    while(OPC_ERROR_NONE==raw->err && i<sizeof(val) && 1==opcZipRawWriteU8(zip, raw, (opc_uint8_t)(((unsigned)val)>>(i<<3)))) {
        i++;
    }
    return i;
}

static puint32_t opcZipEncodeFilename(const xmlChar *name, char *buf, int buf_len) {
    int name_len=xmlStrlen(name);
    int len=name_len;
    int buf_ofs=0;
    int ch=0;
    while(name_len>0 && 0!=(ch=xmlGetUTF8Char(name, &len)) && len>0 && len<=name_len && (NULL==buf || buf_ofs<buf_len)) {
        switch(ch) {
        case '/':
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
            if (NULL!=buf) buf[buf_ofs]=ch; buf_ofs++;
            break;
        default:
            if ((ch>='A' && ch<='Z') || (ch>='a' && ch<='z') || (ch>='0' && ch<='9')) {
                if (NULL!=buf) buf[buf_ofs]=ch; buf_ofs++;
            } else if (NULL==buf || (buf_len+3<=buf_len && 3==snprintf(buf+buf_ofs, 3, "%%%02X", ch))) {
                buf_ofs+=3;
            } else {
                buf_ofs=0; buf_len=0; // indicate error
            }
        }
        name_len-=len;
        name+=len;
    }
    return buf_ofs;
}

static puint32_t opcZipFilenameAppendPiece(opc_uint32_t segment_number, opc_bool_t last_segment, char *buf, int buf_ofs, int buf_len) {
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


#define OPCZIPSEGMENTHEADERSIZE(filename_length, padding) (4*4+7*2+(filename_length)+4*2+(padding)) // file_header+filename_length+growth_hint_extra_header
#define OPCZIPSEGMENTTOTALSIZE(filename_length, segment_size) ((segment_size)+OPCZIPSEGMENTHEADERSIZE(filename_length, 0)) // header+ segment_size

struct OPC_ZIPSEGMENTBUFFER_STRUCT {
    opc_uint16_t compression_method;
    opc_uint16_t state_header_written : 1;
    opc_uint16_t state_space_reserved : 1;
    opc_uint16_t state_data_written : 1;
    opc_uint32_t segment_size;
    opc_uint32_t crc32;
    z_stream stream;
    int inflate_state;
    puint32_t   buf_len;
    puint32_t   buf_ofs;
    opc_uint8_t *buf /*[segment_size]*/;
};
#define OPC_ZIPSEGMENTBUFFER_DATA(buffer) ((opc_uint8_t*)(&buffer->buf_len+sizeof(buffer->buf_len)))

static opc_uint32_t opcZipWriteSegmentHeader(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, xmlChar *name, opc_uint32_t segment_number, opc_uint32_t next_segment_id) {
    char filename[OPC_MAXPATH];
    opc_uint16_t filename_length=opcZipEncodeFilename(name, filename, sizeof(filename)); 
    opc_uint16_t filename_max=filename_length+24; // file+|/[4294967296].last.piece|=file+24
    filename_length=opcZipFilenameAppendPiece(segment_number, next_segment_id, filename, filename_length, sizeof(filename));
    OPC_ASSERT(filename_length<=filename_max);
    if (OPC_ERROR_NONE==rawState->err && 0==filename_length) {
        rawState->err=OPC_ERROR_STREAM;
    }
    opc_uint16_t padding=filename_max-filename_length; // this will ensure that we can rewrite the header with a differenct piece suffix at any time...
    opc_uint32_t ret=0;
    if ((4==(ret+=opcZipRawWriteU32(zip, rawState, 0x04034b50)))
    && (6==(ret+=opcZipRawWriteU16(zip, rawState, 20)))  //version needed to extract
    && (8==(ret+=opcZipRawWriteU16(zip, rawState, segment->bit_flag)))  // bit flag
    && (10==(ret+=opcZipRawWriteU16(zip, rawState, segment->compression_method)))  // compression method
    && (12==(ret+=opcZipRawWriteU16(zip, rawState, 0x0)))  // last mod file time
    && (14==(ret+=opcZipRawWriteU16(zip, rawState, 0x0)))  // last mod file date
    && (18==(ret+=opcZipRawWriteU32(zip, rawState, segment->crc32)))  // crc32
    && (22==(ret+=opcZipRawWriteU32(zip, rawState, segment->compressed_size)))  // compressed size
    && (26==(ret+=opcZipRawWriteU32(zip, rawState, segment->uncompressed_size)))  // uncompressed size
    && (28==(ret+=opcZipRawWriteU16(zip, rawState, filename_length)))  // filename length
    && (30==(ret+=opcZipRawWriteU16(zip, rawState, 4*2+padding)))  // extra length
    && (30+filename_length==(ret+=opcZipRawWrite(zip, rawState, (opc_uint8_t *)filename, filename_length)))
    && (32+filename_length==(ret+=opcZipRawWriteU16(zip, rawState, 0xa220)))  // extra: Microsoft Open Packaging Growth Hint
    && (34+filename_length==(ret+=opcZipRawWriteU16(zip, rawState, 2+2+padding)))  // extra: size of Sig + PadVal + Padding
    && (36+filename_length==(ret+=opcZipRawWriteU16(zip, rawState, 0xa028)))  // extra: verification signature (A028)
    && (38+filename_length==(ret+=opcZipRawWriteU16(zip, rawState, segment->growth_hint)))  // extra: Initial padding value
    && (38+filename_length+padding==(ret+=opcZipRawWriteZero(zip, rawState, padding)))) { // extra: filled with NULL characters
        OPC_ASSERT(4*4+11*2+filename_length+padding==ret);
    }
    return (4*4+11*2+filename_length+padding==ret?ret:0);
}

static opc_error_t opcZipFlushSegmentBuffer(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer) {
    if (OPC_ERROR_NONE==rawState->err) {
        opc_uint32_t len=0;
        while (buffer->buf_len>0 && (len=opcZipRawWrite(zip, rawState, buffer->buf+buffer->buf_ofs, buffer->buf_len))>0 && len<=buffer->buf_len) {
            buffer->buf_ofs+=len;
            buffer->buf_len-=len;
        }
        if (0==buffer->buf_len) {
            buffer->buf_ofs=0; // reset, so that the buffer can be filled again..
        } else {
            rawState->err=OPC_ERROR_STREAM;
        }
    }
    return rawState->err;
}



opcZipSegmentBuffer* opcZipCreateSegment(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, xmlChar *name, opc_uint32_t segment_number, opc_uint16_t compression_method, opc_uint32_t growth_hint, opc_uint32_t max_size) {
    OPC_ASSERT(max_size>=0); // currently unsused. but may be important for future performance optimizations.
    opc_bzero_mem(segment, sizeof(*segment));
    segment->growth_hint=(growth_hint>0?growth_hint:OPC_DEFAULT_GROWTH_HINT);
    segment->stream_ofs=rawState->buf_pos;
    segment->compression_method=compression_method;
    if (OPC_ERROR_NONE==rawState->err && 0!=segment->compression_method && 8!=segment->compression_method) {
        rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
    }
    segment->bit_flag=6; // BEST-SPEED
    opcZipSegmentBuffer* buffer=NULL;
    opc_uint32_t segment_size=(max_size>0?max_size:segment->growth_hint);
    //@TODO close "active" segment..
    if (NULL!=zip->_ioseek // when we are not in streaming mode: write header and reserve space...
     && 0!=(segment->header_size=opcZipWriteSegmentHeader(zip, rawState, segment, name, segment_number, OPC_FALSE)) 
     && OPC_ERROR_NONE==_opcZipFileGrow(zip, rawState->buf_pos+segment_size)) {
        buffer=(opcZipSegmentBuffer*)xmlMalloc(sizeof(*buffer));
        if (NULL!=buffer) {
            opc_bzero_mem(buffer, sizeof(*buffer));
            buffer->segment_size=segment_size;
            buffer->state_header_written=1;
            buffer->state_space_reserved=1;
            buffer->compression_method=segment->compression_method;
            OPC_ASSERT(0==buffer->compression_method || 8==buffer->compression_method);
            if (8==buffer->compression_method) { // delfate
                buffer->stream.zalloc = Z_NULL;
                buffer->stream.zfree = Z_NULL;
                buffer->stream.opaque = Z_NULL;
                if (Z_OK!=(buffer->inflate_state=deflateInit2(&buffer->stream, Z_BEST_SPEED, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY))) {
                    rawState->err=OPC_ERROR_DEFLATE;
                }
            }
            buffer->buf=(opc_uint8_t *)xmlMalloc(segment_size);
            if (NULL==buffer->buf) {
                xmlFree(buffer); buffer=NULL;
            }
        }
    }
    return buffer;
}


opc_error_t opcZipUpdateHeader(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, xmlChar *name, opc_uint32_t segment_number, opc_bool_t last_segment) {
    //@TODO in stream mode simply why an empty "[x].last.piece".
    puint32_t old_ofs=rawState->buf_pos;
    OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileSeekRawState(zip, rawState, segment->stream_ofs));
    if (OPC_ERROR_NONE!=rawState->err || opcZipWriteSegmentHeader(zip, rawState, segment, name, segment_number, last_segment)!=segment->header_size) {
        OPC_ASSERT(OPC_ERROR_NONE!=rawState->err);
    }
    OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileSeekRawState(zip, rawState, old_ofs));
    return rawState->err;
}

opcZipSegmentBuffer* opcZipFinalizeSegment(opcZip *zip, opcZipRawState *rawState, opcZipSegment *segment, opcZipSegmentBuffer* buffer) {
    if (OPC_ERROR_NONE==rawState->err) {
        if (!buffer->state_header_written) {
            //@TODO write header & flush data
            OPC_ASSERT(0);; //@TODO implement me!
        } else if (NULL!=zip->_ioseek) {
            OPC_ASSERT(buffer->state_header_written);
            // flush data
            if (OPC_ERROR_NONE==opcZipFlushSegmentBuffer(zip, rawState, buffer)) {
                // write updated header
                segment->uncompressed_size=buffer->stream.total_in;
                segment->compressed_size=buffer->stream.total_out;
                segment->crc32=buffer->crc32;
#if 0
                puint32_t old_ofs=rawState->buf_pos;
                OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileSeekRawState(zip, rawState, segment->stream_ofs));
                if (OPC_ERROR_NONE!=rawState->err || opcZipWriteSegmentHeader(zip, rawState, segment, name, segment_number, last_segment)!=segment->header_size) {
                    OPC_ASSERT(OPC_ERROR_NONE!=rawState->err);
                }
                OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileSeekRawState(zip, rawState, old_ofs));
#endif
            }
        } else {
            rawState->err=OPC_ERROR_SEEK;
        }
    }
    if (OPC_ERROR_NONE==rawState->err) {
        //@TODO write segment buffer!
        //@TODO deal with overflow buffer
    }
    OPC_ASSERT(Z_STREAM_END==buffer->inflate_state || Z_OK==buffer->inflate_state);
    if (Z_OK==buffer->inflate_state) { // this means the stream could not be closed...
        OPC_ASSERT(0); //@TODO implement me: return this stream...
    }
    if (Z_OK!=(buffer->inflate_state=deflateEnd(&buffer->stream))) {
        rawState->err=OPC_ERROR_DEFLATE;
    }
    if (NULL!=buffer->buf) xmlFree(buffer->buf);
    if (NULL!=buffer) xmlFree(buffer);
    return NULL;
}

opc_error_t opcZipInitSegment(opcZipSegment *segment) {
    opc_bzero_mem(segment, sizeof(*segment));
    return OPC_ERROR_NONE;
}

opc_error_t opcZipCleanupSegment(opcZipSegment *segment) {
    return OPC_ERROR_NONE;
}

static opc_bool_t opcZipGrowSegment(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer) {
    return zip->file_size==rawState->buf_pos+buffer->segment_size            // we are the last segment in the zip file
        && OPC_ERROR_NONE==opcZipFlushSegmentBuffer(zip, rawState, buffer)   // and we successfully flushed the buffer
        && OPC_ERROR_NONE==_opcZipFileGrow(zip, rawState->buf_pos+buffer->segment_size); // and we have enough file space left
}

static opc_uint32_t opcZipSegmentBufferFill(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer, const opc_uint8_t *data, opc_uint32_t data_len) {
    OPC_ASSERT(NULL!=zip && NULL!=rawState && NULL!=buffer && NULL!=buffer->buf && NULL!=data && buffer->buf_ofs+buffer->buf_len<=buffer->segment_size);
    opc_uint32_t const free=buffer->segment_size-buffer->buf_ofs-buffer->buf_len;
    opc_uint32_t const len=(free<data_len?free:data_len);
    opc_uint32_t ret=0;
    OPC_ASSERT(len<=data_len && len<=buffer->segment_size-(buffer->buf_ofs+buffer->buf_len));
    if (len>0) {
        if (0==buffer->compression_method) { // STORE
            buffer->stream.total_in+=len;
            buffer->stream.total_out+=len;
            buffer->crc32=crc32(buffer->crc32, data, len);
            memcpy(buffer->buf+buffer->buf_ofs, data, len);
            buffer->buf_len+=len;
            ret=len;
        } else if (8==buffer->compression_method) { // DEFLATE
            buffer->stream.avail_in=data_len;
            buffer->stream.next_in=(Bytef*)data;
            buffer->stream.avail_out=free;
            buffer->stream.next_out=buffer->buf+buffer->buf_ofs;
            if (Z_OK==(buffer->inflate_state=deflate(&buffer->stream, Z_NO_FLUSH))) {
                opc_uint32_t const bytes_in=data_len-buffer->stream.avail_in;
                opc_uint32_t const bytes_out=free-buffer->stream.avail_out;
                buffer->crc32=crc32(buffer->crc32, data, bytes_in);
                ret=bytes_in;
                buffer->buf_len+=bytes_out;
            } else {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else {
            rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return ret;
}

static opc_bool_t opcZipSegmentBufferFinish(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer) {
    opc_bool_t ret=OPC_FALSE;
    if (OPC_ERROR_NONE==rawState->err) {
        if (0==buffer->compression_method) { // STORE
            OPC_ASSERT(Z_OK==buffer->inflate_state);
            buffer->inflate_state=Z_STREAM_END;
            ret=(Z_STREAM_END==buffer->inflate_state);
        } else if (8==buffer->compression_method) { // DEFLATE
            opc_uint32_t const free=buffer->segment_size-buffer->buf_ofs-buffer->buf_len;
            buffer->stream.avail_in=NULL;
            buffer->stream.next_in=0;
            buffer->stream.avail_out=free;
            buffer->stream.next_out=buffer->buf+buffer->buf_ofs;
            if (Z_OK==(buffer->inflate_state=deflate(&buffer->stream, Z_FINISH)) || Z_STREAM_END==buffer->inflate_state) {
                opc_uint32_t const bytes_out=free-buffer->stream.avail_out;
                buffer->buf_len+=bytes_out;
                ret=(Z_STREAM_END==buffer->inflate_state);
            } else {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else {
            rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return ret;
}

opc_uint32_t opcZipSegmentBufferWrite(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer, const opc_uint8_t *buf, opc_uint32_t buf_len) {
    opc_uint32_t out=0;
    do {
        out+=opcZipSegmentBufferFill(zip, rawState, buffer, buf+out, buf_len-out);
        OPC_ASSERT(out<=buf_len);
    } while (out<buf_len && opcZipGrowSegment(zip, rawState, buffer));
    if (buffer->buf_len>0) {
        opcZipGrowSegment(zip, rawState, buffer);
    }
    return out;
}

opc_bool_t opcZipSegmentBufferClose(opcZip *zip, opcZipRawState *rawState, opcZipSegmentBuffer* buffer) {
    opc_bool_t closed=OPC_FALSE;
    opc_bool_t grow=OPC_TRUE;
    while (grow && !(closed=opcZipSegmentBufferFinish(zip, rawState, buffer))) {
        grow=opcZipGrowSegment(zip, rawState, buffer);
    }
    if (closed && buffer->buf_len>0) {
        closed=opcZipGrowSegment(zip, rawState, buffer);
    }
    return closed;
}


opc_error_t opcZipWriteCentralDirectory(opcZip *zip, opcZipRawState *rawState, opcZipSegment* segment, xmlChar *name, opc_uint32_t segment_number, opc_bool_t last_segment) {
    char filename[OPC_MAXPATH];
    opc_uint16_t filename_length=opcZipEncodeFilename(name, filename, sizeof(filename)); 
    filename_length=opcZipFilenameAppendPiece(segment_number, last_segment, filename, filename_length, sizeof(filename)); 
    if (OPC_ERROR_NONE==rawState->err && 0==filename_length) {
        rawState->err=OPC_ERROR_STREAM;
    }
    if (OPC_ERROR_NONE==rawState->err) {
        if((4==opcZipRawWriteU32(zip, rawState, 0x02014b50))
        && (2==opcZipRawWriteU16(zip, rawState, 20))  // version made by 
        && (2==opcZipRawWriteU16(zip, rawState, 20))  // version needed to extract
        && (2==opcZipRawWriteU16(zip, rawState, segment->bit_flag))  // bit flag
        && (2==opcZipRawWriteU16(zip, rawState, segment->compression_method))  // compression method
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // last mod file time
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // last mod file date
        && (4==opcZipRawWriteU32(zip, rawState, segment->crc32))  // crc32
        && (4==opcZipRawWriteU32(zip, rawState, segment->compressed_size))  // compressed size
        && (4==opcZipRawWriteU32(zip, rawState, segment->uncompressed_size))  // uncompressed size
        && (2==opcZipRawWriteU16(zip, rawState, filename_length))  // filename length
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // extra length
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // comment length
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // disk number start
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // internal file attributes
        && (4==opcZipRawWriteU32(zip, rawState, 0x0))  // external file attributes
        && (4==opcZipRawWriteU32(zip, rawState, segment->stream_ofs))  // relative offset of local header
        && (filename_length==opcZipRawWrite(zip, rawState, (opc_uint8_t*)filename, filename_length))) {

        } else {
            rawState->err=OPC_ERROR_STREAM;
        }
    }
    return rawState->err;
}


opc_error_t opcZipWriteEndOfCentralDirectory(opcZip *zip, opcZipRawState *rawState, opc_ofs_t central_dir_start_ofs, opc_uint32_t segments) {
    opc_ofs_t central_dir_end_ofs=rawState->buf_pos;
    OPC_ASSERT(central_dir_start_ofs<=central_dir_end_ofs);
    if (OPC_ERROR_NONE==rawState->err) {
        if((4==opcZipRawWriteU32(zip, rawState, 0x06054b50))
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // number of this disk
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))  // number of the disk with the start of the central directory
        && (2==opcZipRawWriteU16(zip, rawState, segments))  // total number of entries in the central directory on this disk
        && (2==opcZipRawWriteU16(zip, rawState, segments))  // total number of entries in the central directory 
        && (4==opcZipRawWriteU32(zip, rawState, central_dir_end_ofs-central_dir_start_ofs))  // size of the central directory
        && (4==opcZipRawWriteU32(zip, rawState, central_dir_start_ofs))  // offset of start of central directory with respect to the starting disk number
        && (2==opcZipRawWriteU16(zip, rawState, 0x0))) {// .ZIP file comment length

        } else {
            rawState->err=OPC_ERROR_STREAM;
        }
    }
    return rawState->err;
}




static inline int opcZipRawReadBuffer(opcZip *zip, opcZipRawBuffer *raw, opc_uint8_t *buffer, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=raw);
    OPC_ASSERT(OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs<=raw->buf_len);
    opc_uint32_t buf_ofs=0;
    while(OPC_ERROR_NONE==raw->state.err && buf_ofs<buf_len) {
        opc_uint32_t req_size=buf_len-buf_ofs;
        if (raw->buf_ofs<raw->buf_len) {
            opc_uint32_t size=raw->buf_len-raw->buf_ofs;
            if (size>req_size) size=req_size;
            memcpy(buffer+buf_ofs, raw->buf+raw->buf_ofs, size);
            raw->buf_ofs+=size;
            buf_ofs+=size;
            raw->state.buf_pos+=size;
        } else {
            OPC_ASSERT(raw->buf_ofs==raw->buf_len);
            int ret=_opcZipFileRead(zip, buffer+buf_ofs, req_size);
            if (0==ret) {
                buf_len=0; // causes the loop to exit
            } else if (ret<0) {
                raw->state.err=OPC_ERROR_STREAM;
            } else {
                buf_ofs+=ret;
                raw->state.buf_pos+=ret;
            }
        }
    }
    return buf_ofs;
}



static inline opc_uint32_t opcZipRawPeekHeaderSignature(opcZip *zip, opcZipRawBuffer *raw) {
    OPC_ASSERT(NULL!=zip && NULL!=raw);
    if (OPC_ERROR_NONE==raw->state.err && raw->buf_ofs+4>raw->buf_len) {
        // less than four bytes available...
        if (raw->buf_ofs>0) { // move the bytes to the beginning
            opc_uint32_t delta=raw->buf_len-raw->buf_ofs;
            OPC_ASSERT(delta<=4);
            for(opc_uint32_t i=0;i<delta;i++) {
                raw->buf[i]=raw->buf[i+raw->buf_ofs];
            }
            raw->buf_len-=raw->buf_ofs;
            raw->buf_ofs=0;
        }
        // fill the remaining buffer
        OPC_ASSERT(0==raw->buf_ofs && raw->buf_len<sizeof(raw->buf));
        int ret=_opcZipFileRead(zip, raw->buf+raw->buf_len, (sizeof(raw->buf)-raw->buf_len));
        if (ret<0) {
            raw->state.err=OPC_ERROR_STREAM;
        } else {
            raw->buf_len+=ret;
        }
        OPC_ASSERT(0==raw->buf_ofs && raw->buf_len<=sizeof(raw->buf));
    }
    if (OPC_ERROR_NONE==raw->state.err && 0==raw->buf_len) {
        return 0; // end of file
    } else if (OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs+4>raw->buf_len) { 
        // either an error or not enough bytes
        return -1;
    } else { 
        OPC_ASSERT(OPC_ERROR_NONE==raw->state.err && raw->buf_ofs+4<=raw->buf_len); // enough bytes...
        return (raw->buf[raw->buf_ofs]<<0)
              +(raw->buf[raw->buf_ofs+1]<<8)
              +(raw->buf[raw->buf_ofs+2]<<16)
              +(raw->buf[raw->buf_ofs+3]<<24);
    }
}

static inline opc_error_t opcZipRawFill(opcZip *zip, opcZipRawBuffer *raw, opc_uint32_t max) {
    OPC_ASSERT(OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs<=raw->buf_len);
    if (OPC_ERROR_NONE==raw->state.err && raw->buf_ofs==raw->buf_len) {
        opc_uint32_t const len=(sizeof(raw->buf)<max?sizeof(raw->buf):max);
        int ret=_opcZipFileRead(zip, raw->buf, len);
        if (ret<0) {
            raw->state.err=OPC_ERROR_STREAM;
            raw->buf_len=0;
        } else {
            raw->buf_len=ret;
        }
        raw->buf_ofs=0;
    }
    OPC_ASSERT(OPC_ERROR_NONE==raw->state.err || 0==raw->buf_len || raw->buf_ofs<raw->buf_len);
    return raw->state.err;
}

static inline opc_uint32_t opcZipRawInflateBuffer(opcZip *zip, opcZipRawBuffer *raw, opcZipInflateState *state, opc_uint8_t *buffer, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=raw);
    OPC_ASSERT(OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs<=raw->buf_len);
    opc_uint32_t buf_ofs=0;
    while(OPC_ERROR_NONE==raw->state.err &&  Z_OK==state->inflate_state && buf_ofs<buf_len) {
        int const max_stream=state->compressed_size-state->stream.total_in;
        if (OPC_ERROR_NONE==opcZipRawFill(zip, raw, max_stream)) {
            state->stream.next_in=raw->buf+raw->buf_ofs;
            state->stream.avail_in=raw->buf_len-raw->buf_ofs;
            state->stream.next_out=buffer+buf_ofs;
            state->stream.avail_out=buf_len-buf_ofs;
            if (Z_OK==(state->inflate_state=inflate(&state->stream, Z_SYNC_FLUSH)) || Z_STREAM_END==state->inflate_state) {
                opc_uint32_t const consumed_in=raw->buf_len-raw->buf_ofs-state->stream.avail_in;
                opc_uint32_t const consumed_out=buf_len-buf_ofs-state->stream.avail_out;
                raw->buf_ofs+=consumed_in;
                raw->state.buf_pos+=consumed_in;
                buf_ofs+=consumed_out;
                if (0==consumed_out && 0==consumed_in) {
                    raw->state.err=OPC_ERROR_DEFLATE; // protect us from an endless loop. shlould not happen
                }
            } else {
                raw->state.err=OPC_ERROR_DEFLATE;
            }
        }
    }
    return buf_ofs;
}


static inline int opcZipRawReadU8(opcZip *zip, opcZipRawBuffer *raw, opc_uint8_t *val) {
    OPC_ASSERT(NULL!=raw);
    opcZipRawFill(zip, raw, -1);
    OPC_ASSERT(OPC_ERROR_NONE==raw->state.err || 0==raw->buf_len || raw->buf_ofs<raw->buf_len);
    if (OPC_ERROR_NONE!=raw->state.err) {
        if (NULL!=val) {
            *val=-1;
        }
        return -1;
    } else if (0==raw->buf_len) {
        if (NULL!=val) {
            *val=0;
        }
        return 0;
    } else {
        if (NULL!=val) {
            *val=raw->buf[raw->buf_ofs];
        }
        raw->buf_ofs++;
        raw->state.buf_pos++;
        return 1;
    }
}

static inline int opcZipRawRead8(opcZip *zip, opcZipRawBuffer *raw, opc_int8_t *val) {
    return opcZipRawReadU8(zip, raw, (opc_uint8_t*)val);
}

#define OPC_READ_LITTLE_ENDIAN(zip, raw, type, val) \
    opc_uint8_t aux;\
    int ret;\
    int i=0;\
    if (NULL!=val) {\
        *val=0;\
    }\
    while(i<sizeof(type) && (1==(ret=opcZipRawReadU8(zip, raw, &aux)))) {\
        if (NULL!=val) {\
            *val|=((type)aux)<<(i<<3);\
        }\
        i++;\
    }\
    return (i==sizeof(type)?i:ret);\

static inline int opcZipRawReadU16(opcZip *zip, opcZipRawBuffer *raw, opc_uint16_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_uint16_t, val);
}

static inline int opcZipRawRead16(opcZip *zip, opcZipRawBuffer *raw, opc_int16_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_int16_t, val);
}

static inline int opcZipRawReadU32(opcZip *zip, opcZipRawBuffer *raw, opc_uint32_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_uint32_t, val);
}

static inline int opcZipRawRead32(opcZip *zip, opcZipRawBuffer *raw, opc_int32_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_int32_t, val);
}

static inline int opcZipRawReadU64(opcZip *zip, opcZipRawBuffer *raw, opc_uint64_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_uint64_t, val);
}

static inline int opcZipRawRead64(opcZip *zip, opcZipRawBuffer *raw, opc_int64_t *val) {
    OPC_READ_LITTLE_ENDIAN(zip, raw, opc_int64_t, val);
}

static inline int opcZipRawSkipBytes(opcZip *zip, opcZipRawBuffer *raw, opc_uint32_t len) {
    if (OPC_ERROR_NONE!=raw->state.err) {
        return 0;
    } else {
        //@TODO speed me up! when seek is available
        int ret=0;
        opc_uint32_t i=0;
        opc_uint8_t val;
        while(i<len && (1==(ret=opcZipRawReadU8(zip, raw, &val)))) i++;
        return (i==len?i:ret);
    }
}

static inline int opcZipRawReadString(opcZip *zip, opcZipRawBuffer *raw, xmlChar *str, opc_uint32_t str_len, opc_uint32_t str_max) {
    opc_uint32_t str_ofs=0;
    opc_uint32_t flag=1;
    while(OPC_ERROR_NONE==raw->state.err && str_ofs<str_len && str_ofs+1<str_max && flag>0) {
        opc_uint8_t ch=0;
        if (1==(flag=opcZipRawReadU8(zip, raw, &ch))) {
            if ('%'==ch) {
                opc_uint8_t hex1=0;
                opc_uint8_t hex2=0;
                if (1==(flag=opcZipRawReadU8(zip, raw, &hex1)) && 1==(flag=opcZipRawReadU8(zip, raw, &hex2))) {
                    opc_uint32_t hi=0;
                    if (hex1>='0' && hex1<='9') hi=hex1-'0';
                    else if (hex1>='A' && hex1<='F') hi=hex1-'A'+10;
                    else if (hex1>='a' && hex1<='f') hi=hex1-'a'+10;
                    else flag=0; // error
                    opc_uint32_t low=0;
                    if (hex1>='0' && hex1<='9') low=hex1-'0';
                    else if (hex1>='A' && hex1<='F') low=hex1-'A'+10;
                    else if (hex1>='a' && hex1<='f') low=hex1-'a'+10;
                    else flag=0; // error
                    if (flag) {
                        str[str_ofs++]=hi*16+low;
                    }
                }
            } else {
                str[str_ofs++]=ch;
            }
        }
    }
    OPC_ASSERT(str_ofs+1<str_max);
    str[str_ofs]=0; // terminate string
    return str_ofs;
}


static opc_error_t _opcZipSplitFilename(opc_uint8_t *filename, opc_uint32_t filename_length, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment) {
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

opc_bool_t opcZipRawReadLocalFile(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment, xmlChar **name, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment) {
    opc_bool_t ret=OPC_FALSE;
    opc_uint32_t sig=opcZipRawPeekHeaderSignature(zip, rawBuffer);
    if (0x04034b50==sig) {
        opc_uint32_t header_signature;
        opc_uint16_t filename_length;
        opc_uint16_t extra_length;
        opc_uint32_t compressed_size;
        opc_uint32_t uncompressed_size;
        opc_uint8_t filename[OPC_MAXPATH];
        opc_bzero_mem(segment, sizeof(*segment));
        segment->stream_ofs=rawBuffer->state.buf_pos;
        opc_uint16_t dummy;
        if (4==opcZipRawReadU32(zip, rawBuffer, &header_signature) && header_signature==sig) // version_needed
        if (2==opcZipRawReadU16(zip, rawBuffer, &dummy)) // version_needed
        if (2==opcZipRawReadU16(zip, rawBuffer, &segment->bit_flag))
        if (2==opcZipRawReadU16(zip, rawBuffer, &segment->compression_method))
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // last_mod_time
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // last_mod_date
        if (4==opcZipRawReadU32(zip, rawBuffer, &segment->crc32)) // 
        if (4==opcZipRawReadU32(zip, rawBuffer, &compressed_size))
        if (4==opcZipRawReadU32(zip, rawBuffer, &uncompressed_size))
        if (2==opcZipRawReadU16(zip, rawBuffer, &filename_length))
        if (2==opcZipRawReadU16(zip, rawBuffer, &extra_length))
        if (filename_length==opcZipRawReadString(zip, rawBuffer, filename, filename_length, sizeof(filename))) {
            segment->compressed_size=compressed_size;
            segment->uncompressed_size=uncompressed_size;
            segment->header_size=4*4+7*2+filename_length+extra_length;

            while(OPC_ERROR_NONE==rawBuffer->state.err && extra_length>0) {
                opc_uint16_t extra_id;
                opc_uint16_t extra_size;
                if (2==opcZipRawReadU16(zip, rawBuffer, &extra_id) && 2==opcZipRawReadU16(zip, rawBuffer, &extra_size)) {
                    switch(extra_id) {
                    case 0xa220: // Microsoft Open Packaging Growth Hint => ignore
                        {
                            opc_uint16_t sig;
                            opc_uint16_t padVal;
                            if (2==opcZipRawReadU16(zip, rawBuffer, &sig) && 0xa028==sig
                                && 2==opcZipRawReadU16(zip, rawBuffer, &padVal) 
                                && extra_size>=4 && extra_size-4==opcZipRawSkipBytes(zip, rawBuffer, extra_size-4)) {
                                segment->growth_hint=padVal;
                                extra_length-=4+extra_size;
                            } else {
                                rawBuffer->state.err=OPC_ERROR_STREAM;
                            }

                        }
                        break;
                    default: // just ignore
                        opc_logf("**ignoring extra data id=%02X size=%02X\n", extra_id, extra_size);
                        // no break;
                        if (extra_size==opcZipRawSkipBytes(zip, rawBuffer, extra_size)) {
                            extra_length-=4+extra_size;
                        } else {
                            rawBuffer->state.err=OPC_ERROR_STREAM;
                        }
                        break;
                    }
                }
                
            }
            if (0==extra_length && OPC_ERROR_NONE==rawBuffer->state.err) {
                rawBuffer->state.err=_opcZipSplitFilename(filename, filename_length, segment_number, last_segment, rel_segment);
            }
            ret=(OPC_ERROR_NONE==rawBuffer->state.err && 0==extra_length);
            if (ret && NULL!=name) {
                *name=xmlStrdup(_X(filename));
            }
        }
    }
    return ret;
}


opc_error_t opcZipInitInflateState(opcZipRawState *rawState, opcZipSegment *segment, opcZipInflateState *state) {
    memset(state, 0, sizeof(*state));
    state->stream.zalloc = Z_NULL;
    state->stream.zfree = Z_NULL;
    state->stream.opaque = Z_NULL;
    state->stream.next_in = Z_NULL;
    state->stream.avail_in = 0;
    state->compressed_size=segment->compressed_size;
    state->compression_method=segment->compression_method;
    state->inflate_state = Z_STREAM_ERROR;
    if (OPC_ERROR_NONE!=rawState->err) {
        return rawState->err;
    } else if (0==state->compression_method) { // STORE
        return rawState->err;
    } else if (8==state->compression_method) { // DEFLATE
        if (Z_OK!=(state->inflate_state=inflateInit2(&state->stream, -MAX_WBITS))) {
            rawState->err=OPC_ERROR_DEFLATE;
        }
        return rawState->err;
    } else {
        rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        return rawState->err;
    }
}

opc_error_t opcZipCleanupInflateState(opcZipRawState *rawState, opcZipSegment *segment, opcZipInflateState *state) {
    if (OPC_ERROR_NONE==rawState->err) {
        if (0==state->compression_method) { // STORE
            if (state->stream.total_in!=segment->compressed_size) {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else if (8==state->compression_method) { // DEFLATE
            if (Z_OK!=(state->inflate_state=inflateEnd(&state->stream))) {
                rawState->err=OPC_ERROR_DEFLATE;
            } else if (state->stream.total_in!=segment->compressed_size || state->stream.total_out!=segment->uncompressed_size) {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else {
            rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return rawState->err;
}


opc_uint32_t opcZipRawReadFileData(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipInflateState *state, opc_uint8_t *buffer, opc_uint32_t buf_len) {
    if (8==state->compression_method) {
        return opcZipRawInflateBuffer(zip, rawBuffer, state, buffer, buf_len);
    } else if (0==state->compression_method) {
        opc_uint32_t const max_stream=state->compressed_size-state->stream.total_in;
        opc_uint32_t const max_in=buf_len<max_stream?buf_len:max_stream;
        int ret=opcZipRawReadBuffer(zip, rawBuffer, buffer, max_in);
        state->stream.total_in+=ret;
        return ret;
    } else {
        return OPC_ERROR_UNSUPPORTED_COMPRESSION;
    }
}

opc_error_t opcZipRawSkipFileData(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment) {
    if (segment->compressed_size!=opcZipRawSkipBytes(zip, rawBuffer, segment->compressed_size) && OPC_ERROR_NONE==rawBuffer->state.err) {
        rawBuffer->state.err=OPC_ERROR_STREAM;
    }
    return rawBuffer->state.err;
}

opc_error_t opcZipRawReadDataDescriptor(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment) {
    return OPC_ERROR_NONE;
}

opc_bool_t opcZipRawReadCentralDirectory(opcZip *zip, opcZipRawBuffer *rawBuffer, opcZipSegment *segment, xmlChar **name, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment) {
    opc_bool_t ret=OPC_FALSE;
    opc_uint32_t sig=opcZipRawPeekHeaderSignature(zip, rawBuffer);
    if (0x02014b50==sig) {
        opc_uint32_t header_signature;
        opc_uint16_t filename_length;
        opc_uint16_t extra_length;
        opc_uint16_t comment_length;
        opc_uint32_t compressed_size;
        opc_uint32_t uncompressed_size;
        opc_uint8_t filename[OPC_MAXPATH];
        opc_bzero_mem(segment, sizeof(*segment));
        opc_uint16_t dummy;
        if (4==opcZipRawReadU32(zip, rawBuffer, &header_signature) && header_signature==sig) // version_needed
        if (2==opcZipRawReadU16(zip, rawBuffer, &dummy)) // version_needed
        if (2==opcZipRawReadU16(zip, rawBuffer, &dummy)) // version_needed_to_extract
        if (2==opcZipRawReadU16(zip, rawBuffer, &segment->bit_flag))
        if (2==opcZipRawReadU16(zip, rawBuffer, &segment->compression_method))
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // last_mod_time
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // last_mod_date
        if (4==opcZipRawReadU32(zip, rawBuffer, &segment->crc32)) // 
        if (4==opcZipRawReadU32(zip, rawBuffer, &compressed_size))
        if (4==opcZipRawReadU32(zip, rawBuffer, &uncompressed_size))
        if (2==opcZipRawReadU16(zip, rawBuffer, &filename_length))
        if (2==opcZipRawReadU16(zip, rawBuffer, &extra_length))
        if (2==opcZipRawReadU16(zip, rawBuffer, &comment_length))
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // disk number start
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // internal file attributes
        if (4==opcZipRawReadU32(zip, rawBuffer, NULL)) // external file attributes
        if (4==opcZipRawReadU32(zip, rawBuffer, &segment->stream_ofs))  // relative offset of local header
        if (filename_length==opcZipRawReadString(zip, rawBuffer, filename, filename_length, sizeof(filename))) {
            segment->compressed_size=compressed_size;
            segment->uncompressed_size=uncompressed_size;

            while(OPC_ERROR_NONE==rawBuffer->state.err && extra_length>0) {
                opc_uint16_t extra_id;
                opc_uint16_t extra_size;
                if (2==opcZipRawReadU16(zip, rawBuffer, &extra_id) && 2==opcZipRawReadU16(zip, rawBuffer, &extra_size)) {
                    switch(extra_id) {
                    default: // just ignore
                        opc_logf("**ignoring extra data id=%02X size=%02X\n", extra_id, extra_size);
                        // no break;
                        if (extra_size==opcZipRawSkipBytes(zip, rawBuffer, extra_size)) {
                            extra_length-=4+extra_size;
                        } else {
                            rawBuffer->state.err=OPC_ERROR_STREAM;
                        }
                        break;
                    }
                }
                
            }
            if (0==extra_length && OPC_ERROR_NONE==rawBuffer->state.err && comment_length>0) {
                comment_length-=opcZipRawSkipBytes(zip, rawBuffer, comment_length);
            }
            if (0==extra_length && 0==comment_length && OPC_ERROR_NONE==rawBuffer->state.err) {
                rawBuffer->state.err=_opcZipSplitFilename(filename, filename_length, segment_number, last_segment, rel_segment);
            }
            ret=(OPC_ERROR_NONE==rawBuffer->state.err && 0==extra_length && 0==comment_length);
            if (ret && NULL!=name) {
                *name=xmlStrdup(_X(filename));
            }
        }
    }
    return ret;
}

opc_bool_t opcZipRawReadEndOfCentralDirectory(opcZip *zip, opcZipRawBuffer *rawBuffer, opc_uint16_t *central_dir_entries) {
    opc_bool_t ret=OPC_FALSE;
    opc_uint32_t sig=opcZipRawPeekHeaderSignature(zip, rawBuffer);
    if (0x06054b50==sig) {
        opc_uint32_t header_signature;
        opc_uint16_t comment_length;
        if (4==opcZipRawReadU32(zip, rawBuffer, &header_signature) && header_signature==sig) // version_needed
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // number of this disk
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // number of the disk with the start of the central directory
        if (2==opcZipRawReadU16(zip, rawBuffer, NULL)) // total number of entries in the central directory on this disk
        if (2==opcZipRawReadU16(zip, rawBuffer, central_dir_entries)) // total number of entries in the central directory
        if (4==opcZipRawReadU32(zip, rawBuffer, NULL)) // size of the central directory
        if (4==opcZipRawReadU32(zip, rawBuffer, NULL)) // offset of start of central directory with respect to the starting disk number
        if (2==opcZipRawReadU16(zip, rawBuffer, &comment_length)) {
            if (OPC_ERROR_NONE==rawBuffer->state.err && comment_length>0) {
                comment_length-=opcZipRawSkipBytes(zip, rawBuffer, comment_length);
            }

            ret=(OPC_ERROR_NONE==rawBuffer->state.err && 0==comment_length && rawBuffer->buf_ofs==rawBuffer->buf_len && rawBuffer->state.buf_pos==zip->file_size);
        }
    }
    return ret;
}


opcZipSegmentInputStream* opcZipCreateSegmentInputStream(opcZip *zip, opcZipSegment *segment) {
    opcZipSegmentInputStream *stream=(opcZipSegmentInputStream*)xmlMalloc(sizeof(opcZipSegmentInputStream));
    if (NULL!=stream) {
        opc_bzero_mem(stream, sizeof(*stream));
        opcZipInitRawBuffer(zip, &stream->raw);
        stream->raw.state.buf_pos=segment->stream_ofs+segment->header_size;
        if(OPC_ERROR_NONE!=opcZipInitInflateState(&stream->raw.state, segment, &stream->state)) {
            xmlFree(stream); stream=NULL; // error
        }
    }
    return stream;
}

opc_uint32_t opcZipReadSegmentInputStream(opcZip *zip, opcZipSegmentInputStream *stream, opc_uint8_t *buf, opc_uint32_t buf_len) {
    opc_uint32_t len=0;
    OPC_ENSURE(stream->raw.state.buf_pos+stream->raw.buf_len-stream->raw.buf_ofs==_opcZipFileSeek(zip, stream->raw.state.buf_pos+stream->raw.buf_len-stream->raw.buf_ofs, opcZipSeekSet));
    OPC_ASSERT(zip->state.buf_pos==stream->raw.state.buf_pos+stream->raw.buf_len-stream->raw.buf_ofs);
    len=opcZipRawReadFileData(zip, &stream->raw, &stream->state, buf, buf_len);
    return len;
}

opc_error_t opcZipCloseSegmentInputStream(opcZip *zip, opcZipSegment *segment, opcZipSegmentInputStream *stream) {
    opc_error_t err=opcZipCleanupInflateState(&stream->raw.state, segment, &stream->state);
    return err;
}
