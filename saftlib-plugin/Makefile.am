AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SAFTBUS_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)/src -DDATADIR='"$(datadir)/@PACKAGE@"'

lib_LTLIBRARIES = \
	lib@PACKAGE@-service.la  \
	lib@PACKAGE@-proxy.la    
# @PACKAGE@ is substituded with the string given to AC_INIT([...])
lib@PACKAGE@_service_la_LDFLAGS = -Wl,--export-dynamic 
lib@PACKAGE@_service_la_CPPADD  = $(EB_CFLAGS) 
lib@PACKAGE@_service_la_LIBADD  = $(EB_LIBS)
lib@PACKAGE@_service_la_SOURCES = \
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
	src/OpenDevice.cpp                                        \
	src/OpenDevice_Service.cpp                                 \
	src/WhiteRabbit.cpp                                         \
	src/WhiteRabbit_Service.cpp                                  \
	src/Mailbox.cpp                                               \
	src/Watchdog.cpp                                               \
	src/SerdesClockGen.cpp                                          \
	src/Io.cpp                                                       \
	src/IoControl.cpp                                                 \
	src/Output.cpp                                                     \
	src/Output_Service.cpp                                              \
	src/OutputCondition.cpp                                              \
	src/OutputCondition_Service.cpp                                       \
	src/EventSource.cpp                                                    \
	src/Input.cpp                                                           \
	src/Input_Service.cpp                                                    \
	src/ECA.cpp                                                               \
	src/ECA_Service.cpp                                                        \
	src/ECA_TLU.cpp                                                             \
	src/BuildIdRom.cpp                                                           \
	src/BuildIdRom_Service.cpp                                                    \
	src/TempSensor.cpp                                                             \
	src/TempSensor_Service.cpp                                                      \
	src/TimingReceiver.cpp                                                           \
	src/TimingReceiver_Service.cpp                                                    \
	src/create_service.cpp

lib@PACKAGE@_service_includedir = $(includedir)/saftlib/service
lib@PACKAGE@_service_include_HEADERS = \
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
	src/OpenDevice.hpp                                 \
	src/WhiteRabbit.hpp                                 \
	src/Mailbox.hpp                                      \
	src/Watchdog.hpp                                      \
	src/SerdesClockGen.hpp                                 \
	src/Io.hpp                                              \
	src/IoControl.hpp                                        \
	src/Output.hpp                                            \
	src/OutputCondition.hpp                                    \
	src/EventSource.hpp                                         \
	src/Input.hpp                                                \
	src/ECA.hpp                                                   \
	src/ECA_TLU.cpp                                                \
	src/BuildIdRom.hpp                                              \
	src/TempSensor.hpp                                               \
	src/TimingReceiver.hpp                                        
# 	src/Owned_Service.hpp                  \
# 	src/SAFTd_Service.hpp                   \
# 	src/Condition_Service.hpp                \
# 	src/ActionSink_Service.hpp                \
# 	src/SoftwareCondition_Service.hpp          \
# 	src/SoftwareActionSink_Service.hpp          \
# 	src/SCUbusCondition_Service.hpp              \
# 	src/SCUbusActionSink_Service.hpp              \
# 	src/EmbeddedCPUCondition_Service.hpp           \
# 	src/EmbeddedCPUActionSink_Service.hpp           \
# 	src/WbmCondition_Service.hpp                     \
# 	src/WbmActionSink_Service.hpp                     \
# 	src/OpenDevice_Service.hpp                         \
# 	src/WhiteRabbit_Service.hpp                         \
# 	src/Output_Service.hpp                               \
# 	src/OutputCondition_Service.hpp                       \
# 	src/Input_Service.hpp                                  \
# 	src/ECA_Service.hpp                                     \
# 	src/BuildIdRom_Service.hpp                               \
# 	src/TempSensor_Service.hpp                                \
# 	src/TimingReceiver_Service.hpp                           
# the service headers are not neede by client programs (event standalone ones), right???

lib@PACKAGE@_proxy_la_LIBADD  = $(SAFTBUS_LIBS)
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
	src/TimingReceiver_Proxy.cpp         


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
	src/TimingReceiver_Proxy.hpp 

# the file leap-seconds.list ist needed by the Time class to convert between UTC and WR-time
leapseconddir = $(datadir)/@PACKAGE@
leapsecond_DATA = leap-seconds.list
EXTRA_DIST = leap-seconds.list

dist_bin_SCRIPTS = src/saftd


bin_PROGRAMS = eb-plugin-ctl eb-standalone software-tr soft-tr wait-msi saft-ctl saft-io-ctl saft-pps-gen saft-scu-ctl saft-ecpu-ctl saft-wbm-ctl saft-clk-gen saft-dm saft-eb-fwd saft-gmt-check  saft-uni
eb_plugin_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
eb_plugin_ctl_SOURCES = src/eb-plugin-ctl.cpp

eb_standalone_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
eb_standalone_SOURCES = src/eb-standalone.cpp

# software timing receiver program
software_tr_sdb_DATA = src/software-tr.sdb
software_tr_sdbdir   = $(datadir)/@PACKAGE@
software_tr_LDADD    = $(EB_LIBS)
software_tr_SOURCES  = src/software-tr.cpp 

# another software timing receiver program
soft_tr_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
soft_tr_SOURCES  = src/soft-tr.cpp 

# simple program to find out how to receive MSIs from soft-tr
wait_msi_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@-service.la
wait_msi_SOURCES  = src/wait-msi.cpp 


saft_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_ctl_SOURCES = src/saft-ctl.cpp src/CommonFunctions.cpp

saft_io_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_io_ctl_SOURCES = src/saft-io-ctl.cpp src/CommonFunctions.cpp

saft_pps_gen_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_pps_gen_SOURCES = src/saft-pps-gen.cpp src/CommonFunctions.cpp

saft_scu_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_scu_ctl_SOURCES = src/saft-scu-ctl.cpp src/CommonFunctions.cpp

saft_ecpu_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_ecpu_ctl_SOURCES = src/saft-ecpu-ctl.cpp src/CommonFunctions.cpp

saft_wbm_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_wbm_ctl_SOURCES = src/saft-wbm-ctl.cpp src/CommonFunctions.cpp

saft_clk_gen_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_clk_gen_SOURCES = src/saft-clk-gen.cpp src/CommonFunctions.cpp

saft_dm_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_dm_SOURCES = src/saft-dm.cpp src/CommonFunctions.cpp

saft_eb_fwd_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_eb_fwd_SOURCES = src/saft-eb-fwd.cpp src/CommonFunctions.cpp

saft_gmt_check_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_gmt_check_SOURCES = src/saft-gmt-check.cpp src/CommonFunctions.cpp

saft_uni_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@-proxy.la
saft_uni_SOURCES = src/saft-uni.cpp src/CommonFunctions.cpp


# autogenerated files
%_Service.cpp %_Service.hpp %_Proxy.cpp %_Proxy.hpp: %.hpp
	saftbus-gen $< -o src

# cleaning of autogenerated files
CLEANFILES = $(top_builddir)/src/*_Proxy.*pp $(top_builddir)/src/*_Service.*pp 
