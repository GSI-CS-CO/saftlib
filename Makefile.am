AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src

lib_LTLIBRARIES = libmini-saftlib.la      \
                  libtiming-receiver.la

# libmini_saftlib_la_LDFLAGS = -version-info 0:0:0 
libmini_saftlib_la_SOURCES = \
	src/loop.cpp        src/loop.hpp      \
	src/saftbus.cpp     src/saftbus.hpp   \
	src/client.cpp      src/client.hpp    


libtiming_receiver_la_CPPADD  = $(EB_CFLAGS)
libtiming_receiver_la_LIBADD  = $(EB_LIBS)
libtiming_receiver_la_SOURCES = \
	tr/driver.cpp       tr/driver.hpp 

bin_PROGRAMS = mini-saftd      \
               mini-saftclient 
               
mini_saftd_LDADD   = $(SIGC_LIBS) libmini-saftlib.la -lltdl
mini_saftd_SOURCES = \
    src/mini-saftd.cpp   \
	src/service.cpp     src/service.hpp   \
	src/plugins.cpp     src/plugins.hpp   \
	src/server.cpp      src/server.hpp    

mini_saftclient_LDADD   = $(SIGC_LIBS) libmini-saftlib.la
mini_saftclient_SOURCES = src/mini-saftclient.cpp

