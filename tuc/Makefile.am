
AM_CPPFLAGS = $(SIGCPP_CFLAGS) -I $(top_srcdir)/tuc

lib_LTLIBRARIES = libtuc.la 
tucdir = $(includedir)/saftlib
tuc_HEADERS = \
	tuc.h     

libtuc_la_LIBADD = 
libtuc_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

nodist_libtuc_la_SOURCES = \
	tuc.cpp 
