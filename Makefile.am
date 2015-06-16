SUBDIRS = interfaces

sbin_PROGRAMS = saftd
bin_PROGRAMS = dm-snoop
noinst_PROGRAMS = client

dbussystemdir = @sysconfdir@/dbus-1/system.d
dist_dbussystem_DATA = saftlib.conf

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)

saftd_SOURCES = \
	src/Device.cpp			src/Device.h			\
	src/OpenDevice.cpp		src/OpenDevice.h		\
	src/Driver.cpp			src/Driver.h			\
	src/SAFTd.cpp			src/SAFTd.h			\
	src/RegisteredObject.cpp	src/RegisteredObject.h		\
	src/eb-source.cpp		src/eb-source.h			\
	drivers/Owned.cpp		drivers/Owned.h			\
	drivers/Condition.cpp		drivers/Condition.h		\
	drivers/SoftwareCondition.cpp	drivers/SoftwareCondition.h	\
	drivers/ActionSink.cpp		drivers/ActionSink.h		\
	drivers/SoftwareActionSink.cpp	drivers/SoftwareActionSink.h	\
	drivers/TimingReceiver.cpp	drivers/TimingReceiver.h	\
	src/main.cpp			drivers/eca_regs.h
saftd_LDADD = $(EB_LIBS) $(GIOMM_LIBS) interfaces/libsaftlib.la

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

dm_snoop_SOURCES = src/dm-snoop.cpp
dm_snoop_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la
