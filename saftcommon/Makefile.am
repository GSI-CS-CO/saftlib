
AM_CPPFLAGS = $(SIGCPP_CFLAGS) -I $(top_srcdir)/saftcommon

lib_LTLIBRARIES = libsaftcommon.la 
saftcommondir = $(includedir)/saftlib
saftcommon_HEADERS =         \
	Time.h             \
	CommonFunctions.h

leapseconddir = $(datadir)
leapsecond_DATA = leap-seconds.list


# saftcommondir = $(includedir)/saftlib
# saftcommon_DATA = leap-seconds.list

libsaftcommon_la_LIBADD = 
libsaftcommon_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

nodist_libsaftcommon_la_SOURCES = \
	Time.cpp                \
	CommonFunctions.cpp
