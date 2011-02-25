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

struct OPC_ZIP_STRUCT {
	opcZipReadCallback *ioread;
	opcZipCloseCallback *ioclose;
	opcZipSeekCallback *ioseek;
	void *iocontext;
	int flags;
	int read_state; // state of last read action
	opc_uint32_t header_signature;
	opc_uint16_t version_needed;
	opc_uint16_t bit_flag;
	opc_uint16_t compression_method;
	opc_uint16_t last_mod_time;
	opc_uint16_t last_mod_date;
	opc_uint32_t crc32;
	opc_uint32_t compressed_size;
	opc_uint32_t uncompressed_size;
	opc_uint16_t filename_length;
	opc_uint16_t extra_length;
	opc_uint16_t comment_length;
	opc_uint32_t rel_ofs;
	opc_uint8_t filename[260];
};

static int opcZipFileRead(void *iocontext, char *buffer, int len) {
	return fread(buffer, sizeof(char), len, (FILE*)iocontext);
}

static int opcZipFileClose(void *iocontext) {
	return fclose((FILE*)iocontext);
}

static opc_ofs_t opcZipFileSeek(void *iocontext, opc_ofs_t ofs, opcZipSeekMode whence) {
	int ret=fseek((FILE*)iocontext, ofs, whence);
	if (ret>=0) {
		return ftell((FILE*)iocontext);
	} else {
		return ret;
	}
}

static inline int opcZipReadU8(opcZip *zip, opc_uint8_t *val) {
	return zip->ioread(zip->iocontext, (char*)val, sizeof(*val));
}

static inline int opcZipRead8(opcZip *zip, opc_int8_t *val) {
	return zip->ioread(zip->iocontext, (char*)val, sizeof(*val));
}

static inline int opcZipReadString(opcZip *zip, opc_uint8_t *buffer, int buffer_size, int len) {
	int ret=zip->ioread(zip->iocontext, (char*)buffer, len<buffer_size?len:buffer_size);
	if (ret>=0 && ret<buffer_size) buffer[ret]=0; else buffer[buffer_size-1]=0;
	return ret;
}

static inline int opcZipSkipBytes(opcZip *zip, int len) {
	if (NULL==zip->ioseek) {
		int ret=0;
		int i=0;
		opc_uint8_t val;
		while(i<len && (1==(ret=opcZipReadU8(zip, &val)))) i++;
		return (i==len?i:ret);
	} else {
		opc_ofs_t ofs=zip->ioseek(zip->iocontext, len, opcZipSeekCur);
		return (ofs>=0?len:0);
	}
}

#define OPC_READ_LITTLE_ENDIAN(zip, type, val) \
	opc_uint8_t aux;\
	int ret;\
	int i=0;\
	if (NULL!=val) {\
		*val=0;\
	}\
	while(i<sizeof(type) && (1==(ret=opcZipReadU8(zip, &aux)))) {\
		if (NULL!=val) {\
			*val|=((type)aux)<<(i<<3);\
		}\
		i++;\
	}\
	return (i==sizeof(type)?i:ret);\

static inline int opcZipReadU64(opcZip *zip, opc_uint64_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_uint64_t, val);
}

static inline int opcZipReadU32(opcZip *zip, opc_uint32_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_uint32_t, val);
}

static inline int opcZipReadU16(opcZip *zip, opc_uint16_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_uint16_t, val);
}

static inline int opcZipRead64(opcZip *zip, opc_int64_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_int64_t, val);
}

static inline int opcZipRead32(opcZip *zip, opc_int32_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_int32_t, val);
}

static inline int opcZipRead16(opcZip *zip, opc_int16_t *val) {
	OPC_READ_LITTLE_ENDIAN(zip, opc_int16_t, val);
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
		ret=opcZipOpenIO(opcZipFileRead, opcZipFileClose, opcZipFileSeek, file, flags);
	}
	return ret;
}

opcZip *opcZipOpenIO(opcZipReadCallback *ioread,
					 opcZipCloseCallback *ioclose,
					 opcZipSeekCallback *ioseek,
					 void *iocontext,
					 int flags) {
	opcZip *zip=(opcZip*)xmlMalloc(sizeof(opcZip));
	memset(zip, 0, sizeof(*zip));
	zip->ioread=ioread;
	zip->ioclose=ioclose;
	zip->ioseek=ioseek;
	zip->iocontext=iocontext;
	zip->flags=flags;
	zip->read_state=opcZipReadU32(zip, &zip->header_signature); // read first header signature
	return zip;
}

