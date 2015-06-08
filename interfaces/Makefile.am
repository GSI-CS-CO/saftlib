lib_LTLIBRARIES = libsaftlib.la
if INTERFACE_DOCS
doc_DATA = TLU.pdf ECA.pdf ECA_Condition.pdf ECA_Channel.pdf Directory.pdf
endif

NODES =				\
	Directory.xml		\
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
XSL = h.xsl cpp.xsl common.xsl

EXTRA_DIST = $(XSL) $(NODES) $(INTERFACES)
CLEANFILES = *.cpp *.h *.pdf

AM_CPPFLAGS = $(GIOMM_CFLAGS)

libsaftlib_la_LIBADD = $(GIOMM_LIBS)
libsaftlib_la_LDFLAGS = -version-info 1:1:0
nodist_libsaftlib_la_SOURCES =	\
	Directory.cpp		\
	iActionSink.cpp		\
	iCondition.cpp		\
	iDevice.cpp		\
	iDirectory.cpp		\
	iEventSource.cpp	\
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

$(nodist_libsaftlib_la_SOURCES):	$(NODES) $(XSL)
	$(XSLTPROC) --xinclude $(srcdir)/cpp.xsl $(NODES)
	$(XSLTPROC) --xinclude $(srcdir)/h.xsl $(NODES)

.xml.pdf:
	$(GDBUS_CODEGEN) --generate-docbook=docfoo --interface-prefix=de.gsi.saftlib $<
	$(DOCBOOK2PDF) docfoo-*.$*.xml
	mv docfoo-*.$*.pdf $@
	rm -f docfoo-*.$*.*
