.PHONY: all libxml lib tidy

BUILDDIR=../build
LOCALDIR=$(abspath ${BUILDDIR}/local)
ZLIB_VERISON=zlib-1.2.5
LIBXML_VERSION=libxml2-2.7.7

all: zlib libxml

zlib: ${ZLIB_VERISON}/configure
	pushd ${ZLIB_VERISON} && ./configure --prefix=${LOCALDIR} && make install && popd

#                    --with-debug
#--with-mem-debug 
#--with-run-debug

libxml: ${LIBXML_VERSION}/configure
	pushd ${LIBXML_VERSION} && \
        ./configure --prefix=${LOCALDIR} \
                    --without-c14n \
                    --without-catalog \
                    --without-docbook  \
                    --without-fexceptions \
                    --without-ftp \
                    --without-history  \
                    --without-html \
                    --without-http \
                    --without-iconv \
                    --without-iso8859x \
                    --without-legacy \
                    --without-output \
                    --without-pattern \
                    --without-push \
                    --without-python \
                    --with-reader \
                    --without-regexps \
                    --without-sax1 \
                    --without-schemas \
                    --without-schematron \
                    --without-threads \
                    --without-tree \
                    --without-valid \
                    --with-writer \
                    --without-xinclude \
                    --without-xpath \
                    --without-xptr \
                    --without-modules \
                    --with-zlib=${LOCALDIR} && \
        make install && popd


tidy:
	rm -rf ${LOCALDIR}