void opcZipClose(opcZip *zip) {
	if (NULL!=zip) {
		OPC_ENSURE(0==zip->ioclose(zip->iocontext));
		xmlFree(zip);
	}
}

opc_error_t opcZipReadDirectoryFileHeader(opcZip *zip) {
	opc_error_t err=OPC_ERROR_HEADER;
	if (NULL!=zip && 0x02014b50==zip->header_signature) {
        err=OPC_ERROR_STREAM;
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // version made by
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->version_needed)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->bit_flag)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->compression_method)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->last_mod_time)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->last_mod_date)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->crc32)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->compressed_size)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->uncompressed_size)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->filename_length)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->extra_length)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->comment_length)))
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // disk number start
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // internal file attributes
		if (4==(zip->read_state=opcZipReadU32(zip, NULL))) // external file attributes
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->rel_ofs)))
		if (zip->filename_length==(zip->read_state=opcZipReadString(zip, zip->filename, sizeof(zip->filename), zip->filename_length))) {
			while(zip->read_state>0 && zip->extra_length>0) {
				opc_uint16_t extra_id;
				opc_uint16_t extra_size;
				if (2==(zip->read_state=opcZipReadU16(zip, &extra_id))
					&& 2==(zip->read_state=opcZipReadU16(zip, &extra_size))) {
						switch(extra_id) {
						default: // just ignore
							opc_logf("**ignoring extra data id=%02X size=%02X\n", extra_id, extra_size);
							// no break;
						case 0xa220: // Microsoft Open Packaging Growth Hint => ignore
							if (extra_size==(zip->read_state=opcZipSkipBytes(zip, extra_size))) {
								zip->extra_length-=4+extra_size;
							}
							break;
						}
				}
			}
			if (zip->read_state>0
                && 0==zip->extra_length
                && (zip->comment_length>0 || zip->comment_length==opcZipSkipBytes(zip, zip->comment_length))) {
				err=OPC_ERROR_NONE;
			}
		}
	}
	return err;
}

opc_error_t opcZipNextDirectoyFileHeader(opcZip *zip) {
	opc_error_t err=OPC_ERROR_NONE;
	if (NULL!=zip) {
		zip->read_state=opcZipReadU32(zip, &zip->header_signature); // read next header signature
		err=(zip->read_state>=0?OPC_ERROR_NONE:OPC_ERROR_STREAM);
	} 
	return err;
}

opc_error_t opcZipReadLocalFileHeader(opcZip *zip) {
	opc_error_t ret=OPC_ERROR_HEADER;
	if (NULL!=zip && 0x04034b50==zip->header_signature) {
        ret=OPC_ERROR_STREAM;
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->version_needed)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->bit_flag)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->compression_method)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->last_mod_time)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->last_mod_date)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->crc32)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->compressed_size)))
		if (4==(zip->read_state=opcZipReadU32(zip, &zip->uncompressed_size)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->filename_length)))
		if (2==(zip->read_state=opcZipReadU16(zip, &zip->extra_length)))
		if (zip->filename_length==(zip->read_state=opcZipReadString(zip, zip->filename, sizeof(zip->filename), zip->filename_length))) {
			while(zip->read_state>0 && zip->extra_length>0) {
				opc_uint16_t extra_id;
				opc_uint16_t extra_size;
				if (2==(zip->read_state=opcZipReadU16(zip, &extra_id))
					&& 2==(zip->read_state=opcZipReadU16(zip, &extra_size))) {
						switch(extra_id) {
						default: // just ignore
							opc_logf("**ignoring extra data id=%02X size=%02X\n", extra_id, extra_size);
							// no break;
						case 0xa220: // Microsoft Open Packaging Growth Hint => ignore
							if (extra_size==(zip->read_state=opcZipSkipBytes(zip, extra_size))) {
								zip->extra_length-=4+extra_size;
							}
							break;
						}
				}
				
			}
            if (zip->read_state>0 && 0==zip->extra_length) {
                ret=OPC_ERROR_NONE;
            }
		}
	}
    if (OPC_ERROR_NONE==ret && 0x8==(zip->bit_flag&0x8)) {
        // we do not support streamed zip files
        ret=OPC_ERROR_UNSUPPORTED_DATA_DESCRIPTOR;
    }
	return ret;
}

