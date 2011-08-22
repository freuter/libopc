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
#include <opc/helper.h>
#include <libxml/xmlmemory.h>
#include <libxml/globals.h>
#include <string.h>
#include <stdio.h>
#include "internal.h"

static void* ensureItem(void **array_, puint32_t items, puint32_t item_size) {
    *array_=xmlRealloc(*array_, (items+1)*item_size);
    return *array_;
}

static opcZipSegment* ensureSegment(opcZip *zip) {
    return ((opcZipSegment*)ensureItem((void**)&zip->segment_array, zip->segment_items, sizeof(opcZipSegment)))+zip->segment_items;
}


static inline opc_uint32_t _opcZipFileRead(opcIO_t *io, opc_uint8_t *buf, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=io && io->_ioread!=NULL && NULL!=buf);
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==io->state.err) {
        int len=io->_ioread(io->iocontext, (char *)buf, buf_len);
        if (ret<0) {
            io->state.err=OPC_ERROR_STREAM;
        } else {
            ret=(opc_uint32_t)len;
            io->state.buf_pos+=ret;
        }
    }
    return ret;
}

static inline opc_uint32_t _opcZipFileWrite(opcIO_t *io, const opc_uint8_t *buf, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=io && io->_iowrite!=NULL && NULL!=buf);
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==io->state.err) {
        int len=io->_iowrite(io->iocontext, (const char *)buf, buf_len);
        if (ret<0) {
            io->state.err=OPC_ERROR_STREAM;
        } else {
            ret=(opc_uint32_t)len;
            io->state.buf_pos+=ret;
            if (io->state.buf_pos>io->file_size) io->file_size=io->state.buf_pos;
        }
    }
    return ret;
}

static inline opc_error_t _opcZipFileClose(opcIO_t *io) {
    OPC_ASSERT(NULL!=io && io->_ioclose!=NULL);
    if (0!=io->_ioclose(io->iocontext)) {
        if (OPC_ERROR_NONE==io->state.err) {
            io->state.err=OPC_ERROR_STREAM;
        }
    }
    return io->state.err;
}

static inline opc_ofs_t _opcZipFileSeek(opcIO_t *io, opc_ofs_t ofs, opcFileSeekMode whence) {
    OPC_ASSERT(NULL!=io && (opcFileSeekSet==whence || opcFileSeekCur==whence || opcFileSeekEnd==whence));
    opc_ofs_t abs=io->state.buf_pos;
    if (OPC_ERROR_NONE==io->state.err) {
        switch (whence) {
        case opcFileSeekSet: abs=ofs; break;
        case opcFileSeekCur: abs+=ofs; break;
        case opcFileSeekEnd: abs=io->file_size-ofs; break;
        }
        if (abs<0 || abs>io->file_size) {
            io->state.err=OPC_ERROR_STREAM;
        } else if (io->state.buf_pos!=abs) {
            if (NULL!=io->_ioseek) {
                int _ofs=io->_ioseek(io->iocontext, abs);
                if (_ofs!=abs) {
                    io->state.err=OPC_ERROR_STREAM;
                } else {
                    io->state.buf_pos=abs;
                }
            } else {
                io->state.err=OPC_ERROR_SEEK;
            }
        }
    }
    return abs;
}

static inline opc_error_t _opcZipFileGrow(opcIO_t *io, opc_uint32_t abs) {
    if (OPC_ERROR_NONE==io->state.err) {
        if (abs>io->file_size) {
            io->file_size=abs; //@TODO add error handling here if e.g. file can not grow because disk is full etc...
        }
    }
    return io->state.err;
}


opc_error_t _opcZipFileMove(opcIO_t *io, opc_ofs_t dest, opc_ofs_t src, opc_ofs_t len) {
    opc_uint8_t buf[1024];
    while(len>0) {
        opc_ofs_t delta=(dest<src?src-dest:dest-src);
        opc_uint32_t chunk=(delta>sizeof(buf)?(opc_uint32_t)sizeof(buf):(opc_uint32_t)delta);
        if (chunk>len) chunk=len;
        OPC_ENSURE(src==_opcZipFileSeek(io, src, opcFileSeekSet));
        OPC_ENSURE(chunk==_opcZipFileRead(io, buf, chunk));
        OPC_ENSURE(dest==_opcZipFileSeek(io, dest, opcFileSeekSet));
        OPC_ENSURE(chunk==_opcZipFileWrite(io, buf, chunk));
        OPC_ASSERT(chunk<=len);
        len-=chunk;
        dest+=chunk;
        src+=chunk;
    }
    OPC_ASSERT(0==len);
    return io->state.err;
}

opc_error_t _opcZipFileTrim(opcIO_t *io, opc_ofs_t new_size) {
    OPC_ASSERT(new_size<=io->file_size);
    int ret=(NULL!=io->_iotrim?io->_iotrim(io->iocontext, new_size):-1);
    return (0==ret?OPC_ERROR_NONE:OPC_ERROR_STREAM);
}

opc_error_t _opcZipFileFlush(opcIO_t *io) {
    int ret=(NULL!=io->_ioflush?io->_ioflush(io->iocontext):-1);
    return (0==ret?OPC_ERROR_NONE:OPC_ERROR_STREAM);
}

opcZip *opcZipCreate(opcIO_t *io) {
    opcZip *zip=(opcZip*)xmlMalloc(sizeof(opcZip));
    if (NULL!=zip) {
        memset(zip, 0, sizeof(*zip));
        zip->first_free_segment_id=-1;
        zip->io=io; 
    }
    return zip;
}


void opcZipClose(opcZip *zip, opcZipSegmentReleaseCallback* releaseCallback) {
    if (NULL!=zip) {
        if (NULL!=releaseCallback) {
            for(opc_uint32_t i=0;i<zip->segment_items;i++) {
                if (!zip->segment_array[i].deleted_segment) {
                    releaseCallback(zip, i);
                }
            }
        }
        OPC_ASSERT(NULL!=zip->io->_ioclose);
        OPC_ENSURE(0==zip->io->_ioclose(zip->io->iocontext));
        xmlFree(zip);
    }
}



static inline opc_error_t _opcZipFileSeekRawState(opcIO_t *io, opcFileRawState *rawState, puint32_t new_ofs) {
    if (OPC_ERROR_NONE==rawState->err) {
        rawState->err=io->state.err;
    }
    if (OPC_ERROR_NONE==rawState->err) {
        if (_opcZipFileSeek(io, new_ofs, opcFileSeekSet)!=new_ofs) {
            *rawState=io->state; // indicate an error
        } else {
            rawState->buf_pos=new_ofs; // all OK
        }
    }
    return rawState->err;
}

static inline opc_uint32_t opcZipRawWrite(opcIO_t *io, opcFileRawState *raw, const opc_uint8_t *buffer, opc_uint32_t len) {
    opc_uint32_t ret=0;
    if (OPC_ERROR_NONE==raw->err) {
        if (OPC_ERROR_NONE==io->state.err) {
            if (raw->buf_pos==io->state.buf_pos) {
                ret=_opcZipFileWrite(io, buffer, len);
                *raw=io->state;
            } else {
                raw->err=OPC_ERROR_SEEK;
            }
        } else {
            *raw=io->state;
        }
    }
    return ret;
}

static inline opc_error_t opcZipInitRawState(opcIO_t *io, opcFileRawState *rawState) {
    *rawState=io->state;
    return rawState->err;
}


static inline int opcZipRawWriteU8(opcIO_t *io, opcFileRawState *raw, opc_uint8_t val) {
    return opcZipRawWrite(io, raw, (const opc_uint8_t *)&val, sizeof(opc_uint8_t));
}

static inline int opcZipRawWrite8(opcIO_t *io, opcFileRawState *raw, opc_int8_t val) {
    return opcZipRawWriteU8(io, raw, (opc_uint8_t)val);
}

static inline int opcZipRawWriteZero(opcIO_t *io, opcFileRawState *raw, opc_uint32_t count) {
    int ret=0;
    opc_uint8_t val=0;
    //@TODO may speed me up a bit (using fappend...)
    for(opc_uint32_t i=0;i<count;i++) {
        ret+=opcZipRawWriteU8(io, raw, val);
    }
    return ret;
}


static inline int opcZipRawWriteU16(opcIO_t *io, opcFileRawState *raw, opc_uint16_t val) {
    int i=0;
    while(OPC_ERROR_NONE==raw->err && i<sizeof(val) && 1==opcZipRawWriteU8(io, raw, (opc_uint8_t)(((unsigned)val)>>(i<<3)))) {
        i++;
    }
    return i;
}

