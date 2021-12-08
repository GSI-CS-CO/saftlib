AUTOMAKE_OPTIONS=foreign subdir-objects
AM_CPPFLAGS = -Wall -g $(EB_CFLAGS) $(SIGC_CFLAGS) -I $(top_srcdir)/src
bin_PROGRAMS = mini-saftd
mini_saftd_LDADD = $(EB_LIBS) $(SIGC_LIBS)
mini_saftd_SOURCES = \
	src/main.cpp                            \
	src/container.cpp     src/container.hpp \
	src/object.cpp        src/object.hpp    \
	src/loop.cpp          src/loop.hpp   


#