opc_error_t opcZipInitDeflateStream(opcZipPartInfo *partInfo, opcZipDeflateStream *stream) {
	memset(stream, 0, sizeof(*stream));
	stream->stream.zalloc = Z_NULL;
	stream->stream.zfree = Z_NULL;
	stream->stream.opaque = Z_NULL;
	stream->stream.next_in = Z_NULL;
	stream->stream.avail_in = 0;
	stream->stream_size=partInfo->stream_compressed_size;
	stream->stream_ofs=partInfo->stream_ofs; 
	stream->compression_method=partInfo->compression_method;
	return 8==stream->compression_method || 0==stream->compression_method ? OPC_ERROR_NONE : OPC_ERROR_UNSUPPORTED_COMPRESSION; // either DEFLATE or STORE
}

opc_error_t opcZipOpenDeflateStream(opcZipPartInfo *partInfo, opcZipDeflateStream *stream) {
	if (8==stream->compression_method) {
		return Z_OK==inflateInit2(&stream->stream, -MAX_WBITS) ? OPC_ERROR_NONE : OPC_ERROR_DEFLATE;
	} else if (0==stream->compression_method) {
		return OPC_ERROR_NONE;
	} else {
		// unsupported compression
		return OPC_ERROR_UNSUPPORTED_COMPRESSION;
	}
}
static inline int _opcZipFillBuffer(opcZip *zip, opcZipDeflateStream *stream, opc_error_t *err) {
	OPC_ASSERT(stream->stream.avail_in<=sizeof(stream->buf));
	if ((NULL==err || OPC_ERROR_NONE==*err) &&
        Z_OK==stream->stream_state && 
		0==stream->stream.avail_in &&
		stream->stream.total_in+stream->stream.avail_in<stream->stream_size) {
		OPC_ASSERT(stream->stream.total_in<=stream->stream_size);
		if (NULL!=zip->ioseek) {
			OPC_ENSURE(stream->stream_ofs+stream->stream.total_in==zip->ioseek(zip->iocontext, stream->stream_ofs+stream->stream.total_in, opcZipSeekSet));
		}
		OPC_ASSERT(NULL==zip->ioseek || zip->ioseek(zip->iocontext, 0, opcZipSeekCur)==stream->stream_ofs+stream->stream.total_in);
		int const max_stream=stream->stream_size-stream->stream.total_in;
		int const max_buf=sizeof(stream->buf)-stream->stream.avail_in;
		int const max=(max_buf<max_stream?max_buf:max_stream);
		stream->stream.next_in=(Bytef*)stream->buf;
		int ret=zip->ioread(zip->iocontext, ((char*)stream->stream.next_in), max);
		if (ret>0) {
			OPC_ASSERT(stream->stream.avail_in<=sizeof(stream->buf));
			stream->stream.avail_in=ret;
			OPC_ASSERT(stream->stream.avail_in<=sizeof(stream->buf));
            if (NULL!=*err) *err=OPC_ERROR_NONE;
		} else if (0==ret) {
			stream->stream_state=Z_STREAM_END;
            if (NULL!=*err) *err=OPC_ERROR_NONE;
		} else {
			stream->stream_state=Z_STREAM_ERROR;
            if (NULL!=*err) *err=OPC_ERROR_STREAM;
		}
	}
	OPC_ASSERT(stream->stream.avail_in<=sizeof(stream->buf));
	return (Z_OK==stream->stream_state?stream->stream.avail_in:0);
}

static inline int _opcZipInflateBuffer(opcZip *zip, opcZipDeflateStream *stream, char *buf, int len, opc_error_t *err) {
	int ret=0;
    if (NULL==err || OPC_ERROR_NONE==*err) {
        stream->stream.next_out=(Bytef*)buf;
        stream->stream.avail_out=len;
        if (Z_OK==stream->inflate_state 
            && (Z_OK==(stream->inflate_state=inflate(&stream->stream, Z_SYNC_FLUSH)) || Z_STREAM_END==stream->inflate_state)) {
                OPC_ASSERT(stream->stream.avail_out<=(uInt)len);
                ret=len-stream->stream.avail_out;
        } else {
            if (NULL!=err && OPC_ERROR_NONE==*err) {
                *err=OPC_ERROR_DEFLATE;
            }
        }
    }
	return ret;
}

