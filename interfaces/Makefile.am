lib_LTLIBRARIES = libsaftlib.la

AM_CPPFLAGS = $(GIOMM_CFLAGS)

libsaftlib_la_SOURCES = ECA.cpp
libsaftlib_la_LIBADD = $(GIOMM_LIBS)

%.h %.cpp: %.xml h.xsl cpp.xsl common.xsl
	$(XSLTPROC) h.xsl $< > $*.h
	$(XSLTPROC) cpp.xsl $< > $*.cpp
