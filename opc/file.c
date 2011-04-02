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
#include <opc/file.h>
#include <stdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/globals.h>

static void *opcFileOpen(const xmlChar *filename, int flags) {
    char mode[5];
    int mode_ofs=0;
    mode[mode_ofs++]='r';
    mode[mode_ofs++]='b';
    mode[mode_ofs++]='\0';
    FILE *file=fopen((const char*)filename, mode); // try to open in READ mode...
    if (flags & OPC_FILE_WRITE) {
        if (NULL==file) {
            // try to create one
            mode_ofs=0;
            mode[mode_ofs++]='w';
            mode[mode_ofs++]='+';
            mode[mode_ofs++]='b';
            mode[mode_ofs++]='\0';
            file=fopen((const char *)filename, mode); // try to open new file
        } else {
            fclose(file); // close the read handle...
            mode_ofs=0;
            mode[mode_ofs++]='r';
            mode[mode_ofs++]='+';
            mode[mode_ofs++]='b';
            mode[mode_ofs++]='\0';
            file=fopen((const char *)filename, mode); // try to open existing for read/write
        }
    }
    return file;
}

static int opcFileClose(void *iocontext) {
    return fclose((FILE*)iocontext);
}

static int opcFileRead(void *iocontext, char *buffer, int len) {
    return fread(buffer, sizeof(char), len, (FILE*)iocontext);
}

static int opcFileWrite(void *iocontext, const char *buffer, int len) {
    return fwrite(buffer, sizeof(char), len, (FILE*)iocontext);
}

static opc_ofs_t opcFileSeek(void *iocontext, opc_ofs_t ofs) {
    int ret=fseek((FILE*)iocontext, ofs, SEEK_SET);
    if (ret>=0) {
        return ftell((FILE*)iocontext);
    } else {
        return ret;
    }
}

static int opcFileTrim(void *iocontext, opc_ofs_t new_size) {
#ifdef P_WIN32
    return _chsize(fileno((FILE*)iocontext), new_size);
#else
    return ftruncate(fileno((FILE*)iocontext), new_size);
#endif
}

static int opcFileFlush(void *iocontext) {
    return fflush((FILE*)iocontext);
}

static opc_uint32_t opcFileLength(void *iocontext) {
    opc_ofs_t current=ftell((FILE*)iocontext);
    OPC_ENSURE(fseek((FILE*)iocontext, 0, SEEK_END)>=0);
    opc_ofs_t length=ftell((FILE*)iocontext);
    OPC_ENSURE(fseek((FILE*)iocontext, current, SEEK_SET)>=0);
    OPC_ASSERT(current==ftell((FILE*)iocontext));
    return length;
}


struct __opcZipMemContext {
    const opc_uint8_t *data;
    opc_uint32_t data_len;
    opc_uint32_t data_pos;    
};

static void *opcMemOpen(const opc_uint8_t *data, opc_uint32_t data_len) {
    struct __opcZipMemContext *mem=(struct __opcZipMemContext *)xmlMalloc(sizeof(struct __opcZipMemContext));
    memset(mem, 0, sizeof(*mem));
    mem->data_len=data_len;
    mem->data_pos=0;
    mem->data=data;
    return mem;
}

static int opcMemClose(void *iocontext) {
    struct __opcZipMemContext *mem=(struct __opcZipMemContext*)iocontext;
    xmlFree(mem);
    return 0;
}


static int opcMemRead(void *iocontext, char *buffer, int len) {
    struct __opcZipMemContext *mem=(struct __opcZipMemContext*)iocontext;
    opc_uint32_t max=(mem->data_pos+len<=mem->data_len?len:mem->data_len-mem->data_pos);
    OPC_ASSERT(max>=0 && mem->data_pos+max<=mem->data_len);
    memcpy(buffer, mem->data+mem->data_pos, max);
    mem->data_pos+=max;    
    return max;
}

static int opcMemWrite(void *iocontext, const char *buffer, int len) {
    OPC_ASSERT(0); // not valid for mem
    return -1;
}

static opc_ofs_t opcMemSeek(void *iocontext, opc_ofs_t ofs) {
    struct __opcZipMemContext *mem=(struct __opcZipMemContext*)iocontext;
    if (ofs<=mem->data_len) {
        mem->data_pos=ofs;
    } else {
        mem->data_pos=mem->data_len;
    }
    return mem->data_pos;
}

static int opcMemTrim(void *iocontext, opc_ofs_t new_size) {
    OPC_ASSERT(0); // not valid for mem
    return -1;
}

static int opcMemFlush(void *iocontext) {
    return 0;
}

opc_error_t opcFileInitIO(opcIO_t *io,
                          opcFileReadCallback *ioread,
                          opcFileWriteCallback *iowrite,
                          opcFileCloseCallback *ioclose,
                          opcFileSeekCallback *ioseek,
                          opcFileTrimCallback *iotrim,
                          opcFileFlushCallback *ioflush,
                          void *iocontext,
                          pofs_t file_size,
                          int flags) {
    opc_bzero_mem(io, sizeof(*io));
    io->_ioread=ioread;
    io->_iowrite=iowrite;
    io->_ioclose=ioclose;
    io->_ioseek=ioseek;
    io->_iotrim=iotrim;
    io->_ioflush=ioflush;
    io->iocontext=iocontext;
    io->file_size=file_size;
    io->flags=flags;
    return OPC_ERROR_NONE;
}

opc_error_t opcFileInitIOFile(opcIO_t *io, const xmlChar *filename, int flags) {
    opc_error_t ret=OPC_ERROR_NONE;
    void *iocontext=opcFileOpen(filename, flags);
    if (iocontext!=NULL) {
        ret=opcFileInitIO(io,
                          opcFileRead, 
                          opcFileWrite, 
                          opcFileClose, 
                          opcFileSeek, 
                          opcFileTrim, 
                          opcFileFlush,
                          iocontext, 
                          opcFileLength(iocontext), 
                          flags);
    } else {
        ret=OPC_ERROR_STREAM;
    }
    if (OPC_ERROR_NONE!=ret && OPC_ERROR_NONE==io->state.err) io->state.err=ret; // propagate error to stream
    return ret;
}

opc_error_t opcFileInitIOMemory(opcIO_t *io, const opc_uint8_t *data, opc_uint32_t data_len, int flags) {
    opc_error_t ret=OPC_ERROR_NONE;
    void *iocontext=opcMemOpen(data, data_len);
    if (iocontext!=NULL) {
        ret=opcFileInitIO(io, 
                          opcMemRead, 
                          opcMemWrite, 
                          opcMemClose, 
                          opcMemSeek, 
                          opcMemTrim, 
                          opcMemFlush, 
                          iocontext, 
                          data_len, 
                          flags);
    } else {
        ret=OPC_ERROR_STREAM;
    }
    if (OPC_ERROR_NONE!=ret && OPC_ERROR_NONE==io->state.err) io->state.err=ret; // propagate error to stream
    return ret;
}

opc_error_t opcFileCleanupIO(opcIO_t *io) {
    if (NULL!=io->iocontext) {
        io->_ioclose(io->iocontext);
        io->iocontext=NULL;
    }
    return OPC_ERROR_NONE;
}