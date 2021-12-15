AUTOMAKE_OPTIONS=foreign subdir-objects
AM_CPPFLAGS = -Wall -g $(EB_CFLAGS) $(SIGC_CFLAGS) -I $(top_srcdir)/src
bin_PROGRAMS = mini-saftd      \
               mini-saftclient 
               
mini_saftd_LDADD = $(EB_LIBS) $(SIGC_LIBS)
mini_saftd_SOURCES = \
	src/mini-saftd.cpp                                   \
	src/container.cpp          src/container.hpp         \
	src/service.cpp            src/service.hpp           \
	src/loop.cpp               src/loop.hpp              \
	src/saftbus.cpp            src/saftbus.hpp           \
	src/server_connection.cpp  src/server_connection.hpp \
	src/client_connection.cpp  src/client_connection.hpp \
	src/saftd.cpp              src/saftd.hpp             

mini_saftclient_LDADD = $(EB_LIBS) $(SIGC_LIBS)
mini_saftclient_SOURCES = \
	src/mini-saftclient.cpp                              \
	src/saftbus.cpp            src/saftbus.hpp           \
	src/client_connection.cpp  src/client_connection.hpp \
	src/saftd.cpp              src/saftd.hpp             


