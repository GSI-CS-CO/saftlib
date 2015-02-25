bin_PROGRAMS    = eb-dbus client
lib_LTLIBRARIES = libsaftlib.la

AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS)

libsaftlib_la_SOURCES = ECA.cpp
libsaftlib_la_LIBADD = $(GIOMM_LIBS)

eb_dbus_SOURCES = GlobalObject.cpp eb-source.cpp main.cpp
eb_dbus_LDADD = $(EB_LIBS) $(GIOMM_LIBS) libsaftlib.la -leca

client_SOURCES  = client.cpp
client_LDADD = $(GIOMM_LIBS) libsaftlib.la

%.h %.cpp: %.xml h.xsl cpp.xsl common.xsl
	$(XSLTPROC) h.xsl $< > $*.h
	$(XSLTPROC) cpp.xsl $< > $*.cpp
