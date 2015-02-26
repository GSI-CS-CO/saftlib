SUBDIRS = interfaces

bin_PROGRAMS = eb-dbus client
dbus_servicedir = @DBUS_SERVICEDIR@
dist_dbus_service_DATA = saftlib.conf

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)/interfaces

eb_dbus_SOURCES = \
	src/ObjectRegistry.cpp	src/ObjectRegistry.h	\
	src/Driver.cpp		src/Driver.h		\
	src/Device.cpp		src/Device.h		\
	src/eb-source.cpp	src/eb-source.h		\
	drivers/ECA.cpp					\
	drivers/TLU.cpp					\
	devices/pexaria5.cpp				\
	src/main.cpp
eb_dbus_LDADD = $(EB_LIBS) $(GIOMM_LIBS) -leca interfaces/libsaftlib.la

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la
