AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src -I /home/michael/local/include/saftbus

lib_LTLIBRARIES = \
	libSAFTd_service.la  \
	libSAFTd_proxy.la    

libSAFTd_service_la_CPPADD  = $(EB_CFLAGS)
libSAFTd_service_la_LIBADD  = $(EB_LIBS)
libSAFTd_service_la_SOURCES = \
	src/SAFTd_service.cpp     

libSAFTd_proxy_la_LIBADD  = -L/home/michael/local/lib -lsaftbus
libSAFTd_proxy_la_SOURCES = \
	src/SAFTd_proxy.cpp     
SAFTd_proxy_includedir = $(includedir)/saftbus
SAFTd_proxy_include_HEADERS = \
	src/SAFTd_proxy.hpp       

bin_PROGRAMS = mini-saft-ctl
mini_saft_ctl_LDADD  = $(SIGC_LIBS) -L/home/michael/local/lib -lsaftbus libSAFTd_proxy.la
mini_saft_ctl_SOURCES = src/saft-ctl.cpp 
