SUBDIRS = interfaces

sbin_PROGRAMS = saftd
bin_PROGRAMS = saft-ctl saft-fg-ctl saft-mfg-ctl saft-io-ctl saft-pps-gen saft-ecpu-ctl saft-scu-ctl saft-clk-gen saft-dm saft-gmt-check saft-lcd
noinst_PROGRAMS = client

dbussystemdir = @sysconfdir@/dbus-1/system.d
dist_dbussystem_DATA = saftlib.conf

pkgdir = @libdir@/pkgconfig
pkg_DATA = saftlib.pc

EXTRA_DIST = saftlib.pc.in

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)

BUILT_SOURCES = version.h
.PHONY: version.h debian # causes build.cpp to be rebuilt every run

saftd_SOURCES = version.h \
	src/Device.cpp			src/Device.h			\
	src/OpenDevice.cpp		src/OpenDevice.h		\
	src/Driver.cpp			src/Driver.h			\
	src/BaseObject.cpp		src/BaseObject.h		\
	src/SAFTd.cpp			src/SAFTd.h			\
	src/RegisteredObject.cpp	src/RegisteredObject.h		\
	src/eb-source.cpp		src/eb-source.h			\
	src/clog.cpp			src/clog.h			\
	src/build.cpp			src/build.h			\
	drivers/SerdesClockGen.cpp	drivers/SerdesClockGen.h	\
	drivers/eca_regs.h		drivers/eca_flags.h		\
	drivers/eca_queue_regs.h	drivers/eca_tlu_regs.h		\
	drivers/io_control_regs.h	drivers/fg_regs.h		\
	drivers/Owned.cpp		drivers/Owned.h			\
	drivers/Condition.cpp		drivers/Condition.h		\
	drivers/SoftwareCondition.cpp	drivers/SoftwareCondition.h	\
	drivers/OutputCondition.cpp	drivers/OutputCondition.h	\
	drivers/SCUbusCondition.cpp	drivers/SCUbusCondition.h	\
	drivers/EmbeddedCPUCondition.cpp	drivers/EmbeddedCPUCondition.h	\
	drivers/MILbusCondition.cpp	drivers/MILbusCondition.h	\
	drivers/ActionSink.cpp		drivers/ActionSink.h		\
	drivers/EventSource.cpp		drivers/EventSource.h		\
	drivers/InoutImpl.cpp           drivers/InoutImpl.h		\
	drivers/SoftwareActionSink.cpp	drivers/SoftwareActionSink.h	\
	drivers/SCUbusActionSink.cpp	drivers/SCUbusActionSink.h	\
	drivers/EmbeddedCPUActionSink.cpp	drivers/EmbeddedCPUActionSink.h	\
	drivers/MILbusActionSink.cpp	drivers/MILbusActionSink.h	\
	drivers/FunctionGenerator.cpp	drivers/FunctionGenerator.h	\
	drivers/MasterFunctionGenerator.cpp	drivers/MasterFunctionGenerator.h	\
	drivers/FunctionGeneratorImpl.cpp	drivers/FunctionGeneratorImpl.h	\
	drivers/Output.cpp		drivers/Output.h		\
	drivers/Input.cpp		drivers/Input.h			\
	drivers/TimingReceiver.cpp	drivers/TimingReceiver.h	\
	src/globals.cpp			src/main.cpp

saftd_LDADD = $(EB_LIBS) $(GIOMM_LIBS) interfaces/libsaftlib.la
saftd_LDFLAGS = -rdynamic # needed for stack trace

client_SOURCES = src/client.cpp
client_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_ctl_SOURCES = src/saft-ctl.cpp src/CommonFunctions.cpp
saft_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_ecpu_ctl_SOURCES = src/saft-ecpu-ctl.cpp src/CommonFunctions.cpp
saft_ecpu_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_scu_ctl_SOURCES = src/saft-scu-ctl.cpp src/CommonFunctions.cpp
saft_scu_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_fg_ctl_SOURCES = src/saft-fg-ctl.cpp
saft_fg_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_mfg_ctl_SOURCES = src/saft-mfg-ctl.cpp
saft_mfg_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la -lpthread

saft_io_ctl_SOURCES = src/saft-io-ctl.cpp src/CommonFunctions.cpp
saft_io_ctl_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_pps_gen_SOURCES = src/saft-pps-gen.cpp
saft_pps_gen_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_clk_gen_SOURCES = src/saft-clk-gen.cpp src/CommonFunctions.cpp
saft_clk_gen_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_dm_SOURCES = src/saft-dm.cpp src/CommonFunctions.cpp
saft_dm_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_gmt_check_SOURCES = src/saft-gmt-check.cpp src/CommonFunctions.cpp
saft_gmt_check_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la

saft_lcd_SOURCES = src/saft-lcd.cpp src/CommonFunctions.cpp
saft_lcd_LDADD   = $(GIOMM_LIBS) interfaces/libsaftlib.la


if GIT_TREE
version.h:
	echo '#define GIT_ID "'$$(git describe --dirty --always --tags)'"' > version.h.tmp
	echo '#define SOURCE_DATE "'$$(date +'%b %d %Y %H:%M:%S' --date=@$$(git log -n1 --pretty='format:%ct'))'"' >> version.h.tmp
	mv -f version.h.tmp version.h
debian:
	cp debian/libsaftlib.install.in debian/libsaftlib@SAFTLIB_SONAME@.install
	./config.status --file=debian/control
endif

# Use manpages in distribution tarball if docbook2man not found
if REBUILD_MAN_PAGES
.sgml.1:
	$(DOCBOOK2MAN) -o $(@D) $<
endif
