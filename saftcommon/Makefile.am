
AM_CPPFLAGS = $(SIGCPP_CFLAGS) -I $(top_srcdir)/saftcommon -DDATADIR='"$(datadir)"'

lib_LTLIBRARIES = libsaftcommon.la 
saftcommondir = $(includedir)/saftlib
saftcommon_HEADERS =         \
	Time.h             \
	CommonFunctions.h  \
	Logger.h           \
	core.h             \
	Proxy.h            \
	Error.h            \
	Connection.h       \
	ProxyConnection.h  \
	Interface.h        \
	SignalGroup.h      \
	MainContext.h      \
	MainLoop.h         \
	Source.h           \
	PollFD.h	       \
	saftbus.h          \
	chunck_allocator_rt.h\
	Data.hpp              \
	GELFConfig.hpp        \
	InternalGELF.hpp      \
	PackageBuilder.hpp    \
	PackageCompressor.hpp \
	UDPProtocol.hpp       \
	DllDefines.hpp        \
	GELF.hpp              \
	MessageBuilder.hpp    


leapseconddir = $(datadir)/saftlib
leapsecond_DATA = leap-seconds.list
EXTRA_DIST = leap-seconds.list

# saftcommondir = $(includedir)/saftlib
# saftcommon_DATA = leap-seconds.list

libsaftcommon_la_LIBADD =  
libsaftcommon_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

libsaftcommon_la_SOURCES =     \
	Time.cpp                   \
	CommonFunctions.cpp        \
	Logger.cpp                 \
	core.cpp                   \
	Proxy.cpp                  \
	Error.cpp                  \
	Connection.cpp             \
	ProxyConnection.cpp        \
	Interface.cpp              \
	SignalGroup.cpp            \
	saftbus.cpp                \
	chunck_allocator_rt.cpp    \
	MainContext.cpp            \
	MainLoop.cpp               \
	Source.cpp                 \
	PollFD.cpp     		       \
	GELF.cpp                   \
	MessageBuilder.cpp         \
	PackageBuilder.cpp         \
	PackageCompressor.cpp      \
	UDPProtocol.cpp            

bin_PROGRAMS = saftbus-ctl

saftbus_ctl_SOURCES = saftbus-ctl.cpp
saftbus_ctl_LDADD   = libsaftcommon.la $(SIGCPP_LIBS) -lz

