NODES =				\
	SAFTd.xml		\
	FunctionGenerator.xml	\
	Inoutput.xml		\
	Input.xml		\
	MILbusActionSink.xml	\
	MILbusCondition.xml	\
	OutputCondition.xml	\
	Output.xml		\
	SCUbusActionSink.xml	\
	SCUbusCondition.xml	\
	SoftwareActionSink.xml	\
	SoftwareCondition.xml	\
	TimingReceiver.xml
INTERFACES =			\
	ActionSink.xml		\
	Condition.xml		\
	Device.xml		\
	EventSource.xml		\
	InputEventSource.xml	\
	OutputActionSink.xml	\
	Owned.xml
XSL = h.xsl cpp.xsl doc.xsl common.xsl

EXTRA_DIST = $(XSL) $(NODES) $(INTERFACES)
CLEANFILES = *.cpp *.h *.pdf *.doc-xml

AM_CPPFLAGS = $(GIOMM_CFLAGS)

lib_LTLIBRARIES = libsaftlib.la
saftlibdir = $(includedir)/saftlib
nodist_saftlib_HEADERS = 	\
	SAFTd.h			\
	FunctionGenerator.h	\
	iActionSink.h		\
	iCondition.h		\
	iDevice.h		\
	iSAFTd.h		\
	iEventSource.h		\
	iInputEventSource.h	\
	iMILbusActionSink.h	\
	iMILbusCondition.h	\
	Inoutput.h		\
	Input.h			\
	iFunctionGenerator.h	\
	iOutputActionSink.h	\
	iOutputCondition.h	\
	iOwned.h		\
	iSCUbusActionSink.h	\
	iSCUbusCondition.h	\
	iSoftwareActionSink.h	\
	iSoftwareCondition.h	\
	iTimingReceiver.h	\
	MILbusActionSink.h	\
	MILbusCondition.h	\
	OutputCondition.h	\
	Output.h		\
	SCUbusActionSink.h	\
	SCUbusCondition.h	\
	SoftwareActionSink.h	\
	SoftwareCondition.h	\
	TimingReceiver.h

if INTERFACE_DOCS
doc_DATA = 			\
	ActionSink.pdf		\
	Condition.pdf		\
	Device.pdf		\
	SAFTd.pdf		\
	EventSource.pdf		\
	FunctionGenerator.pdf	\
	InputEventSource.pdf	\
	MILbusActionSink.pdf	\
	MILbusCondition.pdf	\
	OutputActionSink.pdf	\
	OutputCondition.pdf	\
	Owned.pdf		\
	SCUbusActionSink.pdf	\
	SCUbusCondition.pdf	\
	SoftwareActionSink.pdf	\
	SoftwareCondition.pdf	\
	TimingReceiver.pdf
endif

libsaftlib_la_LIBADD = $(GIOMM_LIBS)
libsaftlib_la_LDFLAGS = -version-info 2:1:0
nodist_libsaftlib_la_SOURCES =	\
	SAFTd.cpp		\
	FunctionGenerator.cpp	\
	iActionSink.cpp		\
	iCondition.cpp		\
	iDevice.cpp		\
	iSAFTd.cpp		\
	iEventSource.cpp	\
	iFunctionGenerator.cpp	\
	iInputEventSource.cpp	\
	iMILbusActionSink.cpp	\
	iMILbusCondition.cpp	\
	Inoutput.cpp		\
	Input.cpp		\
	iOutputActionSink.cpp	\
	iOutputCondition.cpp	\
	iOwned.cpp		\
	iSCUbusActionSink.cpp	\
	iSCUbusCondition.cpp	\
	iSoftwareActionSink.cpp	\
	iSoftwareCondition.cpp	\
	iTimingReceiver.cpp	\
	MILbusActionSink.cpp	\
	MILbusCondition.cpp	\
	OutputCondition.cpp	\
	Output.cpp		\
	SCUbusActionSink.cpp	\
	SCUbusCondition.cpp	\
	SoftwareActionSink.cpp	\
	SoftwareCondition.cpp	\
	TimingReceiver.cpp

$(doc_DATA:.pdf=.doc-xml) $(nodist_libsaftlib_la_SOURCES) $(nodist_saftlib_HEADERS):	$(NODES) $(XSL)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/h.xsl $(NODES)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/cpp.xsl $(NODES)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/doc.xsl $(NODES)

.doc-xml.pdf:
	$(GDBUS_CODEGEN) --generate-docbook=docfoo --interface-prefix=de.gsi.saftlib $<
	$(DOCBOOK2PDF) docfoo-*.$*.xml
	mv docfoo-*.$*.pdf $@
	rm -f docfoo-*.$*.*
