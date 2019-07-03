
AM_CPPFLAGS = $(SIGCPP_CFLAGS) -I $(top_srcdir)/saftcommon -DDATADIR='"$(datadir)"'

lib_LTLIBRARIES = libsaftcommon.la 
saftcommondir = $(includedir)/saftlib
saftcommon_HEADERS =         \
	Time.h             \
	CommonFunctions.h

leapseconddir = $(datadir)/saftlib
leapsecond_DATA = leap-seconds.list
EXTRA_DIST = leap-seconds.list

# saftcommondir = $(includedir)/saftlib
# saftcommon_DATA = leap-seconds.list

libsaftcommon_la_LIBADD = 
libsaftcommon_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

libsaftcommon_la_SOURCES = \
	Time.cpp                \
	CommonFunctions.cpp
