AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SAFTBUS_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)/src -I $(top_srcdir)/src/interfaces -I $(top_builddir)/src/interfaces -DDATADIR='"$(datadir)/@PACKAGE@"'

pkgdir = @libdir@/pkgconfig
pkg_DATA = saftlib.pc
EXTRA_DIST = saftlib.pc.in 
#the file leap-seconds.list ist needed by the Time class to convert between UTC and WR-time
leapseconddir = $(datadir)/@PACKAGE@
leapsecond_DATA = leap-seconds.list
EXTRA_DIST += leap-seconds.list 
EXTRA_DIST += src/software-tr.sdb

dist_bin_SCRIPTS = src/saftd


BUILT_SOURCES = version.h
.PHONY: version.h # causes build.cpp to be rebuilt every run


lib_LTLIBRARIES = \
	lib@PACKAGE@-service.la  \
	lib@PACKAGE@-proxy.la    
# @PACKAGE@ is substituded with the string given to AC_INIT([...])
lib@PACKAGE@_service_la_LDFLAGS = -Wl,--export-dynamic 
lib@PACKAGE@_service_la_CPPADD  = $(EB_CFLAGS) $(SIGCPP_CFLAGS)
lib@PACKAGE@_service_la_LIBADD  = $(EB_LIBS)   $(SIGCPP_LIBS)  -lltdl
lib@PACKAGE@_service_la_SOURCES =\
	src/build.cpp                 \
	src/Time.cpp                   \
	src/eb-source.cpp               \
	src/eb-forward.cpp               \
	src/Owned.cpp                     \
	src/Owned_Service.cpp              \
	src/SAFTd.cpp                       \
	src/SAFTd_Service.cpp                \
	src/Condition.cpp                     \
	src/Condition_Service.cpp              \
	src/ActionSink.cpp                      \
	src/ActionSink_Service.cpp               \
	src/SoftwareCondition.cpp                 \
	src/SoftwareCondition_Service.cpp          \
	src/SoftwareActionSink.cpp                  \
	src/SoftwareActionSink_Service.cpp           \
	src/SCUbusCondition.cpp                       \
	src/SCUbusCondition_Service.cpp                \
	src/SCUbusActionSink.cpp                        \
	src/SCUbusActionSink_Service.cpp                 \
	src/EmbeddedCPUCondition.cpp                      \
	src/EmbeddedCPUCondition_Service.cpp               \
	src/EmbeddedCPUActionSink.cpp                       \
	src/EmbeddedCPUActionSink_Service.cpp                \
	src/WbmCondition.cpp                                  \
	src/WbmCondition_Service.cpp                           \
	src/WbmActionSink.cpp                                   \
	src/WbmActionSink_Service.cpp                            \
	src/SdbDevice.cpp                                        \
	src/MsiDevice.cpp                                        \
	src/OpenDevice.cpp                                        \
	src/WhiteRabbit.cpp                                         \
	src/Mailbox.cpp                                              \
	src/Watchdog.cpp                                             \
	src/SerdesClockGen.cpp                                       \
	src/Io.cpp                                                   \
	src/IoControl.cpp                                            \
	src/Output.cpp                                               \
	src/Output_Service.cpp                                       \
	src/OutputCondition.cpp                                      \
	src/OutputCondition_Service.cpp                              \
	src/EventSource.cpp                                          \
	src/Input.cpp                                                \
	src/Input_Service.cpp                                        \
	src/ECA.cpp                                                  \
	src/ECA_TLU.cpp                                              \
	src/BuildIdRom.cpp                                           \
	src/TempSensor.cpp                                           \
	src/Reset.cpp                                                \
	src/LM32Cluster.cpp                                          \
	src/LM32Firmware.cpp                                         \
	src/LM32Firmware_Service.cpp                                 \
	src/TimingReceiver.cpp                                       \
	src/TimingReceiver_Service.cpp                               \
	src/CommonFunctions.cpp                                      \
	src/create_service.cpp

