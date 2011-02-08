
.PHONY: test install all

BUILDDIR=../build
OBJDIR=../build/sample
LIBDIR=../build/local
CONFIGDIR=../config

CFLAGS+=-I${CONFIGDIR} -I.. -I${LIBDIR}/include -I${LIBDIR}/include/libxml2
CFLAGS+=-g -Werror -Wall -std=c99 

LDFLAGS+=${LIBDIR}/lib/libxml2.a \
         ${LIBDIR}/lib/libz.a \
	 ${BUILDDIR}/opc/libopc.a \
	 ${BUILDDIR}/mce/textreader.o \
	 ${BUILDDIR}/mce/textwriter.o

SAMPLES = ${OBJDIR}/opc_helloworld \
	    ${OBJDIR}/opc_zipwrite \
	    ${OBJDIR}/opc_zipwrite2 \
	    ${OBJDIR}/opc_zipwrite3 \
	    ${OBJDIR}/opc_zipwrite4 \
	    ${OBJDIR}/opc_zipwrite5 \
	    ${OBJDIR}/opc_zipwrite6 \
	    ${OBJDIR}/opc_zipread \
	    ${OBJDIR}/opc_zipread2 \
	    ${OBJDIR}/opc_zipextract \
	    ${OBJDIR}/opc_zipextract2 \
	    ${OBJDIR}/opc_zipdefrag \
	    ${OBJDIR}/opc_zipcopy \
	    ${OBJDIR}/opc_dump \
	    ${OBJDIR}/opc_extract \
	    ${OBJDIR}/opc_xml \
	    ${OBJDIR}/opc_xml2 \
	    ${OBJDIR}/mce_read \
	    ${OBJDIR}/mce_write



${OBJDIR}/%.o : %.c 
	@mkdir -p ${OBJDIR}
	$(CC) ${CFLAGS} -c $< -o $@

${OBJDIR}/% : ${OBJDIR}/%.o 
	$(CC) $< -o $@ ${LDFLAGS}

all: $(SAMPLES)

clean: 
	rm -f $(SAMPLES) 
	rm -f ${OBJDIR}/main.o ${OBJDIR}/opc
