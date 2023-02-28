AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src 

pkgdir = @libdir@/pkgconfig
pkg_DATA = saftbus.pc
EXTRA_DIST = saftbus.pc.in

saftbus_includedir = $(includedir)/saftbus

lib_LTLIBRARIES = libsaftbus.la

libsaftbus_la_LDFLAGS = -version-info @SAFTBUS_SO_VERSION@ 
libsaftbus_la_SOURCES = \
	src/loop.cpp         \
	src/saftbus.cpp       \
	src/error.cpp          \
	src/client.cpp          \
	src/service.cpp          \
	src/plugins.cpp           \
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

bin_PROGRAMS =   \
	saftbusd      \
	saftbusd-srta  \
	saftbusd-norta \
	saftbus-ctl

saftbusd_LDFLAGS = -Wl,--export-dynamic 
saftbusd_LDADD   = $(SIGC_LIBS) libsaftbus.la -ldl #-lltdl
saftbusd_SOURCES = \
	src/saftbusd.cpp src/configurable_chunck_allocator_rt.cpp

saftbusd_srta_LDFLAGS = -Wl,--export-dynamic 
saftbusd_srta_LDADD   = $(SIGC_LIBS) libsaftbus.la -ldl #-lltdl
saftbusd_srta_SOURCES = \
	src/saftbusd-srta.cpp 

saftbusd_norta_LDFLAGS = -Wl,--export-dynamic 
saftbusd_norta_LDADD   = $(SIGC_LIBS) libsaftbus.la -ldl #-lltdl
saftbusd_norta_SOURCES = \
	src/saftbusd-norta.cpp 

saftbus_ctl_LDADD   = $(SIGC_LIBS) libsaftbus.la 
saftbus_ctl_SOURCES = src/saftbus-ctl.cpp
