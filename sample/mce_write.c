/**
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
#include <mce/textwriter.h>
#include <stdio.h>

const char v1_ns[]="http://schemas.openxmlformats.org/Circles/v1";
const char v2_ns[]="http://schemas.openxmlformats.org/Circles/v2";
const char v3_ns[]="http://schemas.openxmlformats.org/Circles/v3";

/*
 Produces sample 10-3 from ISO-IEC-29500 Part 3:
 
 <Circles xmlns="http://schemas.openxmlformats.org/Circles/v1" 
		  xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" 
          xmlns:v2="http://schemas.openxmlformats.org/Circles/v2" 
          mc:Ignorable="v2" 
		  mc:ProcessContent="v2:Blink" > 
  <v2:Watermark Opacity="v0.1"> 
   <Circle Center="0,0" Radius="20" Color="Blue" /> 
   <Circle Center="25,0" Radius="20" Color="Black" /> 
   <Circle Center="50,0" Radius="20" Color="Red" /> 
  </v2:Watermark> 
  <v2:Blink> 
   <Circle Center="13,0" Radius="20" Color="Yellow" /> 
   <Circle Center="38,0" Radius="20" Color="Green" /> 
  </v2:Blink> 
 </Circles> 
 
 */

int main( int argc, const char* argv[] )
{
	if (opcInitLibrary()) {
		opcContainer *c=opcContainerOpen(_X("sample.zip"), OPC_OPEN_WRITE_ONLY, NULL, NULL);
		opcPart *part=opcPartOpen(c, _X("sample.txt"), NULL, OPC_PART_CREATE | OPC_PART_COMPRESSED);
		mceTextWriter *writer=mceTextWriterOpen(part);
		mceTextWriterStartDocument();
		mceTextWriterRegisterNamespace(_X(v1_ns), NULL, MCE_DEFAULT);
		mceTextWriterRegisterNamespace(_X(v2_ns), _X("v2"), MCE_IGNORABLE);
		mceTextWriterStartElement(_X(v1_ns), _X("Circles"));
		mceTextWriterProcessContent(_X(v2_ns), _X("Blink"));		
		
		mceTextWriterStartElement(_X(v2_ns), _X("Watermark"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Opacity"), "v0.1");	
		mceTextWriterStartElement(_X(v1_ns), _X("Circle"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Center"), "0,0");
		mceTextWriterAttributeF(_X(v1_ns), _X("Radius"), "20");
		mceTextWriterAttributeF(_X(v1_ns), _X("Color"), "Blue");
		mceTextWriterEndElement(); // Circle
		mceTextWriterStartElement(_X(v1_ns), _X("Circle"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Center"), "25,0");
		mceTextWriterAttributeF(_X(v1_ns), _X("Radius"), "20");
		mceTextWriterAttributeF(_X(v1_ns), _X("Color"), "Black");
		mceTextWriterEndElement(); // Circle
		mceTextWriterStartElement(_X(v1_ns), _X("Circle"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Center"), "50,0");
		mceTextWriterAttributeF(_X(v1_ns), _X("Radius"), "20");
		mceTextWriterAttributeF(_X(v1_ns), _X("Color"), "Red");
		mceTextWriterEndElement(); // Circle			
		mceTextWriterEndElement(); // v2:Watermark

		mceTextWriterStartElement(_X(v2_ns), _X("BLink"));
		mceTextWriterStartElement(_X(v1_ns), _X("Circle"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Center"), "13,0");
		mceTextWriterAttributeF(_X(v1_ns), _X("Radius"), "20");
		mceTextWriterAttributeF(_X(v1_ns), _X("Color"), "Yellow");
		mceTextWriterEndElement(); // Circle			
		mceTextWriterStartElement(_X(v1_ns), _X("Circle"));
		mceTextWriterAttributeF(_X(v1_ns), _X("Center"), "38,0");
		mceTextWriterAttributeF(_X(v1_ns), _X("Radius"), "20");
		mceTextWriterAttributeF(_X(v1_ns), _X("Color"), "Green");
		mceTextWriterEndElement(); // Circle			
		mceTextWriterEndElement(); // v2:Blink			
				
		mceTextWriterEndElement(); // Circles
		mceTextWriterEndDocument();
		mceTextWriterClose(writer);	
		opcFreeLibrary();
	}
	return 0;
}
