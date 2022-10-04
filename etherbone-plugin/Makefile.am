AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SAFTBUS_CFLAGS) -I $(top_srcdir)/src -I $(top_builddir)/src -DDATADIR='"$(datadir)/@PACKAGE@"'

lib_LTLIBRARIES = \
	lib@PACKAGE@_service.la  \
	lib@PACKAGE@_proxy.la    
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
	src/OpenDevice.cpp                            \
	src/OpenDevice_Service.cpp                     \
	src/WhiteRabbit.cpp                             \
	src/WhiteRabbit_Service.cpp                      \
	src/Watchdog.cpp                                  \
	src/SerdesClockGen.cpp                             \
	src/Io.cpp                                          \
	src/IoControl.cpp                                    \
	src/Output.cpp                                        \
	src/Output_Service.hpp                                 \
	src/ECA.cpp                                             \
	src/ECA_Service.cpp                                      \
	src/BuildIdRom.cpp                                        \
	src/BuildIdRom_Service.cpp                                 \
	src/TempSensor.cpp                                          \
	src/TempSensor_Service.cpp                                   \
	src/TimingReceiver.cpp                                        \
	src/TimingReceiver_Service.cpp                                 \
	src/create_service.cpp

lib@PACKAGE@_service_includedir = $(includedir)/@PACKAGE@/service
lib@PACKAGE@_service_include_HEADERS = \
	src/Time.hpp                        \
	src/eb-source.hpp                    \
	src/eb-forward.hpp                    \
	src/Owned.hpp                          \
	src/Owned_Service.hpp                   \
	src/SAFTd.hpp                            \
	src/SAFTd_Service.hpp                     \
	src/Condition.hpp                          \
	src/Condition_Service.hpp                   \
	src/ActionSink.hpp                           \
	src/ActionSink_Service.hpp                    \
	src/SoftwareCondition.hpp                      \
	src/SoftwareCondition_Service.hpp               \
	src/SoftwareActionSink.hpp                       \
	src/SoftwareActionSink_Service.hpp                \
	src/OpenDevice.hpp                                 \
	src/OpenDevice_Service.hpp                          \
	src/WhiteRabbit.hpp                                  \
	src/WhiteRabbit_Service.hpp                           \
	src/Watchdog.hpp                                       \
	src/SerdesClockGen.hpp                                  \
	src/Io.hpp                                               \
	src/IoControl.hpp                                         \
	src/Output.hpp                                             \
	src/Output_Service.hpp                                      \
	src/ECA.hpp                                                  \
	src/ECA_Service.hpp                                           \
	src/BuildIdRom.hpp                                             \
	src/BuildIdRom_Service.hpp                                      \
	src/TempSensor.hpp                                               \
	src/TempSensor_Service.hpp                                        \
	src/TimingReceiver.hpp                                             \
	src/TimingReceiver_Service.hpp                           

lib@PACKAGE@_proxy_la_LIBADD  = $(SAFTBUS_LIBS)
lib@PACKAGE@_proxy_la_SOURCES =   \
	src/Time.cpp                   \
	src/Owned_Proxy.cpp             \
	src/SAFTd_Proxy.cpp              \
	src/Condition_Proxy.cpp           \
	src/ActionSink_Proxy.cpp           \
	src/SoftwareCondition_Proxy.cpp     \
	src/SoftwareActionSink_Proxy.cpp     \
	src/OpenDevice_Proxy.cpp              \
	src/WhiteRabbit_Proxy.cpp              \
	src/Output_Proxy.cpp                    \
	src/ECA_Proxy.cpp                        \
	src/BuildIdRom_Proxy.cpp                  \
	src/TempSensor_Proxy.cpp                   \
	src/TimingReceiver_Proxy.cpp         


lib@PACKAGE@_proxy_includedir = $(includedir)/@PACKAGE@
lib@PACKAGE@_proxy_include_HEADERS = \
	src/Time.hpp                      \
	src/Owned_Proxy.hpp                \
	src/SAFTd_Proxy.hpp                 \
	src/Condition_Proxy.hpp              \
	src/ActionSink_Proxy.hpp              \
	src/SoftwareCondition_Proxy.hpp        \
	src/SoftwareActionSink_Proxy.hpp        \
	src/OpenDevice_Proxy.hpp                 \
	src/WhiteRabbit_Proxy.hpp                 \
	src/Output_Proxy.hpp                       \
	src/ECA_Proxy.hpp                           \
	src/BuildIdRom_Proxy.hpp                     \
	src/TempSensor_Proxy.hpp                      \
	src/TimingReceiver_Proxy.hpp

# the file leap-seconds.list ist needed by the Time class to convert between UTC and WR-time
leapseconddir = $(datadir)/@PACKAGE@
leapsecond_DATA = leap-seconds.list
EXTRA_DIST = leap-seconds.list



bin_PROGRAMS = eb-plugin-ctl eb-standalone software-tr soft-tr wait-msi
eb_plugin_ctl_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) lib@PACKAGE@_proxy.la
eb_plugin_ctl_SOURCES = src/eb-plugin-ctl.cpp

eb_standalone_LDADD   = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@_service.la
eb_standalone_SOURCES = src/eb-standalone.cpp

# software timing receiver program
software_tr_sdb_DATA = src/software-tr.sdb
software_tr_sdbdir   = $(datadir)/@PACKAGE@
software_tr_LDADD    = $(EB_LIBS)
software_tr_SOURCES  = src/software-tr.cpp 

# another software timing receiver program
soft_tr_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@_service.la
soft_tr_SOURCES  = src/soft-tr.cpp 

# simple program to find out how to receive MSIs from soft-tr
wait_msi_LDADD    = $(EB_LIBS) $(SAFTBUS_LIBS) -lsaftbus-standalone lib@PACKAGE@_service.la
wait_msi_SOURCES  = src/wait-msi.cpp 


# autogenerated files
%_Service.cpp %_Service.hpp %_Proxy.cpp %_Proxy.hpp: %.hpp
	saftbus-gen $< -o src -v

# cleaning of autogenerated files
CLEANFILES = $(top_builddir)/src/*_Proxy.*pp $(top_builddir)/src/*_Service.*pp 
