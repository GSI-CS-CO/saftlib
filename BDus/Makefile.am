
AM_CPPFLAGS = $(GIOMM_CFLAGS) -I $(top_srcdir)/interfaces/BDus

lib_LTLIBRARIES = libbdus.la
bdusdir = $(includedir)/saftlib
bdus_HEADERS = 	\
	Proxy.h            \
	Error.h            \
	Connection.h       \
	Interface.h        \
	bdus.h  

libbdus_la_LIBADD = 
libbdus_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

nodist_libbdus_la_SOURCES =	\
	Proxy.cpp          \
	Error.cpp          \
	Connection.cpp     \
	Interface.cpp      \
	bdus.cpp

