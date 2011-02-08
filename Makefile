.PHONY: test install all doc opc sample mce tidy third_party

BUILDDIR=build
LOCALDIR=$(abspath ${BUILDDIR}/local)

all: opc mce sample doc

${LOCALDIR}/include/libxml2/libxml/xmlreader.h:
	$(MAKE) -C third_party

third_party: ${LOCALDIR}/include/libxml2/libxml/xmlreader.h

opc: third_party
	$(MAKE) -C opc

mce: opc 
	$(MAKE) -C mce

sample: opc mce
	$(MAKE) -C sample

doc: opc mce sample
	/Users/flr/local/bin/doxygen Doxyfile 

tidy:
	rm -rf ${BUILDDIR}