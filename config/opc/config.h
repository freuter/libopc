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
/**@file config/opc/config.h
 */
#ifndef OPC_CONFIG_H
#define OPC_CONFIG_H

#include <libxml/xmlstring.h>
#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif   

/**
 Converts an ASCII string to a xmlChar string. This only works for ASCII strings.
 */
#ifndef _X
#define _X(s) BAD_CAST(s) 
#endif
#ifndef _X2C
#define _X2C(s) ((char*)(s))
#endif

#define OPC_ASSERT(e) assert(e)
#define OPC_ENSURE(e) assert(e)

	typedef long opc_ofs_t;
	typedef unsigned char opc_uint8_t;
	typedef unsigned short opc_uint16_t;
	typedef unsigned int opc_uint32_t;
	typedef unsigned long opc_uint64_t;
	typedef char opc_int8_t;
	typedef short opc_int16_t;
	typedef int opc_int32_t;
	typedef long opc_int64_t;

#define OPC_DEFLATE_BUFFER_SIZE 1024

#ifdef __cplusplus
} /* extern "C" */
#endif  

#endif /* OPC_CONFIG_H */