int opcZipReadDeflateStream(opcZip *zip, opcZipDeflateStream *stream, char *buf, int len, opc_error_t *err) {
	int ofs=0;
    if (NULL==err || *err==OPC_ERROR_NONE) {
        if (8==stream->compression_method) {
            int bytes=0;
            while((NULL==err || OPC_ERROR_NONE==*err)
                  && ofs<len 
                  && _opcZipFillBuffer(zip, stream, err)>0 
                  && (NULL==err || OPC_ERROR_NONE==*err)
                  && (bytes=_opcZipInflateBuffer(zip, stream, buf+ofs, len-ofs, err))>0
                  && (NULL==err || OPC_ERROR_NONE==*err)) {
                ofs+=bytes;
            }
        } else {
            int const max_stream=stream->stream_size-stream->stream.total_in;
            int const max=(len<max_stream?len:max_stream);
            int bytes=zip->ioread(zip->iocontext, buf, max);
            if (bytes>0) {
                ofs=bytes;
                stream->stream.total_in+=bytes;
                stream->stream.total_out+=bytes;
            } else {
                ofs=0;
                if ((NULL==err || OPC_ERROR_NONE==*err) && bytes<0) {
                    *err=OPC_ERROR_STREAM;
                }
            }
        }
    } else {
        if (NULL!=err) *err=OPC_ERROR_UNSUPPORTED_COMPRESSION;
    }
	return ofs;
}

opc_error_t opcZipCloseDeflateStream(opcZipPartInfo *partInfo, opcZipDeflateStream *stream) {
	OPC_ASSERT(stream->stream.total_in==stream->stream_size);
	OPC_ASSERT(stream->stream.total_in==partInfo->stream_compressed_size);
	OPC_ASSERT(stream->stream.total_out==partInfo->stream_uncompressed_size);
	if (8==stream->compression_method) {
		return Z_OK==inflateEnd(&stream->stream) ? OPC_ERROR_NONE : OPC_ERROR_DEFLATE;
	} else {
		return OPC_ERROR_NONE;
	}
}

opc_error_t opcZipSkipLocalFileData(opcZip *zip) {
	opc_error_t err=OPC_ERROR_STREAM;
	if (NULL!=zip && 0x04034b50==zip->header_signature && 0==zip->extra_length) {
        err=(opcZipSkipBytes(zip, zip->compressed_size)==zip->compressed_size?OPC_ERROR_NONE:OPC_ERROR_STREAM);
	}
	return err;
}



opc_error_t opcZipReadDataDescriptor(opcZip *zip) {
	opc_error_t err=OPC_ERROR_STREAM;
	if (NULL!=zip && 0x04034b50==zip->header_signature && 0==zip->extra_length) {
        if (zip->read_state>0) { 
            err=(4==(zip->read_state=opcZipReadU32(zip, &zip->header_signature))?OPC_ERROR_NONE:OPC_ERROR_STREAM);
        }
	}
	return err;
}

opc_error_t opcZipReadEndOfCentralDirectory(opcZip *zip) {
	opc_error_t err=OPC_ERROR_STREAM;
	opc_uint16_t comment_length=0;
	if (NULL!=zip && 0x06054b50==zip->header_signature) {
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // number of this disk
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // number of the disk with the start of the central directory  
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // total number of entries in the central directory on this disk
		if (2==(zip->read_state=opcZipReadU16(zip, NULL))) // total number of entries in the central directory 
		if (4==(zip->read_state=opcZipReadU32(zip, NULL))) // size of the central directory   
		if (4==(zip->read_state=opcZipReadU32(zip, NULL))) // offset of start of central directory with respect to the starting disk number
		if (2==(zip->read_state=opcZipReadU16(zip, &comment_length))) // .ZIP file comment len
		if (comment_length==(opcZipSkipBytes(zip, comment_length))) {
			char ch=0;
			err=(0==zip->ioread(zip->iocontext, &ch, sizeof(ch))?OPC_ERROR_NONE:OPC_ERROR_STREAM); // if not, then we have trailing bytes...
		}
	}
	return err;
}

opc_error_t opcZipInitPartInfo(opcZip *zip, opcZipPartInfo *partInfo) {
	opc_error_t err=OPC_ERROR_STREAM;
	if (NULL!=zip && 0x04034b50==zip->header_signature && NULL!=zip->ioseek) {
		memset(partInfo, 0, sizeof(*partInfo));
		partInfo->stream_ofs=zip->ioseek(zip->iocontext, 0, opcZipSeekCur);
		partInfo->partName=xmlStrdup(_X(zip->filename)); //@TODO DECODE!!!!
		partInfo->stream_compressed_size=zip->compressed_size;
		partInfo->stream_uncompressed_size=zip->uncompressed_size;
		partInfo->compression_method=zip->compression_method;
		err=(partInfo->partName!=NULL?OPC_ERROR_NONE:OPC_ERROR_STREAM);
	}
	return err;
}

