SUBDIRS = interfaces

bin_PROGRAMS = saftd
noinst_PROGRAMS = client

dbussystemdir = @sysconfdir@/dbus-1/system.d
dist_dbussystem_DATA = saftlib.conf

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)

saftd_SOURCES = \
	src/Device.cpp			src/Device.h			\
	src/OpenDevice.cpp		src/OpenDevice.h		\
	src/Driver.cpp			src/Driver.h			\
	src/Directory.cpp		src/Directory.h			\
	src/RegisteredObject.cpp	src/RegisteredObject.h		\
	src/eb-source.cpp		src/eb-source.h			\
	drivers/Owned.cpp		drivers/Owned.h			\
	drivers/Condition.cpp		drivers/Condition.h		\
	drivers/SoftwareCondition.cpp	drivers/SoftwareCondition.h	\
	drivers/ActionSink.cpp		drivers/ActionSink.h		\
	drivers/SoftwareActionSink.cpp	drivers/SoftwareActionSink.h	\
	drivers/TimingReceiver.cpp	drivers/TimingReceiver.h	\
	src/main.cpp
saftd_LDADD = $(EB_LIBS) $(GIOMM_LIBS) interfaces/libsaftlib.la

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la
