AUTOMAKE_OPTIONS=foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -g  $(SIGC_CFLAGS) -I $(top_srcdir)/src -I /home/michael/local/include/saftbus -I /home/michael/local/include

lib_LTLIBRARIES = \
	libsaftbus_example_service.la  \
	libsaftbus_example_proxy.la    

%_Service.cpp: %.hpp
	saftbus-gen $< -o src

%_Service.hpp: %.hpp
	saftbus-gen $< -o src

%_Proxy.cpp: %.hpp
	saftbus-gen $< -o src

%_Proxy.hpp: %.hpp
	saftbus-gen $< -o src

# libsaftbus_example_service_la_CPPADD  = $(EB_CFLAGS)
# libsaftbus_example_service_la_LIBADD  = $(EB_LIBS)
libsaftbus_example_service_la_SOURCES = \
	src/Dice.cpp src/Dice.hpp      src/Dice_Service.cpp    src/Dice_Service.hpp \
	src/create_service.cpp
	

libsaftbus_example_proxy_la_LIBADD  = -L/home/michael/local/lib -lsaftbus
libsaftbus_example_proxy_la_SOURCES = \
	src/Dice_Proxy.cpp     
saftbus_example_proxy_includedir = $(includedir)/saftbus
saftbus_example_proxy_include_HEADERS = \
	src/Dice_Proxy.hpp       

bin_PROGRAMS = saftbus-example-ctl
saftbus_example_ctl_LDADD  = $(SIGC_LIBS) -L/home/michael/local/lib -lsaftbus libsaftbus_example_proxy.la
saftbus_example_ctl_SOURCES = src/saftbus-example-ctl.cpp 
