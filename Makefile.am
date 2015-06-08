SUBDIRS = interfaces

bin_PROGRAMS = saftd
noinst_PROGRAMS = client

dbussystemdir = @sysconfdir@/dbus-1/system.d
dist_dbussystem_DATA = saftlib.conf

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)

saftd_SOURCES = \
	src/Device.cpp			src/Device.h		\
	src/Directory.cpp		src/Directory.h		\
	src/eb-source.cpp		src/eb-source.h		\
	src/main.cpp
saftd_LDADD = $(EB_LIBS) $(GIOMM_LIBS) interfaces/libsaftlib.la
#	src/Driver.cpp			src/Driver.h		\
#	src/RegisteredObject.cpp	src/RegisteredObject.h	\
#	drivers/ECA.cpp			drivers/eca_regs.h	\
#	drivers/TLU.cpp			drivers/tlu_regs.h

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la