static inline int opcZipRawWriteU32(opcIO_t *io, opcFileRawState *raw, opc_uint32_t val) {
    int i=0;
    while(OPC_ERROR_NONE==raw->err && i<sizeof(val) && 1==opcZipRawWriteU8(io, raw, (opc_uint8_t)(((unsigned)val)>>(i<<3)))) {
        i++;
    }
    return i;
}


static opc_error_t opcZipInitRawBuffer(opcIO_t *io, opcFileRawBuffer *rawBuffer) {
    opc_bzero_mem(rawBuffer, sizeof(*rawBuffer));
    return opcZipInitRawState(io, &rawBuffer->state);;
}

static inline opc_uint16_t opcZipCalculateHeaderSize(const char *name8, opc_uint16_t name8_len, opc_bool_t extra, opc_uint16_t *name8_max) {
    opc_uint16_t len=(NULL!=name8_max?*name8_max:name8_len);
    return 4*4+7*2+(extra?4*2:0)+len;
}

static opc_uint32_t opcZipRawWriteSegmentHeaderEx(opcIO_t *io, 
                                                  opcFileRawState *rawState, 
                                                  const char *name8, opc_uint16_t name8_len,
                                                  opc_uint16_t bit_flag,
                                                  opc_uint32_t crc32,
                                                  opc_uint16_t compression_method,
                                                  opc_ofs_t compressed_size,
                                                  opc_ofs_t uncompressed_size,
                                                  opc_uint16_t header_size,
                                                  opc_uint16_t growth_hint) {
    opc_uint32_t ret=0;
    OPC_ASSERT(opcZipCalculateHeaderSize(name8, name8_len, OPC_FALSE, NULL)<=header_size);
    opc_bool_t extra=opcZipCalculateHeaderSize(name8, name8_len, OPC_TRUE, NULL)<=header_size;
    if (opcZipCalculateHeaderSize(name8, name8_len, extra, NULL)<=header_size) {
        opc_uint16_t padding=(extra?header_size-opcZipCalculateHeaderSize(name8, name8_len, extra, NULL):0);
        OPC_ASSERT(padding==0 || extra); // padding!=0 implies extra... you need the extra header to be able to add padding!
        if ((4==(ret+=opcZipRawWriteU32(io, rawState, 0x04034b50)))
        && (6==(ret+=opcZipRawWriteU16(io, rawState, 20)))  //version needed to extract
        && (8==(ret+=opcZipRawWriteU16(io, rawState, bit_flag)))  // bit flag
        && (10==(ret+=opcZipRawWriteU16(io, rawState, compression_method)))  // compression method
        && (12==(ret+=opcZipRawWriteU16(io, rawState, 0x0)))  // last mod file time
        && (14==(ret+=opcZipRawWriteU16(io, rawState, 0x0)))  // last mod file date
        && (18==(ret+=opcZipRawWriteU32(io, rawState, crc32)))  // crc32
        && (22==(ret+=opcZipRawWriteU32(io, rawState, compressed_size)))  // compressed size
        && (26==(ret+=opcZipRawWriteU32(io, rawState, uncompressed_size)))  // uncompressed size
        && (28==(ret+=opcZipRawWriteU16(io, rawState, name8_len)))  // filename length
        && (30==(ret+=opcZipRawWriteU16(io, rawState, (extra?4*2+padding:0))))  // extra length
        && (30+name8_len==(ret+=opcZipRawWrite(io, rawState, (opc_uint8_t *)name8, name8_len)))
        && (!extra || 32+name8_len==(ret+=opcZipRawWriteU16(io, rawState, 0xa220)))  // extra: Microsoft Open Packaging Growth Hint
        && (!extra || 34+name8_len==(ret+=opcZipRawWriteU16(io, rawState, 2+2+padding)))  // extra: size of Sig + PadVal + Padding
        && (!extra || 36+name8_len==(ret+=opcZipRawWriteU16(io, rawState, 0xa028)))  // extra: verification signature (A028)
        && (!extra || 38+name8_len==(ret+=opcZipRawWriteU16(io, rawState, growth_hint)))  // extra: Initial padding value
        && (!extra || 38+name8_len+padding==(ret+=opcZipRawWriteZero(io, rawState, padding)))) { // extra: filled with NULL characters
            OPC_ASSERT(opcZipCalculateHeaderSize(name8, name8_len, extra, NULL)+padding==ret);
        }
        return (opcZipCalculateHeaderSize(name8, name8_len, extra, NULL)+padding==ret?ret:0);
    } else {
        return 0; // no enough space to write header!
    }
}

static opc_error_t opcZipRawWriteCentralDirectoryEx(opcIO_t *io, 
                                                  opcFileRawState *rawState, 
                                                  const char *name8, opc_uint16_t name8_len,
                                                  opc_uint16_t bit_flag,
                                                  opc_uint32_t crc32,
                                                  opc_uint16_t compression_method,
                                                  opc_ofs_t compressed_size,
                                                  opc_ofs_t uncompressed_size,
                                                  opc_uint16_t header_size,
                                                  opc_uint16_t growth_hint,
                                                  opc_ofs_t stream_ofs) {
    if (OPC_ERROR_NONE==rawState->err) {
        if((4==opcZipRawWriteU32(io, rawState, 0x02014b50))
        && (2==opcZipRawWriteU16(io, rawState, 20))  // version made by 
        && (2==opcZipRawWriteU16(io, rawState, 20))  // version needed to extract
        && (2==opcZipRawWriteU16(io, rawState, bit_flag))  // bit flag
        && (2==opcZipRawWriteU16(io, rawState, compression_method))  // compression method
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // last mod file time
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // last mod file date
        && (4==opcZipRawWriteU32(io, rawState, crc32))  // crc32
        && (4==opcZipRawWriteU32(io, rawState, compressed_size))  // compressed size
        && (4==opcZipRawWriteU32(io, rawState, uncompressed_size))  // uncompressed size
        && (2==opcZipRawWriteU16(io, rawState, name8_len))  // filename length
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // extra length
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // comment length
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // disk number start
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // internal file attributes
        && (4==opcZipRawWriteU32(io, rawState, 0x0))  // external file attributes
        && (4==opcZipRawWriteU32(io, rawState, stream_ofs))  // relative offset of local header
        && (name8_len==opcZipRawWrite(io, rawState, (opc_uint8_t*)name8, name8_len))) {

        } else {
            rawState->err=OPC_ERROR_STREAM;
        }
    }
    return rawState->err;
}


static opc_error_t opcZipRawWriteEndOfCentralDirectoryEx(opcIO_t *io, 
                                                  opcFileRawState *rawState,
                                                  opc_ofs_t central_dir_start_ofs, 
                                                  opc_uint32_t segments) {
    opc_ofs_t central_dir_end_ofs=rawState->buf_pos;
    OPC_ASSERT(central_dir_start_ofs<=central_dir_end_ofs);
    if (OPC_ERROR_NONE==rawState->err) {
        if((4==opcZipRawWriteU32(io, rawState, 0x06054b50))
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // number of this disk
        && (2==opcZipRawWriteU16(io, rawState, 0x0))  // number of the disk with the start of the central directory
        && (2==opcZipRawWriteU16(io, rawState, segments))  // total number of entries in the central directory on this disk
        && (2==opcZipRawWriteU16(io, rawState, segments))  // total number of entries in the central directory 
        && (4==opcZipRawWriteU32(io, rawState, central_dir_end_ofs-central_dir_start_ofs))  // size of the central directory
        && (4==opcZipRawWriteU32(io, rawState, central_dir_start_ofs))  // offset of start of central directory with respect to the starting disk number
        && (2==opcZipRawWriteU16(io, rawState, 0x0))) {// .ZIP file comment length

        } else {
            rawState->err=OPC_ERROR_STREAM;
        }
    }
    return rawState->err;
}


