bin_PROGRAMS = eb-dbus client

AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS)
AM_LDFLAGS  = $(EB_LIBS)   $(GIOMM_LIBS)

eb_dbus_SOURCES = ECA.cpp eb-source.cpp main.cpp
client_SOURCES  = ECA.cpp eb-source.cpp client.cpp

%.h %.cpp: %.xml h.xsl cpp.xsl common.xsl
	xsltproc h.xsl $< > $*.h
	xsltproc cpp.xsl $< > $*.cpp
