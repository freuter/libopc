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
#include <stdio.h>

/** @mainpage libopc

 \section mission Mission Statement

 This is the preliminary API documentation of the libopc project. 
 Its main purpose it to visualize the API design and to help in milestone and time planing.
 
 Preliminary API headers can be found in the "Files" section.
 
 Samples referenced in milestone planing can be found in the "Examples" section.
 
 
 \todo{Add mission statement.}

 
 \section build_linux Building on Linux

 \todo{Add Linux building instructions.}
 
 \section build_windows Building on Windows 

 \todo{Add Windows building instructions.}

 */

/** \example opc_helloworld.c
 Demonstrates the the use of \ref opcInitLibrary and \ref opcFreeLibrary.
 */

/** \example opc_dump.c
 Demonstrates the the use of \ref opcContainerOpen, \ref opcContainerClose and \redf opcContainerDump.
 */

/** \example opc_extract.c
 Demonstrates binary input stream access to an OPC container.
 */

/** \example opc_zipwrite.c
 Demonstrates low level ZIP write functionality as needed by the high level opcContainer API.
 */

/** \example opc_zipwrite2.c
 Demonstrates low level ZIP write functionality as needed by the high level opcContainer API.
 */

/** \example opc_zipwrite3.c
 Demonstrates low level ZIP write functionality for streaming mode as needed by the high level opcContainer API.
 */

/** \example opc_zipwrite4.c
 Demonstrates low level ZIP write functionality for growth_hint parameter as needed by the high level opcContainer API.
 */

/** \example opc_zipwrite5.c
 Demonstrates low level ZIP write functionality for interleaved write operations as needed by the high level opcContainer API.
 */

/** \example opc_zipwrite6.c
 Demonstrates low level ZIP write functionality for interleaved write operations in streaming mode as needed by the high level opcContainer API.
 */

/** \example opc_zipread.c
 Demonstrates low level ZIP read functionality as needed by the high level opcContainer API.
 */

/** \example opc_zipread2.c
 Demonstrates low level ZIP read functionality in streaming mode as needed by the high level opcContainer API.
 */

/** \example opc_zipextract.c
 Demonstrates low level ZIP read functionality as needed by the high level opcContainer API.
 */

/** \example opc_zipextract2.c
 Demonstrates low level ZIP read functionality in streaming mode as needed by the high level opcContainer API.
 */

/** \example opc_zipdefrag.c
 Demonstrates low level ZIP read/write functionality needed for interleave stream defragmentation as needed by the high level opcContainer API.
 */

/** \example opc_zipcopy.c
 Demonstrates low level ZIP read/write functionality needed for template/transition modes as needed by the high level opcContainer API.
 */

/** \example opc_xml.c
 Demonstrates basic non-MCE XML read access.
 */

/** \example opc_xml2.c
 Demonstrates basic non-MCE XML read access via helper macros.
 */

/** \example mce_read.c
 Demonstrates basic MCE proprocessing.
 */

/** \example mce_write.c
 Demonstrates basic MCE proprocessing.
 */


int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		printf("libopc as well as zlib, libzip and libxml2 are ready to use.\n");
		opcFreeLibrary();
		return 0;
	}  else  {
		printf("error initializing libopc.\n ");
		return 1;
	}
}

