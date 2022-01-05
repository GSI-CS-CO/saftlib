AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g $(EB_CFLAGS) $(SIGC_CFLAGS) -I $(top_srcdir)/src

lib_LTLIBRARIES = libmini-saftlib.la
# libmini_saftlib_la_LDFLAGS = -version-info 0:0:0 
libmini_saftlib_la_SOURCES = \
	src/service.cpp     src/service.hpp   \
	src/loop.cpp        src/loop.hpp      \
	src/plugins.cpp     src/plugins.hpp   \
	src/saftbus.cpp     src/saftbus.hpp   \
	src/server.cpp      src/server.hpp    \
	src/client.cpp      src/client.hpp    \
	src/saftd.cpp       src/saftd.hpp             


bin_PROGRAMS = mini-saftd      \
               mini-saftclient 
               
mini_saftd_LDADD = $(EB_LIBS) $(SIGC_LIBS) libmini-saftlib.la
mini_saftd_SOURCES = src/mini-saftd.cpp

mini_saftclient_LDADD = $(EB_LIBS) $(SIGC_LIBS) libmini-saftlib.la
mini_saftclient_SOURCES = src/mini-saftclient.cpp

