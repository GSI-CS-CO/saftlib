AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src 

lib_LTLIBRARIES = libsaftbus.la            
# libmini_saftlib_la_LDFLAGS = -version-info 0:0:0 

saftbus_includedir    = $(includedir)/saftbus
libsaftbus_la_SOURCES = \
	src/saftbus.cpp   \
	src/client.cpp    
saftbus_include_HEADERS = \
	src/make_unique.hpp \
	src/loop.hpp        \
	src/saftbus.hpp     \
	src/client.hpp      \
	src/service.hpp     \
	src/plugins.hpp     \
	src/server.hpp    

bin_PROGRAMS = saftbusd      \
               saftbus-ctl   \
               saftbus-gen

saftbusd_LDFLAGS = -Wl,--export-dynamic 
saftbusd_LDADD   = $(SIGC_LIBS) libsaftbus.la -lltdl
saftbusd_SOURCES = \
	src/loop.cpp       \
	src/saftbusd.cpp \
	src/service.cpp    \
	src/plugins.cpp    \
	src/server.cpp        

saftbus_ctl_LDADD   = $(SIGC_LIBS) libsaftbus.la 
saftbus_ctl_SOURCES = src/saftbus-ctl.cpp

saftbus_gen_SOURCES = src/saftbus-gen.cpp