lib@PACKAGE@_service_includedir = $(includedir)/saftlib
lib@PACKAGE@_service_include_HEADERS =\
	version.h                      \
	src/ats_regs.h                 \
	src/bg_regs.h                  \
	src/eca_ac_wbm_regs.h          \
	src/eca_queue_regs.h           \
	src/eca_regs.h                 \
	src/eca_flags.h                \
	src/eca_tlu_regs.h             \
	src/fg_regs.h                  \
	src/io_control_regs.h          \
	src/wr_mil_gw_regs.h           \
	src/build.hpp                      \
	src/Time.hpp                        \
	src/eb-source.hpp                    \
	src/eb-forward.hpp                    \
	src/Owned.hpp                          \
	src/SAFTd.hpp                           \
	src/Condition.hpp                        \
	src/ActionSink.hpp                        \
	src/SoftwareCondition.hpp                  \
	src/SoftwareActionSink.hpp                  \
	src/SCUbusCondition.hpp                      \
	src/SCUbusActionSink.hpp                      \
	src/EmbeddedCPUCondition.hpp                   \
	src/EmbeddedCPUActionSink.hpp                   \
	src/WbmCondition.hpp                             \
	src/WbmActionSink.hpp                             \
	src/SdbDevice.hpp                                 \
	src/MsiDevice.hpp                                 \
	src/OpenDevice.hpp                                 \
	src/WhiteRabbit.hpp                                 \
	src/Mailbox.hpp                                      \
	src/Watchdog.hpp                                      \
	src/SerdesClockGen.hpp                                \
	src/Io.hpp                                            \
	src/IoControl.hpp                                     \
	src/Output.hpp                                        \
	src/OutputCondition.hpp                               \
	src/EventSource.hpp                                   \
	src/Input.hpp                                         \
	src/ECA.hpp                                           \
	src/ECA_TLU.hpp                                       \
	src/BuildIdRom.hpp                                    \
	src/TempSensor.hpp                                    \
	src/Reset.hpp                                         \
	src/LM32Cluster.hpp                                   \
	src/LM32Firmware.hpp                                  \
	src/TimingReceiver.hpp                                \
	src/CommonFunctions.hpp                               \
 	src/Owned_Service.hpp                                 \
 	src/SAFTd_Service.hpp                                 \
 	src/Condition_Service.hpp                             \
 	src/ActionSink_Service.hpp                            \
 	src/SoftwareCondition_Service.hpp                     \
 	src/SoftwareActionSink_Service.hpp                    \
 	src/SCUbusCondition_Service.hpp                       \
 	src/SCUbusActionSink_Service.hpp                      \
 	src/EmbeddedCPUCondition_Service.hpp                  \
 	src/EmbeddedCPUActionSink_Service.hpp                 \
 	src/WbmCondition_Service.hpp                          \
 	src/WbmActionSink_Service.hpp                         \
 	src/Output_Service.hpp                                \
 	src/OutputCondition_Service.hpp                       \
 	src/Input_Service.hpp                                 \
 	src/LM32Firmware_Service.hpp                          \
 	src/TimingReceiver_Service.hpp                           

lib@PACKAGE@_proxy_la_LIBADD  = $(SAFTBUS_LIBS) $(SIGCPP_LIBS)
lib@PACKAGE@_proxy_la_SOURCES =   \
	src/Time.cpp                   \
	src/Owned_Proxy.cpp             \
	src/SAFTd_Proxy.cpp              \
	src/Condition_Proxy.cpp           \
	src/ActionSink_Proxy.cpp           \
	src/SoftwareCondition_Proxy.cpp     \
	src/SoftwareActionSink_Proxy.cpp     \
	src/SCUbusCondition_Proxy.cpp         \
	src/SCUbusActionSink_Proxy.cpp         \
	src/EmbeddedCPUCondition_Proxy.cpp      \
	src/EmbeddedCPUActionSink_Proxy.cpp      \
	src/WbmCondition_Proxy.cpp                \
	src/WbmActionSink_Proxy.cpp                \
	src/OpenDevice_Proxy.cpp                    \
	src/WhiteRabbit_Proxy.cpp                    \
	src/Output_Proxy.cpp                          \
	src/OutputCondition_Proxy.cpp                  \
	src/Input_Proxy.cpp                             \
	src/EventSource_Proxy.cpp                        \
	src/ECA_Proxy.cpp                                 \
	src/ECA_TLU_Proxy.cpp                              \
	src/BuildIdRom_Proxy.cpp                            \
	src/TempSensor_Proxy.cpp                             \
	src/Reset_Proxy.cpp                                   \
	src/LM32Cluster_Proxy.cpp                              \
	src/LM32Firmware_Proxy.cpp                              \
	src/TimingReceiver_Proxy.cpp                             \
	src/CommonFunctions.cpp                               