opc_error_t opcZipCleanupPartInfo(opcZipPartInfo *partInfo) {
	if (NULL!=partInfo) {
		xmlFree(partInfo->partName);
	}
	return OPC_ERROR_NONE;
}

opc_error_t opcZipConsumedPartInCallback(opcZip *zip, opcZipPartInfo *partInfo) {
    OPC_ASSERT(zip->compressed_size==partInfo->stream_compressed_size);
    OPC_ASSERT(NULL==zip->ioseek || zip->ioseek(zip->iocontext, 0, opcZipSeekCur)==partInfo->stream_ofs+partInfo->stream_compressed_size); // make sure the stream is really consumed!
    zip->compressed_size=0; // consumed data locally
    return OPC_ERROR_NONE;
}


opc_error_t opZipScan(opcZip *zip, void *callbackCtx, opcZipPartInfoCallback *partInfoCallback) {
	// Parts in ZIP (according to local file infos)
	opc_error_t err=OPC_ERROR_NONE;
	while (OPC_ERROR_NONE==err && OPC_ERROR_NONE==(err=opcZipReadLocalFileHeader(zip))) {
		if (OPC_ERROR_NONE==err && NULL!=partInfoCallback) {
			err=partInfoCallback(callbackCtx, zip);
		}
        if (OPC_ERROR_NONE==err) err=opcZipSkipLocalFileData(zip);
        if (OPC_ERROR_NONE==err) err=opcZipReadDataDescriptor(zip);
	}
    if (OPC_ERROR_HEADER==err) {
        err=OPC_ERROR_NONE;
	    // Parts in ZIP (according to directory)
	    while(OPC_ERROR_NONE==err && OPC_ERROR_NONE==(err=opcZipReadDirectoryFileHeader(zip))) {
    		err=opcZipNextDirectoyFileHeader(zip);
    	}
        if (OPC_ERROR_HEADER==err) err=opcZipReadEndOfCentralDirectory(zip);
    }
	return err;
}


int opcZipWriteStart(opcZip *zip) {
	return 0;
}

int opcZipWriteEnd(opcZip *zip)  {
	return 0;
}

int opcZipWriteOpenPart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *partName, int growthHint)  {
	return 0;
}

int opcZipWritePartData(opcZip *zip, opcZipPartInfo *partInfo, const char *buf, int bufLen)  {
	return 0;
}

int opcZipWriteClosePart(opcZip *zip, opcZipPartInfo *partInfo, const xmlChar *newName)  {
	return 0;
}


int opcZipWriteStartDirectory(opcZip *zip) {
	return 0;
}

int opcZipWriteDirectoryEntry(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipWriteEndDirectory(opcZip *zip) {
	return 0;
}

int opcZipReadStart(opcZip *zip) {
	return 1;
}
/*
int opcZipReadPartInfo(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadSkipPart(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadEnd(opcZip *zip) {
	return 0;
}

int opcZipCleanupPartInfo(opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadDataStart(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo) {
	return 0;
}

int opcZipReadData(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo, char *buf, int bufLen) {
	return 0;
}

int opcZipReadDataEnd(opcZip *zip, opcZipPartInfo *partInfo, opcZipReadInfo *readInfo) {
	return 0;
}

int opcZipReadDirectoryStart(opcZip *zip) {
	return 0;
}

int opcZipReadDirectoryInfo(opcZip *zip, opcZipPartInfo *partInfo) {
	return 0;
}

int opcZipReadDirectoryEnd(opcZip *zip) {
	return 0;
}

int opcZipIsStreamMode(opcZip *zip) {
	return 0;
}

int opcZipMovePart(opcZip *zip, opcZipPartInfo *partInfo, int delta) {
	return 0;
}

int opcZipCopyPart(opcZip *zip, opcZipPartInfo *partInfo, opcZip *target) {
	return 0;
}

int opcZipSwapPart(opcZip *zip, opcZipPartInfo *partInfo, int minGapSize) {
	return 0;
}

int opcZipGetPhysicalPartSize(opcZipPartInfo *partInfo) {
	return 0;
}
*/