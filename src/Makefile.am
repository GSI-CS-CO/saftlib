bin_PROGRAMS = eb-dbus

AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS)
AM_LDFLAGS  = $(EB_LIBS)   $(GIOMM_LIBS)

eb_dbus_SOURCES = ECA.cpp main.cpp

%.h %.cpp: %.xml h.xsl cpp.xsl
	xsltproc h.xsl $< > $*.h
	xsltproc cpp.xsl $< > $*.cpp
