SUBDIRS = interfaces

sbin_PROGRAMS = saftd
bin_PROGRAMS = dm-snoop fg-ctl
noinst_PROGRAMS = client

dbussystemdir = @sysconfdir@/dbus-1/system.d
dist_dbussystem_DATA = saftlib.conf

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)

BUILT_SOURCES = version.h
.PHONY: version.h # causes build.cpp to be rebuilt every run

saftd_SOURCES = version.h \
	src/Device.cpp			src/Device.h			\
	src/OpenDevice.cpp		src/OpenDevice.h		\
	src/Driver.cpp			src/Driver.h			\
	src/SAFTd.cpp			src/SAFTd.h			\
	src/RegisteredObject.cpp	src/RegisteredObject.h		\
	src/eb-source.cpp		src/eb-source.h			\
	src/build.cpp			src/build.h			\
	drivers/eca_regs.h		drivers/fg_regs.h		\
	drivers/Owned.cpp		drivers/Owned.h			\
	drivers/Condition.cpp		drivers/Condition.h		\
	drivers/SoftwareCondition.cpp	drivers/SoftwareCondition.h	\
	drivers/SCUbusCondition.cpp	drivers/SCUbusCondition.h	\
	drivers/MILbusCondition.cpp	drivers/MILbusCondition.h	\
	drivers/ActionSink.cpp		drivers/ActionSink.h		\
	drivers/SoftwareActionSink.cpp	drivers/SoftwareActionSink.h	\
	drivers/SCUbusActionSink.cpp	drivers/SCUbusActionSink.h	\
	drivers/MILbusActionSink.cpp	drivers/MILbusActionSink.h	\
	drivers/FunctionGenerator.cpp	drivers/FunctionGenerator.h	\
	drivers/TimingReceiver.cpp	drivers/TimingReceiver.h	\
	src/globals.cpp			src/main.cpp
saftd_LDADD = $(EB_LIBS) $(GIOMM_LIBS) interfaces/libsaftlib.la
saftd_LDFLAGS = -rdynamic # needed for stack trace

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

dm_snoop_SOURCES = src/dm-snoop.cpp
dm_snoop_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

fg_ctl_SOURCES = src/fg-ctl.cpp
fg_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

if GIT_TREE
version.h:
	echo '#define GIT_ID "'$$(git describe --dirty --always --tags)'"' > version.h.tmp
	echo '#define SOURCE_DATE "'$$(date +'%b %d %Y %H:%M:%S' --date=@$$(git log -n1 --pretty='format:%ct'))'"' >> version.h.tmp
	mv version.h.tmp version.h
endif
