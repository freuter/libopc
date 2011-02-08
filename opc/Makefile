
.PHONY: test install all

BUILDDIR=../build
OBJDIR=../build/opc
LIBDIR=../build/local
CONFIGDIR=../config

CFLAGS+=-I${CONFIGDIR} -I.. -I${LIBDIR}/include -I${LIBDIR}/include/libxml2
CFLAGS+=-g -Werror -Wall -std=c99 

LDFLAGS+=${LIBDIR}/lib/libxml2.a \
         ${LIBDIR}/lib/libz.a

OBJS = ${OBJDIR}/opc.o \
       ${OBJDIR}/container.o \
       ${OBJDIR}/part.o \
       ${OBJDIR}/relation.o \
       ${OBJDIR}/inputstream.o \
       ${OBJDIR}/outputstream.o \
       ${OBJDIR}/xmlreader.o \
       ${OBJDIR}/xmlwriter.o \
       ${OBJDIR}/zip.o 


${OBJDIR}/%.o : %.c %.h
	@mkdir -p ${OBJDIR}
	$(CC) ${CFLAGS} -c $< -o $@

${OBJDIR}/libopc.a: $(OBJS)
	$(AR) -rcs ${OBJDIR}/libopc.a $(OBJS)

${OBJDIR}/opc: ${OBJDIR}/main.o ${OBJDIR}/libopc.a
	$(CC) ${OBJDIR}/libopc.a ${OBJDIR}/main.o -o ${OBJDIR}/opc ${LDFLAGS}

all: ${OBJDIR}/libopc.a ${OBJDIR}/opc

clean: 
	rm -f $(OBJS) 
	rm -f ${OBJDIR}/main.o ${OBJDIR}/opc
