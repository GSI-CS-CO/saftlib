lib_LTLIBRARIES = libsaftlib.la
EXTRA_DIST = h.xsl cpp.xsl common.xsl
CLEANFILES = *.cpp *.h

AM_CPPFLAGS = $(GIOMM_CFLAGS)

libsaftlib_la_SOURCES = ECA.xml
libsaftlib_la_LIBADD = $(GIOMM_LIBS)

.xml.cpp:
	$(XSLTPROC) $(srcdir)/cpp.xsl $< > $*.cpp
	$(XSLTPROC) $(srcdir)/h.xsl $< > $*.h
