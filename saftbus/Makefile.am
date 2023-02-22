AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src 

pkgdir = @libdir@/pkgconfig
pkg_DATA = saftbus.pc
EXTRA_DIST = saftbus.pc.in

saftbus_includedir    = $(includedir)/saftbus

lib_LTLIBRARIES = libsaftbus.la libsaftbus-server.la

libsaftbus_la_LDFLAGS        = -version-info @SAFTBUS_SO_VERSION@ 
libsaftbus_la_SOURCES = \
	src/loop.cpp         \
	src/saftbus.cpp       \
	src/error.cpp          \
	src/client.cpp    

libsaftbus_server_la_LDFLAGS = -version-info @SAFTBUS_SO_VERSION@ 
libsaftbus_server_la_SOURCES = \
	src/service.cpp             \
	src/plugins.cpp              \
	src/server.cpp        

saftbus_include_HEADERS = \
	src/loop.hpp           \
	src/error.hpp           \
	src/saftbus.hpp          \
	src/client.hpp            \
	src/service.hpp            \
	src/plugins.hpp             \
	src/global_allocator.hpp     \
	src/chunck_allocator_rt.hpp   \
	src/server.hpp

bin_PROGRAMS = \
	saftbusd    \
	saftbus-ctl

saftbusd_LDFLAGS = -Wl,--export-dynamic 
saftbusd_LDADD   = $(SIGC_LIBS) libsaftbus.la libsaftbus-server.la -ldl #-lltdl
saftbusd_SOURCES = \
	src/saftbusd.cpp 

saftbus_ctl_LDADD   = $(SIGC_LIBS) libsaftbus.la 
saftbus_ctl_SOURCES = src/saftbus-ctl.cpp
