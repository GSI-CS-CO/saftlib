lib_LTLIBRARIES = libsaftlib.la
if INTERFACE_DOCS
doc_DATA = TLU.pdf ECA.pdf ECA_Condition.pdf ECA_Channel.pdf Directory.pdf
endif

EXTRA_DIST = h.xsl cpp.xsl common.xsl
CLEANFILES = *.cpp *.h *.pdf

AM_CPPFLAGS = $(GIOMM_CFLAGS)

libsaftlib_la_SOURCES = 	\
	Directory.xml		\
	TimingReceiver.xml	\
	Inoutput.xml		\
	Input.xml		\
	MILbusActionSink.xml	\
	MILbusCondition.xml	\
	OutputCondition.xml	\
	Output.xml		\
	SCUbusActionSink.xml	\
	SCUbusCondition.xml	\
	SoftwareActionSink.xml	\
	SoftwareCondition.xml
libsaftlib_la_LIBADD = $(GIOMM_LIBS)

.xml.cpp:
	$(XSLTPROC) --xinclude $(srcdir)/cpp.xsl $< > $*.cpp
	$(XSLTPROC) --xinclude $(srcdir)/h.xsl $< > $*.h

.xml.pdf:
	$(GDBUS_CODEGEN) --generate-docbook=docfoo --interface-prefix=de.gsi.saftlib $<
	$(DOCBOOK2PDF) docfoo-*.$*.xml
	mv docfoo-*.$*.pdf $@
	rm -f docfoo-*.$*.*
