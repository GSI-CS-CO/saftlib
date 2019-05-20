NODES =				\
	SAFTd.xml		\
	FunctionGeneratorFirmware.xml \
	FunctionGenerator.xml	\
	MasterFunctionGenerator.xml	\
	Input.xml		\
	MILbusActionSink.xml	\
	MILbusCondition.xml	\
	OutputCondition.xml	\
	Output.xml		\
	SCUbusActionSink.xml	\
	SCUbusCondition.xml	\
	EmbeddedCPUActionSink.xml	\
	EmbeddedCPUCondition.xml	\
	SoftwareActionSink.xml	\
	SoftwareCondition.xml	\
	TimingReceiver.xml      \
	WrMilGateway.xml
INTERFACES =			\
	ActionSink.xml		\
	Condition.xml		\
	Device.xml		\
	EventSource.xml		\
	InputEventSource.xml	\
	OutputActionSink.xml	\
	Owned.xml
XSL = h.xsl cpp.xsl doc.xsl common.xsl fix-docbook.xsl

EXTRA_DIST = $(XSL) $(NODES) $(INTERFACES)
CLEANFILES = *.cpp *.h *.pdf *.doc-xml

AM_CPPFLAGS = $(SIGCPP_CFLAGS) -I $(top_srcdir)/saftbus -I $(top_srcdir)/saftcommon
#AM_CPPFLAGS = -I $(top_srcdir)/saftbus

lib_LTLIBRARIES = libsaftlib.la
saftlibdir = $(includedir)/saftlib
nodist_saftlib_HEADERS = 	\
	SAFTd.h			\
	FunctionGeneratorFirmware.h	\
	FunctionGenerator.h	\
	MasterFunctionGenerator.h	\
	iActionSink.h		\
	iCondition.h		\
	iDevice.h		\
	iSAFTd.h		\
	iEventSource.h		\
	iInputEventSource.h	\
	iMILbusActionSink.h	\
	iMILbusCondition.h	\
	Input.h			\
	iFunctionGeneratorFirmware.h	\
	iFunctionGenerator.h	\
	iMasterFunctionGenerator.h	\
	iOutputActionSink.h	\
	iOutputCondition.h	\
	iOwned.h		\
	iSCUbusActionSink.h	\
	iSCUbusCondition.h	\
	iEmbeddedCPUActionSink.h	\
	iEmbeddedCPUCondition.h	\
	iSoftwareActionSink.h	\
	iSoftwareCondition.h	\
	iTimingReceiver.h	\
	iWrMilGateway.h     \
	MILbusActionSink.h	\
	MILbusCondition.h	\
	OutputCondition.h	\
	Output.h		\
	SCUbusActionSink.h	\
	SCUbusCondition.h	\
	EmbeddedCPUActionSink.h	\
	EmbeddedCPUCondition.h	\
	SoftwareActionSink.h	\
	SoftwareCondition.h	    \
	TimingReceiver.h        \
	WrMilGateway.h


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
	EmbeddedCPUActionSink.pdf	\
	EmbeddedCPUCondition.pdf	\
	SoftwareActionSink.pdf	\
	SoftwareCondition.pdf	\
	TimingReceiver.pdf     \
	WrMilGateway.pdf
endif

libsaftlib_la_LIBADD = $(SIGCPP_LIBS)
libsaftlib_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@



nodist_libsaftlib_la_SOURCES =	\
	SAFTd.cpp		\
	FunctionGeneratorFirmware.cpp	\
	FunctionGenerator.cpp	\
	MasterFunctionGenerator.cpp	\
	iActionSink.cpp		\
	iCondition.cpp		\
	iDevice.cpp		\
	iSAFTd.cpp		\
	iEventSource.cpp	\
	iFunctionGeneratorFirmware.cpp	\
	iFunctionGenerator.cpp	\
	iMasterFunctionGenerator.cpp	\
	iInputEventSource.cpp	\
	iMILbusActionSink.cpp	\
	iMILbusCondition.cpp	\
	Input.cpp		\
	iOutputActionSink.cpp	\
	iOutputCondition.cpp	\
	iOwned.cpp		\
	iSCUbusActionSink.cpp	\
	iSCUbusCondition.cpp	\
	iEmbeddedCPUActionSink.cpp	\
	iEmbeddedCPUCondition.cpp	\
	iSoftwareActionSink.cpp	\
	iSoftwareCondition.cpp	\
	iTimingReceiver.cpp	\
	iWrMilGateway.cpp   \
	MILbusActionSink.cpp	\
	MILbusCondition.cpp	\
	OutputCondition.cpp	\
	Output.cpp		\
	SCUbusActionSink.cpp	\
	SCUbusCondition.cpp	\
	EmbeddedCPUActionSink.cpp	\
	EmbeddedCPUCondition.cpp	\
	SoftwareActionSink.cpp	\
	SoftwareCondition.cpp	\
	TimingReceiver.cpp     \
	WrMilGateway.cpp


$(doc_DATA:.pdf=.doc-xml) $(nodist_libsaftlib_la_SOURCES) $(nodist_saftlib_HEADERS):	$(NODES) $(INTERFACES) $(XSL)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/h.xsl $(NODES)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/cpp.xsl $(NODES)
	$(XSLTPROC) --path $(srcdir) --xinclude $(srcdir)/doc.xsl $(NODES)

.doc-xml.pdf:
	rm -f docfoo-*.$*.*
	$(GDBUS_CODEGEN) --generate-docbook=docfoo --interface-prefix=de.gsi.saftlib $<
	$(XSLTPROC) --path $(srcdir) fix-docbook.xsl docfoo-*.$*.xml > docfoo-fix.$*.xml
	$(DOCBOOK2PDF) docfoo-fix.$*.xml
	mv docfoo-*.$*.pdf $@
	rm -f docfoo-*.$*.*
