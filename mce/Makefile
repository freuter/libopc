
.PHONY: test install all

BUILDDIR=../build
OBJDIR=../build/mce
LIBDIR=../build/local
CONFIGDIR=../config

CFLAGS+=-I${CONFIGDIR} -I.. -I${LIBDIR}/include -I${LIBDIR}/include/libxml2
CFLAGS+=-g -Werror -Wall -std=c99 

LDFLAGS+=${LIBDIR}/lib/libxml2.a \
         ${LIBDIR}/lib/libz.a

OBJS = ${OBJDIR}/textreader.o \
       ${OBJDIR}/textwriter.o 

${OBJDIR}/%.o : %.c %.h
	@mkdir -p ${OBJDIR}
	$(CC) ${CFLAGS} -c $< -o $@

${OBJDIR}/libmce.a: $(OBJS)
	$(AR) -rcs ${OBJDIR}/libmce.a $(OBJS)


all: ${OBJDIR}/libmce.a

clean: 
	rm -f $(OBJS) 
