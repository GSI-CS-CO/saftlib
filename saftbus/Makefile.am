
AM_CPPFLAGS = $(GIOMM_CFLAGS) -I $(top_srcdir)/saftbus

lib_LTLIBRARIES = libsaftbus.la
saftbusdir = $(includedir)/saftlib
saftbus_HEADERS = 	\
	core.h             \
	Proxy.h            \
	Error.h            \
	Connection.h       \
	Interface.h        \
	saftbus.h  

libsaftbus_la_LIBADD = 
libsaftbus_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

nodist_libsaftbus_la_SOURCES =	\
	core.cpp           \
	Proxy.cpp          \
	Error.cpp          \
	Connection.cpp     \
	Interface.cpp      \
	saftbus.cpp

