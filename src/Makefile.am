bin_PROGRAMS = eb-dbus client

AM_CPPFLAGS = $(EB_CFLAGS) $(GIOMM_CFLAGS)

eb_dbus_SOURCES = GlobalObject.cpp eb-source.cpp main.cpp
eb_dbus_LDADD = $(EB_LIBS) $(GIOMM_LIBS) ../interfaces/libsaftlib.la -leca

client_SOURCES  = client.cpp
client_LDADD = $(GIOMM_LIBS) ../interfaces/libsaftlib.la