lib@PACKAGE@_proxy_includedir = $(includedir)/saftlib
lib@PACKAGE@_proxy_include_HEADERS = \
	src/Time.hpp                      \
	src/Owned_Proxy.hpp                \
	src/SAFTd_Proxy.hpp                 \
	src/Condition_Proxy.hpp              \
	src/ActionSink_Proxy.hpp              \
	src/SoftwareCondition_Proxy.hpp        \
	src/SoftwareActionSink_Proxy.hpp        \
	src/SCUbusCondition_Proxy.hpp            \
	src/SCUbusActionSink_Proxy.hpp            \
	src/EmbeddedCPUCondition_Proxy.hpp         \
	src/EmbeddedCPUActionSink_Proxy.hpp         \
	src/WbmCondition_Proxy.hpp                   \
	src/WbmActionSink_Proxy.hpp                   \
	src/OpenDevice_Proxy.hpp                       \
	src/WhiteRabbit_Proxy.hpp                       \
	src/Output_Proxy.hpp                             \
	src/OutputCondition_Proxy.hpp                     \
	src/Input_Proxy.hpp                                \
	src/EventSource_Proxy.hpp                           \
	src/ECA_Proxy.hpp                                    \
	src/ECA_TLU_Proxy.hpp                                 \
	src/BuildIdRom_Proxy.hpp                               \
	src/TempSensor_Proxy.hpp                                \
	src/Reset_Proxy.hpp                                      \
	src/LM32Cluster_Proxy.hpp                                 \
	src/LM32Firmware_Proxy.hpp                                 \
	src/TimingReceiver_Proxy.hpp                                \
	src/CommonFunctions.hpp                                      \
	src/interfaces/BurstGenerator.h                              \
	src/interfaces/SAFTd.h                                       \
	src/interfaces/iDevice.h                                     \
	src/interfaces/iOwned.h                                      \
	src/interfaces/CommonFunctions.h                             \
	src/interfaces/SCUbusActionSink.h                            \
	src/interfaces/iEmbeddedCPUActionSink.h                      \
	src/interfaces/iSAFTd.h                                      \
	src/interfaces/EmbeddedCPUActionSink.h                       \
	src/interfaces/SCUbusCondition.h                             \
	src/interfaces/iEmbeddedCPUCondition.h                       \
	src/interfaces/iSCUbusActionSink.h                           \
	src/interfaces/EmbeddedCPUCondition.h                        \
	src/interfaces/SoftwareActionSink.h                          \
	src/interfaces/iEventSource.h                                \
	src/interfaces/iSCUbusCondition.h                            \
	src/interfaces/FunctionGenerator.h                           \
	src/interfaces/SoftwareCondition.h                           \
	src/interfaces/iFunctionGenerator.h                          \
	src/interfaces/iSoftwareActionSink.h                         \
	src/interfaces/FunctionGeneratorFirmware.h                   \
	src/interfaces/TimingReceiver.h                              \
	src/interfaces/iFunctionGeneratorFirmware.h                  \
	src/interfaces/iSoftwareCondition.h                          \
	src/interfaces/Input.h                                       \
	src/interfaces/WbmActionSink.h                               \
	src/interfaces/iInputEventSource.h                           \
	src/interfaces/iTimingReceiver.h                             \
	src/interfaces/MILbusActionSink.h                            \
	src/interfaces/WbmCondition.h                                \
	src/interfaces/iMILbusActionSink.h                           \
	src/interfaces/iWbmActionSink.h                              \
	src/interfaces/MILbusCondition.h                             \
	src/interfaces/WrMilGateway.h                                \
	src/interfaces/iMILbusCondition.h                            \
	src/interfaces/iWbmCondition.h                               \
	src/interfaces/MasterFunctionGenerator.h                     \
	src/interfaces/iActionSink.h                                 \
	src/interfaces/iMasterFunctionGenerator.h                    \
	src/interfaces/iWrMilGateway.h                               \
	src/interfaces/Output.h                                      \
	src/interfaces/iBurstGenerator.h                             \
	src/interfaces/iOutputActionSink.h                           \
	src/interfaces/OutputCondition.h                             \
	src/interfaces/iCondition.h                                  \
	src/interfaces/iOutputCondition.h

