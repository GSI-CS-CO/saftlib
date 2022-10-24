AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src 

pkgdir = @libdir@/pkgconfig
pkg_DATA = saftbus.pc
EXTRA_DIST = saftbus.pc.in

lib_LTLIBRARIES = libsaftbus.la libsaftbus-standalone.la libsaftbus-server.la
# libmini_saftlib_la_LDFLAGS = -version-info 0:0:0 

saftbus_includedir    = $(includedir)/saftbus
libsaftbus_la_SOURCES = \
	src/loop.cpp        \
	src/saftbus.cpp      \
	src/error.cpp         \
	src/client.cpp    

libsaftbus_standalone_la_SOURCES = \
	src/service-standalone.cpp

libsaftbus_server_la_SOURCES = \
	src/service.cpp   \
	src/plugins.cpp    \
	src/server.cpp        

saftbus_include_HEADERS = \
	src/make_unique.hpp    \
	src/loop.hpp            \
	src/error.hpp            \
	src/saftbus.hpp           \
	src/client.hpp             \
	src/service.hpp             \
	src/plugins.hpp              \
	src/chunck_allocator_rt.hpp   \
	src/server.hpp    



bin_PROGRAMS = \
	saftbusd    \
	saftbus-ctl  \
	saftbus-gen

saftbusd_LDFLAGS = -Wl,--export-dynamic 
saftbusd_LDADD   = $(SIGC_LIBS) libsaftbus.la libsaftbus-server.la -lltdl
saftbusd_SOURCES = \
	src/saftbusd.cpp 

saftbus_ctl_LDADD   = $(SIGC_LIBS) libsaftbus.la 
saftbus_ctl_SOURCES = src/saftbus-ctl.cpp

saftbus_gen_SOURCES = src/saftbus-gen.cpp

