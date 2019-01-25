
AM_CPPFLAGS = $(GIOMM_CFLAGS) -I $(top_srcdir)/saftbus

lib_LTLIBRARIES = libsaftbus.la
saftbusdir = $(includedir)/saftlib
saftbus_HEADERS = \
	Logger.h       \
	core.h          \
	Proxy.h          \
	Error.h           \
	Socket.h           \
	Connection.h       \
	ProxyConnection.h  \
	Interface.h        \
	SignalGroup.h      \
	saftbus.h          \
	MainContext.h		\
	MainLoop.h			\
	Source.h			\
	PollFD.h			

libsaftbus_la_LIBADD = 
libsaftbus_la_LDFLAGS = -version-info @SAFTLIB_API@:@SAFTLIB_REVISION@:@SAFTLIB_MINOR@ # @SAFTLIB_SONAME@

nodist_libsaftbus_la_SOURCES = \
	Logger.cpp                  \
	core.cpp                     \
	Proxy.cpp                    \
	Error.cpp                    \
	Socket.cpp                   \
	Connection.cpp               \
	ProxyConnection.cpp          \
	Interface.cpp                \
	SignalGroup.cpp              \
	saftbus.cpp                  \
	MainContext.cpp		         \
	MainLoop.cpp		         \
	Source.cpp			         \
	PollFD.cpp			          
bin_PROGRAMS = saftbus-ctl

saftbus_ctl_SOURCES = saftbus-ctl.cpp
saftbus_ctl_LDADD   = $(GIOMM_LIBS) libsaftbus.la