if GIT_TREE
version.h:
	echo '#define GIT_ID "'$$(git describe --dirty --always --tags)'"' > version.h.tmp
	echo '#define SOURCE_DATE "'$$(date +'%b %d %Y %H:%M:%S' --date=@$$(git log -n1 --pretty='format:%ct'))'"' >> version.h.tmp
	mv -f version.h.tmp version.h
debian:
	cp debian/libsaftlib.install.in debian/libsaftlib@SAFTLIB_SONAME@.install
	./config.status --file=debian/control
endif



bin_PROGRAMS = eb-plugin-ctl eb-standalone software-tr soft-tr wait-msi saft-ctl saft-io-ctl saft-pps-gen saft-scu-ctl saft-ecpu-ctl saft-wbm-ctl saft-clk-gen saft-dm saft-eb-fwd saft-gmt-check  saft-uni saft-lcd saft-standalone-mbox
eb_plugin_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
eb_plugin_ctl_SOURCES = src/eb-plugin-ctl.cpp

eb_standalone_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
eb_standalone_SOURCES = src/eb-standalone.cpp

# software timing receiver program
software_tr_sdb_DATA = src/software-tr.sdb
software_tr_sdbdir   = $(datadir)/@PACKAGE@
software_tr_LDADD    = $(EB_LIBS) -lpthread
software_tr_SOURCES  = src/software-tr.cpp 

# another software timing receiver program
soft_tr_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
soft_tr_SOURCES  = src/soft-tr.cpp 

# simple program to find out how to receive MSIs from soft-tr
wait_msi_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
wait_msi_SOURCES  = src/wait-msi.cpp 


saft_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) -lpthread lib@PACKAGE@-proxy.la
saft_ctl_SOURCES = src/saft-ctl.cpp

saft_io_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_io_ctl_SOURCES = src/saft-io-ctl.cpp

saft_pps_gen_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_pps_gen_SOURCES = src/saft-pps-gen.cpp

saft_scu_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_scu_ctl_SOURCES = src/saft-scu-ctl.cpp

saft_ecpu_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_ecpu_ctl_SOURCES = src/saft-ecpu-ctl.cpp

saft_wbm_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_wbm_ctl_SOURCES = src/saft-wbm-ctl.cpp

saft_clk_gen_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_clk_gen_SOURCES = src/saft-clk-gen.cpp

saft_dm_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_dm_SOURCES = src/saft-dm.cpp

saft_eb_fwd_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_eb_fwd_SOURCES = src/saft-eb-fwd.cpp

saft_gmt_check_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_gmt_check_SOURCES = src/saft-gmt-check.cpp

saft_uni_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_uni_SOURCES = src/saft-uni.cpp

saft_lcd_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) lib@PACKAGE@-proxy.la
saft_lcd_SOURCES = src/saft-lcd.cpp

saft_standalone_mbox_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) $(SIGCPP_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
saft_standalone_mbox_SOURCES = src/saft-standalone-mbox.cpp


# autogenerated files
%_Service.cpp %_Service.hpp %_Proxy.cpp %_Proxy.hpp: %.hpp
	saftbus-gen $< -o src

# cleaning of autogenerated files
CLEANFILES = $(top_builddir)/src/*_Proxy.*pp $(top_builddir)/src/*_Service.*pp 
