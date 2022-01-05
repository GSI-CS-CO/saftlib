AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src -I $(top_srcdir)/timingreceiver

lib_LTLIBRARIES = libmini-saftlib.la            \
                  libtimingreceiver_service.la  \
                  libtimingreceiver_proxy.la    

# libmini_saftlib_la_LDFLAGS = -version-info 0:0:0 

mini_saftlib_includedir    = $(includedir)/mini-saftlib
libmini_saftlib_la_SOURCES = \
	src/loop.cpp      \
	src/saftbus.cpp   \
	src/client.cpp    
mini_saftlib_include_HEADERS = \
	src/loop.hpp      \
	src/saftbus.hpp   \
	src/client.hpp    \
	src/service.hpp   \
	src/plugins.hpp   \
	src/server.hpp    


libtimingreceiver_service_la_CPPADD  = $(EB_CFLAGS)
libtimingreceiver_service_la_LIBADD  = $(EB_LIBS)
libtimingreceiver_service_la_SOURCES = \
	timingreceiver/timingreceiver_service.cpp 

libtimingreceiver_proxy_la_LIBADD  = libmini-saftlib.la
libtimingreceiver_proxy_la_SOURCES = \
	timingreceiver/timingreceiver_proxy.cpp 
timingreceiver_proxy_includedir = $(includedir)/mini-saftlib
timingreceiver_proxy_include_HEADERS = \
	timingreceiver/timingreceiver_proxy.hpp 

bin_PROGRAMS = mini-saftd      \
               mini-saftclient 

mini_saftd_LDFLAGS = -Wl,--export-dynamic # -Wl,--whole-archive,libmini-saftlib.la,--no-whole-archive        
mini_saftd_LDADD   = $(SIGC_LIBS) libmini-saftlib.la -lltdl
mini_saftd_SOURCES = \
    src/mini-saftd.cpp   \
	src/service.cpp      \
	src/plugins.cpp      \
	src/server.cpp        

mini_saftclient_LDADD   = $(SIGC_LIBS) libmini-saftlib.la libtimingreceiver_proxy.la 
mini_saftclient_SOURCES = src/mini-saftclient.cpp

