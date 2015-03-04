lib_LTLIBRARIES = libsaftlib.la
doc_DATA = TLU.pdf TLU_Channel.pdf ECA.pdf ECA_Condition.pdf ECA_Channel.pdf

EXTRA_DIST = h.xsl cpp.xsl common.xsl
CLEANFILES = *.cpp *.h *.pdf

AM_CPPFLAGS = $(GIOMM_CFLAGS)

libsaftlib_la_SOURCES = Directory.xml ECA.xml ECA_Condition.xml ECA_Channel.xml TLU.xml TLU_Channel.xml
libsaftlib_la_LIBADD = $(GIOMM_LIBS)

.xml.cpp:
	$(XSLTPROC) $(srcdir)/cpp.xsl $< > $*.cpp
	$(XSLTPROC) $(srcdir)/h.xsl $< > $*.h

.xml.pdf:
	$(GDBUS_CODEGEN) --generate-docbook=docfoo --interface-prefix=de.gsi.saftlib $<
	$(DOCBOOK2PDF) docfoo-*.xml
	mv docfoo-*.pdf $@
	rm -f docfoo-*
