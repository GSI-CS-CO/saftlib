AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src -I /home/michael/local/include/saftbus

lib_LTLIBRARIES = \
	libtimingreceiver_service.la  \
	libtimingreceiver_proxy.la    

libtimingreceiver_service_la_CPPADD  = $(EB_CFLAGS)
libtimingreceiver_service_la_LIBADD  = $(EB_LIBS)
libtimingreceiver_service_la_SOURCES = \
	src/timingreceiver_service.cpp 

libtimingreceiver_proxy_la_LIBADD  = -L/home/michael/local/lib -lsaftbus
libtimingreceiver_proxy_la_SOURCES = \
	src/timingreceiver_proxy.cpp 
timingreceiver_proxy_includedir = $(includedir)/saftbus
timingreceiver_proxy_include_HEADERS = \
	src/timingreceiver_proxy.hpp 

bin_PROGRAMS = timingreceiver-ctl
timingreceiver_ctl_LDADD  = $(SIGC_LIBS) -L/home/michael/local/lib -lsaftbus libtimingreceiver_proxy.la
timingreceiver_ctl_SOURCES = src/timingreceiver-ctl.cpp 