static inline int opcZipRawReadBuffer(opcIO_t *io, opcFileRawBuffer *raw, opc_uint8_t *buffer, opc_uint32_t buf_len) {
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
            int ret=_opcZipFileRead(io, buffer+buf_ofs, req_size);
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



static inline opc_uint32_t opcZipRawPeekHeaderSignature(opcIO_t *io, opcFileRawBuffer *raw) {
    OPC_ASSERT(NULL!=io && NULL!=raw);
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
        int ret=_opcZipFileRead(io, raw->buf+raw->buf_len, (sizeof(raw->buf)-raw->buf_len));
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

static inline opc_error_t opcZipRawFill(opcIO_t *io, opcFileRawBuffer *raw, opc_uint32_t max) {
    OPC_ASSERT(OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs<=raw->buf_len);
    if (OPC_ERROR_NONE==raw->state.err && raw->buf_ofs==raw->buf_len) {
        opc_uint32_t const len=(sizeof(raw->buf)<max?sizeof(raw->buf):max);
        int ret=_opcZipFileRead(io, raw->buf, len);
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

static inline opc_uint32_t opcZipRawInflateBuffer(opcIO_t *io, opcFileRawBuffer *raw, opcZipInflateState *state, opc_uint8_t *buffer, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=raw);
    OPC_ASSERT(OPC_ERROR_NONE!=raw->state.err || raw->buf_ofs<=raw->buf_len);
    opc_uint32_t buf_ofs=0;
    while(OPC_ERROR_NONE==raw->state.err &&  Z_OK==state->inflate_state && buf_ofs<buf_len) {
        int const max_stream=state->compressed_size-state->stream.total_in;
        if (OPC_ERROR_NONE==opcZipRawFill(io, raw, max_stream)) {
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


static inline int opcZipRawReadU8(opcIO_t *io, opcFileRawBuffer *raw, opc_uint8_t *val) {
    OPC_ASSERT(NULL!=raw);
    opcZipRawFill(io, raw, -1);
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

static inline int opcZipRawRead8(opcIO_t *io, opcFileRawBuffer *raw, opc_int8_t *val) {
    return opcZipRawReadU8(io, raw, (opc_uint8_t*)val);
}

#define OPC_READ_LITTLE_ENDIAN(io, raw, type, val) \
    opc_uint8_t aux;\
    int ret;\
    int i=0;\
    if (NULL!=val) {\
        *val=0;\
    }\
    while(i<sizeof(type) && (1==(ret=opcZipRawReadU8(io, raw, &aux)))) {\
        if (NULL!=val) {\
            *val|=((type)aux)<<(i<<3);\
        }\
        i++;\
    }\
    return (i==sizeof(type)?i:ret);\

static inline int opcZipRawReadU16(opcIO_t *io, opcFileRawBuffer *raw, opc_uint16_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_uint16_t, val);
}

static inline int opcZipRawRead16(opcIO_t *io, opcFileRawBuffer *raw, opc_int16_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_int16_t, val);
}

static inline int opcZipRawReadU32(opcIO_t *io, opcFileRawBuffer *raw, opc_uint32_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_uint32_t, val);
}

static inline int opcZipRawRead32(opcIO_t *io, opcFileRawBuffer *raw, opc_int32_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_int32_t, val);
}

static inline int opcZipRawReadU64(opcIO_t *io, opcFileRawBuffer *raw, opc_uint64_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_uint64_t, val);
}

static inline int opcZipRawRead64(opcIO_t *io, opcFileRawBuffer *raw, opc_int64_t *val) {
    OPC_READ_LITTLE_ENDIAN(io, raw, opc_int64_t, val);
}

static inline int opcZipRawSkipBytes(opcIO_t *io, opcFileRawBuffer *raw, opc_uint32_t len) {
    if (OPC_ERROR_NONE!=raw->state.err) {
        return 0;
    } else {
        //@TODO speed me up! when seek is available
        int ret=0;
        opc_uint32_t i=0;
        opc_uint8_t val;
        while(i<len && (1==(ret=opcZipRawReadU8(io, raw, &val)))) i++;
        return (i==len?i:ret);
    }
}

static inline int opcZipRawReadString(opcIO_t *io, opcFileRawBuffer *raw, xmlChar *str, opc_uint32_t str_len, opc_uint32_t str_max) {
    opc_uint32_t str_ofs=0;
    opc_uint32_t flag=1;
    while(OPC_ERROR_NONE==raw->state.err && str_ofs<str_len && str_ofs+1<str_max && flag>0) {
        opc_uint8_t ch=0;
        if (1==(flag=opcZipRawReadU8(io, raw, &ch))) {
            if ('%'==ch) {
                opc_uint8_t hex1=0;
                opc_uint8_t hex2=0;
                if (1==(flag=opcZipRawReadU8(io, raw, &hex1)) && 1==(flag=opcZipRawReadU8(io, raw, &hex2))) {
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

static opc_bool_t opcZipRawReadLocalFileEx(opcIO_t *io, opcFileRawBuffer *raw, 
                                    xmlChar *name, opc_uint32_t name_size, opc_uint32_t *name_len,
                                    opc_uint32_t *header_size,
                                    opc_uint32_t *min_header_size,
                                    opc_uint32_t *compressed_size,
                                    opc_uint32_t *uncompressed_size,
                                    opc_uint16_t *bit_flag,
                                    opc_uint32_t *crc32,
                                    opc_uint16_t *compression_method,
                                    opc_ofs_t *stream_ofs,
                                    opc_uint16_t *growth_hint) {
    opc_bool_t ret=OPC_FALSE;
    opc_uint32_t sig=opcZipRawPeekHeaderSignature(io, raw);
    if (0x04034b50==sig) {
        *stream_ofs=raw->state.buf_pos;
        opc_uint32_t header_signature;
        opc_uint16_t filename_length;
        opc_uint16_t extra_length;
        opc_uint16_t dummy;
        if (4==opcZipRawReadU32(io, raw, &header_signature) && header_signature==sig) // version_needed
        if (2==opcZipRawReadU16(io, raw, &dummy)) // version_needed
        if (2==opcZipRawReadU16(io, raw, bit_flag))
        if (2==opcZipRawReadU16(io, raw, compression_method))
        if (2==opcZipRawReadU16(io, raw, NULL)) // last_mod_time
        if (2==opcZipRawReadU16(io, raw, NULL)) // last_mod_date
        if (4==opcZipRawReadU32(io, raw, crc32)) // 
        if (4==opcZipRawReadU32(io, raw, compressed_size))
        if (4==opcZipRawReadU32(io, raw, uncompressed_size))
        if (2==opcZipRawReadU16(io, raw, &filename_length))
        if (2==opcZipRawReadU16(io, raw, &extra_length))
        if ((*name_len=opcZipRawReadString(io, raw, name, filename_length, name_size))<=filename_length) {
            *header_size=4*4+7*2+filename_length+extra_length;
            *min_header_size=4*4+7*2+filename_length;
            if (extra_length>=8) {
                *min_header_size+=8;  // reserve space for the Microsoft Open Packaging Growth Hint
            }
            while(OPC_ERROR_NONE==raw->state.err && extra_length>0) {
                opc_uint16_t extra_id;
                opc_uint16_t extra_size;
                if (2==opcZipRawReadU16(io, raw, &extra_id) && 2==opcZipRawReadU16(io, raw, &extra_size)) {
                    switch(extra_id) {
                    case 0xa220: // Microsoft Open Packaging Growth Hint => ignore
                        {
                            opc_uint16_t sig;
                            opc_uint16_t padVal;
                            if (2==opcZipRawReadU16(io, raw, &sig) && 0xa028==sig
                                && 2==opcZipRawReadU16(io, raw, &padVal) 
                                && extra_size>=4 && extra_size-4==opcZipRawSkipBytes(io, raw, extra_size-4)) {
                                *growth_hint=padVal;
                                extra_length-=4+extra_size;
                            } else {
                                raw->state.err=OPC_ERROR_STREAM;
                            }
                        }
                        break;
                    default: // just ignore
                        opc_logf("**ignoring extra data id=%02X size=%02X\n", extra_id, extra_size);
                        // no break;
                        if (extra_size==opcZipRawSkipBytes(io, raw, extra_size)) {
                            extra_length-=4+extra_size;
                        } else {
                            raw->state.err=OPC_ERROR_STREAM;
                        }
                        break;
                    }
                }
                
            }
            ret=(OPC_ERROR_NONE==raw->state.err && 0==extra_length);
        }
    }
    return ret;
}

static opc_error_t opcZipRawSkipFileData(opcIO_t *io, opcFileRawBuffer *raw, opc_uint32_t compressed_size) {
    if (compressed_size!=opcZipRawSkipBytes(io, raw, compressed_size) && OPC_ERROR_NONE==raw->state.err) {
        raw->state.err=OPC_ERROR_STREAM;
    }
    return raw->state.err;
}

static opc_error_t opcZipRawReadDataDescriptor(opcIO_t *io, opcFileRawBuffer *raw, opc_uint16_t bit_flag, opc_uint32_t *compressed_size, opc_uint32_t *uncompressed_size, opc_uint32_t *crc32, opc_uint32_t *trailing_bytes) {
    OPC_ASSERT(0==*trailing_bytes);
    if (0x8==(bit_flag & 0x8) && OPC_ERROR_NONE==raw->state.err) {
        // streaming mode
        OPC_ASSERT(0==*compressed_size);
        opc_uint32_t sig=opcZipRawPeekHeaderSignature(io, raw);
        if (0x08074b50==sig) {
            opc_uint32_t header_signature;
            OPC_ENSURE(4==opcZipRawReadU32(io, raw, &header_signature) && header_signature==sig);
            *trailing_bytes+=4;
        }
        if (OPC_ERROR_NONE==raw->state.err) {
            if ((4==opcZipRawReadU32(io, raw, crc32))
            && (4==opcZipRawReadU32(io, raw, compressed_size))
            && (4==opcZipRawReadU32(io, raw, uncompressed_size))) {
                OPC_ASSERT(OPC_ERROR_NONE==raw->state.err);
                *trailing_bytes+=3*4;
            } else if (OPC_ERROR_NONE==raw->state.err) {
                raw->state.err=OPC_ERROR_STREAM;
            }
        }
    }
    return raw->state.err;
}

static opc_error_t opcZipInitInflateState(opcFileRawState *rawState,
                                   opc_uint32_t compressed_size,
                                   opc_uint32_t uncompressed_size,
                                   opc_uint16_t compression_method, 
                                   opcZipInflateState *state) {
    memset(state, 0, sizeof(*state));
    state->stream.zalloc = Z_NULL;
    state->stream.zfree = Z_NULL;
    state->stream.opaque = Z_NULL;
    state->stream.next_in = Z_NULL;
    state->stream.avail_in = 0;
    state->compressed_size=compressed_size;
    state->compression_method=compression_method;
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

static opc_error_t opcZipCleanupInflateState(opcFileRawState *rawState, 
                                      opc_uint32_t compressed_size,
                                      opc_uint32_t uncompressed_size,
                                      opcZipInflateState *state) {
    if (OPC_ERROR_NONE==rawState->err) {
        if (0==state->compression_method) { // STORE
            if (state->stream.total_in!=compressed_size) {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else if (8==state->compression_method) { // DEFLATE
            if (Z_OK!=(state->inflate_state=inflateEnd(&state->stream))) {
                rawState->err=OPC_ERROR_DEFLATE;
            } else if (state->stream.total_in!=compressed_size || state->stream.total_out!=uncompressed_size) {
                rawState->err=OPC_ERROR_DEFLATE;
            }
        } else {
            rawState->err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return rawState->err;
}

static opc_uint32_t opcZipRawReadFileData(opcIO_t *io, opcFileRawBuffer *rawBuffer, opcZipInflateState *state, opc_uint8_t *buffer, opc_uint32_t buf_len) {
    if (8==state->compression_method) {
        return opcZipRawInflateBuffer(io, rawBuffer, state, buffer, buf_len);
    } else if (0==state->compression_method) {
        opc_uint32_t const max_stream=state->compressed_size-state->stream.total_in;
        opc_uint32_t const max_in=buf_len<max_stream?buf_len:max_stream;
        int ret=opcZipRawReadBuffer(io, rawBuffer, buffer, max_in);
        state->stream.total_in+=ret;
        return ret;
    } else {
        return OPC_ERROR_UNSUPPORTED_COMPRESSION;
    }
}

struct OPC_ZIPLOADER_IO_HELPER_STRUCT {
    opcIO_t *io;
    opcZipInflateState inflateState;
    opcFileRawBuffer rawBuffer;
    opcZipSegmentInfo_t info;
};

static int opcZipLoaderOpen(void *iocontext) {
    struct OPC_ZIPLOADER_IO_HELPER_STRUCT *helper=(struct OPC_ZIPLOADER_IO_HELPER_STRUCT *)iocontext;
    opc_error_t err=opcZipInitInflateState(&helper->rawBuffer.state, helper->info.compressed_size, helper->info.uncompressed_size, helper->info.compression_method, &helper->inflateState);
    return (OPC_ERROR_NONE==err?0:-1);
}

static int opcZipLoaderRead(void *iocontext, char *buffer, int len) {
    struct OPC_ZIPLOADER_IO_HELPER_STRUCT *helper=(struct OPC_ZIPLOADER_IO_HELPER_STRUCT *)iocontext;
    opc_uint32_t ret=opcZipRawReadFileData(helper->io, &helper->rawBuffer, &helper->inflateState, (opc_uint8_t*)buffer, len);
    return (OPC_ERROR_NONE==helper->rawBuffer.state.err?(int)ret:-1);
}

static int opcZipLoaderClose(void *iocontext) {
    struct OPC_ZIPLOADER_IO_HELPER_STRUCT *helper=(struct OPC_ZIPLOADER_IO_HELPER_STRUCT *)iocontext;
    opc_error_t err=opcZipRawReadDataDescriptor(helper->io, &helper->rawBuffer, helper->info.bit_flag, &helper->info.compressed_size, &helper->info.uncompressed_size, &helper->info.data_crc, &helper->info.trailing_bytes);
    if (OPC_ERROR_NONE==err) {
        err=opcZipCleanupInflateState(&helper->rawBuffer.state, helper->info.compressed_size, helper->info.uncompressed_size, &helper->inflateState);
    }
    return (OPC_ERROR_NONE==err?0:-1);
}

static int opcZipLoaderSkip(void *iocontext) {
    struct OPC_ZIPLOADER_IO_HELPER_STRUCT *helper=(struct OPC_ZIPLOADER_IO_HELPER_STRUCT *)iocontext;
    opc_error_t err=OPC_ERROR_NONE;
    if (0x8==(helper->info.bit_flag & 0x8)) {
        // streaming mode
        if (8==helper->info.compression_method) {
            if (0==opcZipLoaderOpen(iocontext)) {
                char buf[OPC_DEFLATE_BUFFER_SIZE];
                int ret=0;
                while ((ret=opcZipLoaderRead(iocontext, buf, sizeof(buf)))>0);
                if (OPC_ERROR_NONE==err && ret!=0) {
                    err=OPC_ERROR_STREAM;
                }
                if (opcZipLoaderClose(iocontext)!=0 && OPC_ERROR_NONE==err) {
                    err=OPC_ERROR_STREAM;
                }
            } else {
                err=OPC_ERROR_STREAM;
            }
        } else {
            OPC_ASSERT(0==helper->info.compression_method);
            opc_uint8_t buf[3*sizeof(opc_uint32_t)];
            if (sizeof(buf)==opcZipRawReadBuffer(helper->io, &helper->rawBuffer, buf, sizeof(buf))) {
                opc_uint32_t stream_len=0;
                opc_uint32_t crc=0;
                do {
                    opc_uint32_t v[3];
                    opc_uint32_t k=0;
                    for(opc_uint32_t j=0;j<3;j++) {
                        v[j]=0;
                        for(opc_uint32_t i=0;i<sizeof(opc_uint32_t);i++) {
                            v[j]|=(opc_uint32_t)buf[(stream_len+(j*sizeof(v[0]))+i)%sizeof(buf)]<<(i<<3);
                        }
                    }
                    opc_uint32_t skip_len=0;
                    if (0x08074b50==v[0]) {
                        // useless data descriptor signature, recalc v0..v3
                        for(opc_uint32_t i=0;i<2;i++) v[i]=v[i+1];
                        v[2]=opcZipRawPeekHeaderSignature(helper->io, &helper->rawBuffer);
                        skip_len=sizeof(opc_uint32_t);
                    }
                    if (crc==v[0] && stream_len==v[1] && stream_len==v[2]) {
                        if (OPC_ERROR_NONE==err && skip_len>0) {
                            err=opcZipRawSkipFileData(helper->io, &helper->rawBuffer, skip_len);
                        }
                        // found valid data descriptor, assume this is the end of the local content
                        if (OPC_ERROR_NONE==err) {
                            helper->info.data_crc=v[0];
                            helper->info.compressed_size=v[1];
                            helper->info.uncompressed_size=v[2];
                            helper->info.trailing_bytes=sizeof(buf)+skip_len;
                        }
                        break;
                    } else {
                        // check next byte
                        crc=crc32(crc, &buf[stream_len%sizeof(buf)], sizeof(buf[0]));
                        if (sizeof(buf[0])==opcZipRawReadBuffer(helper->io, &helper->rawBuffer, &buf[stream_len%sizeof(buf)], sizeof(buf[0]))) {
                            stream_len++;
                        } else {
                            err=OPC_ERROR_STREAM;
                            break;
                        }
                    }
                } while(OPC_ERROR_NONE==helper->rawBuffer.state.err);
            } else {
                err=OPC_ERROR_STREAM; // not enough bytes for the trailing data descriptor => error
            }
        }
    } else {
        err=opcZipRawSkipFileData(helper->io, &helper->rawBuffer, helper->info.compressed_size);
    }
    return (OPC_ERROR_NONE==err?0:-1);
}


opc_error_t opcZipLoader(opcIO_t *io, void *userctx, opcZipLoaderSegmentCallback_t *segmentCallback) {
    struct OPC_ZIPLOADER_IO_HELPER_STRUCT helper;
    opc_bzero_mem(&helper, sizeof(helper));
    helper.io=io;
    OPC_ENSURE(OPC_ERROR_NONE==opcZipInitRawBuffer(io, &helper.rawBuffer));
    while(OPC_ERROR_NONE==helper.rawBuffer.state.err &&
        opcZipRawReadLocalFileEx(io, &helper.rawBuffer, helper.info.name, sizeof(helper.info.name), &helper.info.name_len,
        &helper.info.header_size, &helper.info.min_header_size, &helper.info.compressed_size, &helper.info.uncompressed_size, &helper.info.bit_flag, &helper.info.data_crc, &helper.info.compression_method, &helper.info.stream_ofs, &helper.info.growth_hint)) {
        OPC_ASSERT(helper.info.min_header_size<=helper.info.header_size);
        helper.info.trailing_bytes=0;
        OPC_ASSERT(NULL!=segmentCallback);
        OPC_ENSURE(OPC_ERROR_NONE==opcHelperSplitFilename(helper.info.name, helper.info.name_len, &helper.info.segment_number, &helper.info.last_segment, &helper.info.rels_segment));
        opc_error_t ret=segmentCallback(&helper, userctx, &helper.info, opcZipLoaderOpen, opcZipLoaderRead, opcZipLoaderClose, opcZipLoaderSkip);
        OPC_ASSERT(OPC_ERROR_NONE==ret);
        if (OPC_ERROR_NONE==helper.rawBuffer.state.err && OPC_ERROR_NONE!=ret) {
            helper.rawBuffer.state.err=ret; // indicate an error
        }
    }
    //@TODO verify directoy etc..
#if 0
                opcZipSegment segment;
                xmlChar *name=NULL;
                opc_uint32_t segment_number;
                opc_bool_t last_segment;
                while(opcZipRawReadCentralDirectory(zip, &rawBuffer, &segment, &name, &segment_number, &last_segment, NULL)) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupSegment(&segment));
                    xmlFree(name);
                }
                opc_uint16_t central_dir_entries=0;
                OPC_ENSURE(opcZipRawReadEndOfCentralDirectory(zip, &rawBuffer, &central_dir_entries));
                for(opc_uint32_t i=0;i<container.part_items;i++) {
                    opcZipSegmentInputStream *stream=opcZipCreateSegmentInputStream(zip, &container.part_array[i].segment);
                    opc_uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
                    opc_uint32_t len=0;
                    opc_uint32_t crc=0;
                    while((len=opcZipReadSegmentInputStream(zip, stream, buf, sizeof(buf)))>0) {
                        crc=crc32(crc, buf, len);
                    }
                    OPC_ASSERT(crc==container.part_array[i].segment.crc32);
                    printf("%s [%08X]\n", container.part_array[i].name, crc);
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCloseSegmentInputStream(zip, &container.part_array[i].segment, stream));
                }
                for(opc_uint32_t i=0;i<container.part_items;i++) {
                    OPC_ENSURE(OPC_ERROR_NONE==opcZipCleanupSegment(&container.part_array[i].segment));
                    xmlFree(container.part_array[i].name);
                }
#endif

    return helper.rawBuffer.state.err;
}

opcZipInputStream *opcZipOpenInputStream(opcZip *zip, opc_uint32_t segment_id) {
    OPC_ASSERT(segment_id>=0 && segment_id<zip->segment_items);
    opcZipInputStream *stream=(opcZipInputStream *)xmlMalloc(sizeof(opcZipInputStream));
    if (NULL!=stream) {        
        opc_bzero_mem(stream, sizeof(*stream));
        stream->segment_id=segment_id;
        opcZipSegment *segment=&zip->segment_array[segment_id];
        stream->rawBuffer.state.buf_pos=segment->stream_ofs+segment->padding+segment->header_size;
        if (OPC_ERROR_NONE!=opcZipInitInflateState(&stream->rawBuffer.state, 
                                                   segment->compressed_size, 
                                                   segment->uncompressed_size, 
                                                   segment->compression_method, 
                                                   &stream->inflateState)) {
            // error
            xmlFree(stream); stream=NULL;
        }
    }
    return stream;
}

opc_error_t opcZipCloseInputStream(opcZip *zip, opcZipInputStream *stream) {
    OPC_ASSERT(NULL!=zip && NULL!=stream);
    OPC_ASSERT(stream->segment_id>=0 && stream->segment_id<zip->segment_items);
    opcZipSegment *segment=&zip->segment_array[stream->segment_id];
    opc_error_t err=opcZipCleanupInflateState(&stream->rawBuffer.state, 
                                              segment->compressed_size, 
                                              segment->uncompressed_size, 
                                              &stream->inflateState);
    return err;
}

opc_uint32_t opcZipReadInputStream(opcZip *zip, opcZipInputStream *stream, opc_uint8_t *buf, opc_uint32_t buf_len) {
    OPC_ASSERT(NULL!=zip && NULL!=stream);
    OPC_ENSURE(stream->rawBuffer.state.buf_pos+stream->rawBuffer.buf_len-stream->rawBuffer.buf_ofs==_opcZipFileSeek(zip->io, stream->rawBuffer.state.buf_pos+stream->rawBuffer.buf_len-stream->rawBuffer.buf_ofs, opcFileSeekSet));
    OPC_ASSERT(zip->io->state.buf_pos==stream->rawBuffer.state.buf_pos+stream->rawBuffer.buf_len-stream->rawBuffer.buf_ofs);

//    OPC_ENSURE(stream->rawBuffer.state.buf_pos==_opcZipFileSeek(zip->io, stream->rawBuffer.state.buf_pos, opcFileSeekSet));
    opc_uint32_t ret=opcZipRawReadFileData(zip->io, &stream->rawBuffer, &stream->inflateState, buf, buf_len);
    return ret;
}

static opc_uint32_t opcZipAppendSegmentEx(opcZip *zip, 
                                   opc_ofs_t stream_ofs,
                                   opc_ofs_t segment_size,
                                   opc_uint16_t padding,
                                   opc_uint32_t header_size,
                                   opc_uint16_t bit_flag,
                                   opc_uint32_t crc32,
                                   opc_uint16_t compression_method,
                                   opc_ofs_t compressed_size,
                                   opc_ofs_t uncompressed_size,
                                   opc_uint32_t growth_hint,
                                   const xmlChar *partName,
                                   opc_bool_t relsSegment) {
    opc_uint32_t segment_id=-1;
    opcZipSegment *segment=ensureSegment(zip);
    if (NULL!=segment) {
        segment_id=zip->segment_items++;
        opc_bzero_mem(segment, sizeof(*segment));
        segment->stream_ofs=stream_ofs;
        segment->padding=padding;
        segment->header_size=header_size;
        segment->segment_size=segment_size;
        segment->bit_flag=bit_flag;
        segment->crc32=crc32;
        segment->compression_method=compression_method;
        segment->compressed_size=compressed_size;
        segment->uncompressed_size=uncompressed_size;
        segment->growth_hint=growth_hint;
        segment->partName=partName;
        segment->rels_segment=(relsSegment?1:0);
        segment->next_segment_id=-1;
    }
    return segment_id;
}

opc_uint32_t opcZipLoadSegment(opcZip *zip, const xmlChar *partName, opc_bool_t rels_segment, opcZipSegmentInfo_t *info) {
    opc_uint32_t ret=opcZipAppendSegmentEx(zip, 
                                           info->stream_ofs,
                                           info->header_size+info->compressed_size+info->trailing_bytes,
                                           info->header_size-info->min_header_size,
                                           info->min_header_size, 
                                           info->bit_flag,
                                           info->data_crc,
                                           info->compression_method,
                                           info->compressed_size,
                                           info->uncompressed_size,
                                           info->growth_hint,
                                           partName, 
                                           rels_segment);
    return ret;
}

opc_uint32_t opcZipCreateSegment(opcZip *zip, 
                                 const xmlChar *partName, 
                                 opc_bool_t relsSegment, 
                                 opc_uint32_t segment_size, 
                                 opc_uint32_t growth_hint,
                                 opc_uint16_t compression_method,
                                 opc_uint16_t bit_flag) {
    OPC_ASSERT(0==compression_method || 8==compression_method); // either STORE or DEFLATE
    OPC_ASSERT(8!=compression_method || (0<<1==bit_flag || 1<<1==bit_flag || 2<<1==bit_flag || 3<<1==bit_flag)); // WHEN DELFATE set bit_flag to NORMAL, MAXIMUM, FAST or SUPERFAST compression
    opc_uint32_t segment_id=-1;
    //@TODO find free segment
    if (-1==segment_id) {
        opc_ofs_t stream_ofs=(zip->segment_items>0?zip->segment_array[zip->segment_items-1].stream_ofs+zip->segment_array[zip->segment_items-1].segment_size:0);
        opc_uint32_t _growth_hint=(growth_hint>0?growth_hint:OPC_DEFAULT_GROWTH_HINT);
        opc_ofs_t _segment_size=(segment_size>0?segment_size:_growth_hint);
        char name8[OPC_MAX_PATH];
        opc_uint16_t name8_len=opcHelperAssembleSegmentName(name8, sizeof(name8), partName, 0, -1, relsSegment, NULL);
        opc_uint32_t header_size=opcZipCalculateHeaderSize(name8, name8_len, OPC_TRUE, NULL);
        if (OPC_ERROR_NONE==_opcZipFileGrow(zip->io, stream_ofs+_segment_size)) {
            segment_id=opcZipAppendSegmentEx(zip, stream_ofs, _segment_size, 0, header_size, bit_flag, 0, compression_method, 0, 0, _growth_hint, partName, relsSegment);
        }
    }
    return segment_id;
}

opc_error_t opcZipGC(opcZip *zip) {
    for(opc_uint32_t i=1;i<zip->segment_items;i++) { 
        OPC_ASSERT(zip->segment_array[i-1].stream_ofs+zip->segment_array[i-1].segment_size==zip->segment_array[i].stream_ofs);
        if (!zip->segment_array[i-1].deleted_segment && !zip->segment_array[i].deleted_segment) {
            opcZipSegment *segment=&zip->segment_array[i];
            char name8[OPC_MAX_PATH];
            opc_uint16_t name8_len=opcHelperAssembleSegmentName(name8, sizeof(name8), segment->partName,  0, -1, segment->rels_segment, NULL);
            name8[name8_len]=0;
            opc_uint32_t header_size=opcZipCalculateHeaderSize(name8, name8_len, OPC_TRUE, NULL);
            if (header_size>segment->header_size) {
                header_size=opcZipCalculateHeaderSize(name8, name8_len, OPC_FALSE, NULL);
            }
            OPC_ASSERT(header_size<=segment->header_size);
            opc_uint32_t const free_space=segment->padding+(segment->header_size-header_size);
            zip->segment_array[i-1].segment_size+=free_space;
            OPC_ASSERT(segment->padding<=segment->segment_size);
            segment->stream_ofs+=free_space;
            segment->segment_size-=segment->padding;
            segment->padding=0;
            segment->header_size=header_size;
        } 
        OPC_ASSERT(zip->segment_array[i-1].stream_ofs+zip->segment_array[i-1].segment_size==zip->segment_array[i].stream_ofs);
    }
    return OPC_ERROR_NONE;
}

static void opcZipSegmentCalcReal(opcZip *zip, opc_uint32_t segment_id, opc_ofs_t *real_padding, opc_ofs_t *real_ofs) {
    OPC_ASSERT(segment_id>=0 && segment_id<zip->segment_items);
    *real_padding=zip->segment_array[segment_id].padding;
    opc_uint32_t i=segment_id; 
    for(;i>0 && zip->segment_array[i-1].deleted_segment;i--) {
        *real_padding+=zip->segment_array[i-1].segment_size;
    }
    *real_ofs=zip->segment_array[i].stream_ofs;
    if (i>0) {
        OPC_ASSERT(!zip->segment_array[i-1].deleted_segment);
        opc_ofs_t trailing_space=zip->segment_array[i].stream_ofs
                                -zip->segment_array[i-1].stream_ofs
                                -zip->segment_array[i-1].padding
                                -zip->segment_array[i-1].header_size
                                -zip->segment_array[i-1].compressed_size;
        *real_ofs-=trailing_space;
        *real_padding+=trailing_space;
    }
}

static opc_bool_t opcZipValidate(opcZip *zip, opc_ofs_t *append_ofs) {
    opc_bool_t valid=OPC_TRUE;
    for(opc_uint32_t i=0;i<zip->segment_items;i++) { if (!zip->segment_array[i].deleted_segment) {
        opc_ofs_t real_padding=0;
        opc_ofs_t real_ofs=0;
        opcZipSegmentCalcReal(zip, i, &real_padding, &real_ofs);
        char name8[OPC_MAX_PATH];
        opc_uint16_t name8_len=opcHelperAssembleSegmentName(name8, sizeof(name8), zip->segment_array[i].partName,  0, -1, zip->segment_array[i].rels_segment, NULL);
        name8[name8_len]=0;
        opc_uint32_t header_size=opcZipCalculateHeaderSize(name8, name8_len, OPC_TRUE, NULL);
        valid=valid && (real_padding==0 || header_size<=zip->segment_array[i].header_size); // check padding>0 needs extra!
        valid=valid&&(real_padding<65000); //@TODO get real value for max padding!
        if (NULL!=append_ofs) *append_ofs=real_ofs+real_padding+zip->segment_array[i].compressed_size+zip->segment_array[i].header_size;
    } }
    return valid;
}

static void opcZipTrim(opcZip *zip, opc_ofs_t *append_ofs) {
    opc_ofs_t ofs=0;
    for(opc_uint32_t i=0;i<zip->segment_items;i++) { 
        if (!zip->segment_array[i].deleted_segment) {
            opc_ofs_t real_padding=0;
            opc_ofs_t real_ofs=0;
            opcZipSegmentCalcReal(zip, i, &real_padding, &real_ofs);
            OPC_ASSERT(ofs<=real_ofs);
            if (real_padding>0 || ofs<real_ofs) {
                opc_ofs_t src_ofs=real_ofs+real_padding+zip->segment_array[i].header_size;
                opc_ofs_t dest_ofs=ofs+zip->segment_array[i].header_size;
                opc_ofs_t len=zip->segment_array[i].compressed_size;
                OPC_ASSERT(dest_ofs<src_ofs);
                if (dest_ofs<src_ofs) {
                    OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileMove(zip->io, dest_ofs, src_ofs, len));
                }
            }
            zip->segment_array[i].stream_ofs=ofs;
            zip->segment_array[i].padding=0;
            zip->segment_array[i].segment_size=zip->segment_array[i].header_size+zip->segment_array[i].compressed_size;
            for(opc_uint32_t j=i;j>0 && zip->segment_array[j-1].deleted_segment;j--) {
                zip->segment_array[j-1].stream_ofs=ofs;
                zip->segment_array[j-1].segment_size=0; // make it a "ZOMBI" segment
                zip->segment_array[j-1].header_size=0; 
                zip->segment_array[j-1].padding=0;
            }
            ofs+=zip->segment_array[i].segment_size;            
        }
    }
    for(opc_uint32_t i=1;i<zip->segment_items;i++) {
        OPC_ASSERT(zip->segment_array[i-1].stream_ofs+zip->segment_array[i-1].segment_size==zip->segment_array[i].stream_ofs);
    }
    if (NULL!=append_ofs) *append_ofs=ofs;
}

static void opcZipUpdateLocalFileHeader(opcZip *zip) {
    for(opc_uint32_t i=0;i<zip->segment_items;i++) { if (!zip->segment_array[i].deleted_segment) {
        opc_ofs_t real_padding=0;
        opc_ofs_t real_ofs=0;
        opcZipSegmentCalcReal(zip, i, &real_padding, &real_ofs);
        OPC_ENSURE(_opcZipFileSeek(zip->io, real_ofs, opcFileSeekSet)==real_ofs);
        char name8[OPC_MAX_PATH];
        opc_uint16_t name8_len=opcHelperAssembleSegmentName(name8, sizeof(name8), zip->segment_array[i].partName, 0, -1, zip->segment_array[i].rels_segment, NULL);
//        opc_uint32_t header_size=opcZipCalculateHeaderSize(name8, name8_len, OPC_TRUE, NULL);
        OPC_ASSERT(zip->segment_array[i].stream_ofs+zip->segment_array[i].padding==real_ofs+real_padding);
        OPC_ENSURE(opcZipRawWriteSegmentHeaderEx(zip->io, &zip->io->state, 
                                                 name8, name8_len, 
                                                 zip->segment_array[i].bit_flag,
                                                 zip->segment_array[i].crc32,
                                                 zip->segment_array[i].compression_method,
                                                 zip->segment_array[i].compressed_size,
                                                 zip->segment_array[i].uncompressed_size,
                                                 zip->segment_array[i].header_size+real_padding,
                                                 zip->segment_array[i].growth_hint)==zip->segment_array[i].header_size+real_padding);
        OPC_ASSERT(zip->segment_array[i].stream_ofs+zip->segment_array[i].padding+zip->segment_array[i].header_size==zip->io->state.buf_pos);
    } }
}

static void opcZipAppendDirectory(opcZip *zip, opc_ofs_t append_ofs) {
    OPC_ENSURE(_opcZipFileSeek(zip->io, append_ofs, opcFileSeekSet)==append_ofs);
    opc_uint32_t real_segments=0;
    for(opc_uint32_t i=0;i<zip->segment_items;i++) { if (!zip->segment_array[i].deleted_segment) {
        opc_ofs_t real_padding=0;
        opc_ofs_t real_ofs=0;
        opcZipSegmentCalcReal(zip, i, &real_padding, &real_ofs);
        char name8[OPC_MAX_PATH];
        opc_uint16_t name8_len=opcHelperAssembleSegmentName(name8, sizeof(name8), zip->segment_array[i].partName, 0, -1, zip->segment_array[i].rels_segment, NULL);
        OPC_ENSURE(OPC_ERROR_NONE==opcZipRawWriteCentralDirectoryEx(zip->io, &zip->io->state,
                                                                    name8, name8_len, 
                                                                    zip->segment_array[i].bit_flag,
                                                                    zip->segment_array[i].crc32,
                                                                    zip->segment_array[i].compression_method,
                                                                    zip->segment_array[i].compressed_size,
                                                                    zip->segment_array[i].uncompressed_size,
                                                                    zip->segment_array[i].header_size+real_padding,
                                                                    zip->segment_array[i].growth_hint,
                                                                    real_ofs));
        real_segments++;
    } }
    OPC_ENSURE(OPC_ERROR_NONE==opcZipRawWriteEndOfCentralDirectoryEx(zip->io, &zip->io->state, append_ofs, real_segments));
    OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileTrim(zip->io, zip->io->state.buf_pos));
}

opc_error_t opcZipCommit(opcZip *zip, opc_bool_t trim) {
    opc_ofs_t append_ofs=0;
    if (!opcZipValidate(zip, &append_ofs) || trim) {
        opcZipTrim(zip, &append_ofs);
        OPC_ASSERT(opcZipValidate(zip, NULL));
    }
    opcZipUpdateLocalFileHeader(zip);
    opcZipAppendDirectory(zip, append_ofs);
    OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileFlush(zip->io));
    return zip->io->state.err;
}

opcZipOutputStream *opcZipOpenOutputStream(opcZip *zip, opc_uint32_t *segment_id) {
    OPC_ASSERT(NULL!=zip && NULL!=segment_id && -1!=*segment_id);
    OPC_ASSERT(*segment_id>=0 && *segment_id<zip->segment_items);
    opcZipSegment *segment=&zip->segment_array[*segment_id];
    OPC_ASSERT(segment->header_size+segment->padding<=segment->segment_size);
    opc_ofs_t free_size=segment->segment_size-segment->header_size-segment->padding;
    opc_uint32_t buf_size=(free_size>OPC_DEFLATE_BUFFER_SIZE?OPC_DEFLATE_BUFFER_SIZE:free_size);
    opcZipOutputStream *out=(opcZipOutputStream *)xmlMalloc(sizeof(opcZipOutputStream)+OPC_DEFLATE_BUFFER_SIZE);
    if (NULL!=out) {
        opc_bzero_mem(out, sizeof(*out));
        out->buf=(opc_uint8_t*)((&out->buf)+1); // buffer starts right after me...
        out->buf_size=buf_size;
        out->segment_id=*segment_id;
        *segment_id=-1; // take ownership
        segment->compressed_size=0;
        segment->uncompressed_size=0;
        segment->crc32=0;
        out->compression_method=segment->compression_method;
        OPC_ASSERT(0==out->compression_method || 8==out->compression_method);
        if (8==out->compression_method) { // delfate
            out->stream.zalloc = Z_NULL;
            out->stream.zfree = Z_NULL;
            out->stream.opaque = Z_NULL;
            if (Z_OK!=(out->inflate_state=deflateInit2(&out->stream, Z_BEST_SPEED, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY))) {
                xmlFree(out); out=NULL;
            }
        }
    }
    return out;
}

opcZipOutputStream *opcZipCreateOutputStream(opcZip *zip, 
                                             opc_uint32_t *segment_id, 
                                             const xmlChar *partName, 
                                             opc_bool_t relsSegment, 
                                             opc_uint32_t segment_size, 
                                             opc_uint32_t growth_hint,
                                             opc_uint16_t compression_method,
                                             opc_uint16_t bit_flag) {
    opcZipOutputStream *ret=NULL;
    OPC_ASSERT(NULL!=segment_id);
    if (NULL!=segment_id) {
        if (-1==*segment_id) {
            *segment_id=opcZipCreateSegment(zip, partName, relsSegment, segment_size, growth_hint, compression_method, bit_flag);
        } else {
            OPC_ASSERT(*segment_id>=0 && *segment_id<zip->segment_items);
            opcZipSegment *segment=&zip->segment_array[*segment_id];
            segment->bit_flag=bit_flag;
            segment->compression_method=compression_method;
        }
        ret=opcZipOpenOutputStream(zip, segment_id);
    }
    return ret;
}

static opc_uint32_t opcZipOutputStreamFill(opcZip *zip, opcZipOutputStream *stream, const opc_uint8_t *data, opc_uint32_t data_len) {
    OPC_ASSERT(NULL!=stream && NULL!=stream->buf && NULL!=data && stream->buf_ofs+stream->buf_len<=stream->buf_size);
    opc_uint32_t const free=stream->buf_size-stream->buf_ofs-stream->buf_len;
    opc_uint32_t const len=(free<data_len?free:data_len);
    opc_uint32_t ret=0;
    OPC_ASSERT(len<=data_len && len<=stream->buf_size-(stream->buf_ofs+stream->buf_len));
    if (len>0) {
        if (0==stream->compression_method) { // STORE
            stream->stream.total_in+=len;
            stream->stream.total_out+=len;
            stream->crc32=crc32(stream->crc32, data, len);
            memcpy(stream->buf+stream->buf_ofs, data, len);
            stream->buf_len+=len;
            ret=len;
        } else if (8==stream->compression_method) { // DEFLATE
            stream->stream.avail_in=data_len;
            stream->stream.next_in=(Bytef*)data;
            stream->stream.avail_out=free;
            stream->stream.next_out=stream->buf+stream->buf_ofs;
            if (Z_OK==(stream->inflate_state=deflate(&stream->stream, Z_NO_FLUSH))) {
                opc_uint32_t const bytes_in=data_len-stream->stream.avail_in;
                opc_uint32_t const bytes_out=free-stream->stream.avail_out;
                stream->crc32=crc32(stream->crc32, data, bytes_in);
                ret=bytes_in;
                stream->buf_len+=bytes_out;
            } else {                
                zip->io->state.err=OPC_ERROR_DEFLATE;
            }
        } else {
            zip->io->state.err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return ret;
}


static opc_bool_t opcZipOutputStreamFinishCompression(opcZip *zip, opcZipOutputStream *stream) {
    opc_bool_t ret=OPC_FALSE;
    if (OPC_ERROR_NONE==zip->io->state.err) {
        if (0==stream->compression_method) { // STORE
            OPC_ASSERT(Z_OK==stream->inflate_state);
            stream->inflate_state=Z_STREAM_END;
            ret=(Z_STREAM_END==stream->inflate_state);
        } else if (8==stream->compression_method) { // DEFLATE
            opc_uint32_t const free=stream->buf_size-stream->buf_ofs-stream->buf_len;
            OPC_ASSERT(free>0); // hmmm --- no space? make sure you correcly growed the segment...
            stream->stream.avail_in=0;
            stream->stream.next_in=0;
            stream->stream.avail_out=free;
            stream->stream.next_out=stream->buf+stream->buf_ofs;
            if (Z_OK==(stream->inflate_state=deflate(&stream->stream, Z_FINISH)) || Z_STREAM_END==stream->inflate_state) {
                opc_uint32_t const bytes_out=free-stream->stream.avail_out;
                stream->buf_len+=bytes_out;
                ret=(Z_STREAM_END==stream->inflate_state);
            } else {
                zip->io->state.err=OPC_ERROR_DEFLATE;
            }
        } else {
            zip->io->state.err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
        }
    }
    return ret;
}

static void opcZipMarkSegmentDeleted(opcZip *zip, opc_uint32_t segment_id, opcZipSegmentReleaseCallback* releaseCallback) {
    OPC_ASSERT(segment_id>=0 && segment_id<zip->segment_items);
    opcZipSegment *segment=&zip->segment_array[segment_id];
    if (NULL!=releaseCallback) releaseCallback(zip, segment_id);
    segment->deleted_segment=1;
    segment->partName=NULL; // should have been released in "releaseCallback" above
    segment->next_segment_id=zip->first_free_segment_id;
    zip->first_free_segment_id=segment_id;
}

static void opcZipOutputStreamFlushAndGrow(opcZip *zip, opcZipOutputStream *stream) {
    opc_error_t err=OPC_ERROR_NONE;
    if (stream->buf_len>0 && OPC_ERROR_NONE==zip->io->state.err) {
        OPC_ASSERT(stream->segment_id>=0 && stream->segment_id<zip->segment_items);
        opcZipSegment *segment=&zip->segment_array[stream->segment_id];
        opc_ofs_t ofs=segment->padding+segment->header_size+segment->compressed_size;
        OPC_ASSERT(ofs<=segment->segment_size);
        opc_ofs_t free_space=segment->segment_size-ofs;
        if (stream->buf_len>free_space && stream->segment_id+1==zip->segment_items) {
            // not enoght space and last segment => simply grow it...
            segment->growth_hint=(segment->growth_hint<=0?OPC_DEFAULT_GROWTH_HINT:segment->growth_hint);
            OPC_ASSERT(segment->growth_hint>0);
            while(segment->segment_size-ofs<stream->buf_len) segment->segment_size+=segment->growth_hint;
            free_space=segment->segment_size-ofs;
            OPC_ASSERT(stream->buf_len<=free_space);
            OPC_ENSURE(OPC_ERROR_NONE==_opcZipFileGrow(zip->io, segment->stream_ofs+segment->segment_size));
        }
        if (stream->buf_len>free_space) {
            // can't grow it, so move to a new segment!
            opc_ofs_t size_needed=segment->segment_size+stream->buf_len+segment->header_size;
            opc_uint32_t new_segment_id=opcZipCreateSegment(zip, segment->partName, segment->rels_segment, size_needed, segment->growth_hint, segment->compression_method, segment->bit_flag);
            segment=&zip->segment_array[stream->segment_id]; // recalc segment, since create can realloc base address
            if (OPC_ERROR_NONE==err && -1!=new_segment_id) {
                OPC_ASSERT(new_segment_id>=0 && new_segment_id<zip->segment_items);
                opcZipSegment *new_segment=&zip->segment_array[new_segment_id];
                opc_ofs_t const free_size=new_segment->segment_size-new_segment->header_size-new_segment->padding;
                OPC_ASSERT(segment->compressed_size<free_size);
                err=_opcZipFileMove(zip->io,
                                    new_segment->stream_ofs+new_segment->padding+new_segment->header_size, // dest
                                    segment->stream_ofs+segment->padding+segment->header_size,  // src
                                    segment->compressed_size);
                new_segment->compressed_size=segment->compressed_size;
                segment->partName=NULL; // ownership transfered to new_segment
                opcZipMarkSegmentDeleted(zip, stream->segment_id, NULL /* no release needed, since partName is copied to new segment */);
                OPC_ASSERT(1==segment->deleted_segment);
                segment=new_segment;
                stream->segment_id=new_segment_id;
                opc_uint32_t buf_size=(free_size>OPC_DEFLATE_BUFFER_SIZE?OPC_DEFLATE_BUFFER_SIZE:free_size);
                OPC_ASSERT(stream->buf_size<=buf_size);
                stream->buf_size=buf_size;
                ofs=segment->padding+segment->header_size+segment->compressed_size;
                free_space=segment->segment_size-ofs;
                OPC_ASSERT(stream->buf_len<=free_space);
            }
        }
        if (stream->buf_len<=free_space) {
            // enought free space in the current segment
            OPC_ENSURE(_opcZipFileSeek(zip->io, segment->stream_ofs+ofs, opcFileSeekSet)==segment->stream_ofs+ofs);
            OPC_ENSURE(_opcZipFileWrite(zip->io, stream->buf+stream->buf_ofs, stream->buf_len)==stream->buf_len);
            segment->compressed_size+=stream->buf_len;
            stream->buf_ofs=0;
            stream->buf_len=0;
        } else {
            OPC_ASSERT(0); // should not happend! => can't get enought space!
            err=OPC_ERROR_STREAM; 
        }
        if (OPC_ERROR_NONE!=err && OPC_ERROR_NONE==zip->io->state.err) {
            zip->io->state.err=err;
        }
    }    
}

static void opcZipOutputStreamFinishSegment(opcZip *zip, opcZipOutputStream *stream) {
    opc_bool_t done=OPC_FALSE;
    while(!done && OPC_ERROR_NONE==zip->io->state.err) {
        done=opcZipOutputStreamFinishCompression(zip, stream);
        opcZipOutputStreamFlushAndGrow(zip, stream);
    }
}


opc_error_t opcZipCloseOutputStream(opcZip *zip, opcZipOutputStream *stream, opc_uint32_t *segment_id) {
    OPC_ASSERT(NULL!=zip && NULL!=segment_id && -1==*segment_id);
    opcZipOutputStreamFinishSegment(zip, stream);
    OPC_ASSERT(stream->segment_id>=0 && stream->segment_id<zip->segment_items);
    opcZipSegment *segment=&zip->segment_array[stream->segment_id];
    *segment_id=stream->segment_id;
    OPC_ASSERT(segment->compressed_size==stream->stream.total_out);
    segment->uncompressed_size=stream->stream.total_in;
    segment->crc32=stream->crc32;
    xmlFree(stream); stream=NULL;
    return zip->io->state.err;
}

opc_uint32_t opcZipWriteOutputStream(opcZip *zip, opcZipOutputStream *stream, const opc_uint8_t *buf, opc_uint32_t buf_len) {
    opc_uint32_t out=0;
    do {
        out+=opcZipOutputStreamFill(zip, stream, buf+out, buf_len-out);
        OPC_ASSERT(out<=buf_len);
        opcZipOutputStreamFlushAndGrow(zip, stream);
    } while (out<buf_len);
    return out;
}

opc_uint32_t opcZipGetFirstSegmentId(opcZip *zip) {
    opc_uint32_t i=0;
    while(i<zip->segment_items && zip->segment_array[i].deleted_segment) i++;
    return (i<zip->segment_items?i:-1);
}

opc_uint32_t opcZipGetNextSegmentId(opcZip *zip, opc_uint32_t segment_id) {
    OPC_ASSERT(segment_id>=0 && segment_id<zip->segment_items);
    if (segment_id>=0 && segment_id<zip->segment_items) {
        opc_uint32_t i=segment_id+1;
        while(i<zip->segment_items && zip->segment_array[i].deleted_segment) i++;
        return (i<zip->segment_items?i:-1);
    } else {
        return -1;
    }
}

opc_error_t opcZipGetSegmentInfo(opcZip *zip, opc_uint32_t segment_id, const xmlChar **name, opc_bool_t *rels_segment, opc_uint32_t *crc) {
    OPC_ASSERT(segment_id>=0 && segment_id<zip->segment_items);
    if (NULL!=name) {
        *name=zip->segment_array[segment_id].partName;
    }
    if (NULL!=rels_segment) {
        *rels_segment=zip->segment_array[segment_id].rels_segment;
    }
    if (NULL!=crc) {
        *crc=zip->segment_array[segment_id].crc32;
    }
    return OPC_ERROR_NONE;
}


opc_bool_t opcZipSegmentDelete(opcZip *zip, opc_uint32_t *first_segment, opc_uint32_t *last_segment, opcZipSegmentReleaseCallback* releaseCallback) {
    OPC_ASSERT(NULL==last_segment || *first_segment==*last_segment); // not not implemented... needed for fragmented containers
    OPC_ASSERT(*first_segment>=0 && *first_segment<zip->segment_items);
    opc_bool_t ret=OPC_FALSE;
    opc_uint32_t segment_id=*first_segment;
    while(segment_id>=0 && segment_id<zip->segment_items) {
        opc_uint32_t next_segment_id=zip->segment_array[segment_id].next_segment_id;
        opcZipMarkSegmentDeleted(zip, segment_id, releaseCallback);
        segment_id=next_segment_id;
    }
    if (NULL!=last_segment) {
        OPC_ASSERT(*last_segment=segment_id);
        *last_segment=-1;
    }
    *first_segment=-1;
    return ret;
}
