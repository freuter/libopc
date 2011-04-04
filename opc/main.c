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

 This is the API documentation of the libopc project. 

 API headers can be found in the "Files" section.
 
 Samples can be found in the "Examples" section.
 
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


/** \example opc_zipread.c
 Demonstrates low level ZIP read functionality as needed by the high level opcContainer API.
 */

/** \example opc_zipextract.c
 Demonstrates low level ZIP read functionality as needed by the high level opcContainer API.
 */

/** \example opc_xml.c
 Demonstrates basic non-MCE XML read access.
 */

/** \example opc_xml2.c
 Demonstrates basic non-MCE XML read access via helper macros.
 */

/** \example mce_read.c
 Demonstrates basic MCE proprocessing.
 \todo MCE functionality is not implemented in this release, so this example is not yet running.
 */

/** \example mce_write.c
 Demonstrates basic MCE proprocessing.
 \todo MCE functionality is not implemented in this release, so this example is not yet running.
*/

/** \example opc_image.c
 Sample program which will extract all images from an OPC container.
 E.g. opc_dump hello.pptx will extract all pictures from "hello.pptx" in the current directory.
 The call opc_dump hello.pptx C:\Users\flr\Pictures will extract all pictures from "hello.pptx" in the directory "C:\Users\flr\Pictures".
 */

/** \example opc_mem.c
 Demonstrates the the use of \ref opcContainerOpenMem, i.e. how to use "in-memory" containers.
 */

/** \example opc_part.c
 Demonstrates how to dump a part from an OPC container. Ussage opc_dump [container] [part-name]. E.g. opc_dump sample.docx "word/document.xml".
 */

/** \example opc_relation.c
 Demonstrates how to traverse all relations in an OPC container using the API.
 */

/** \example opc_text.c
 Sample program which will extract all text form an Word document and dump it as HTML.
 */

/** \example opc_trim.c
 Opens an OPC containers and saves it back in "trimming" mode, which will reduce the size as much as possible.
 */

/** \example opc_type.c
 Demonstrate how to corretly get the type of an Office document.
 */

/** \example opc_generate.c
 Sample program which will read an OPC container and generate a "C" file which uses the API to generate the passed container.
 */


int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		printf("libopc as well as zlib and libxml2 are ready to use.\n");
		opcFreeLibrary();
		return 0;
	}  else  {
		printf("error initializing libopc.\n ");
		return 1;
	}
}

