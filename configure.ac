AC_PREREQ(2.63)

AC_INIT([saftlib], [0.1])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(tools)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

if test "x$USE_MAINTAINER_MODE" != "xno"; then
  CXXFLAGS="$CXXFLAGS -Wall -O0 -g"
fi

PKG_PROG_PKG_CONFIG([0.25])
PKG_CHECK_MODULES([EB], [etherbone >= 0.1])
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.8.12],
  [ac_dbus_rawservicedir="$($PKG_CONFIG --variable=system_bus_services_dir dbus-1)"
   ac_dbus_prefixdir="$($PKG_CONFIG --variable=prefix dbus-1)"
   ac_dbus_servicedir="\${prefix}/${ac_dbus_rawservicedir#${ac_dbus_prefixdir}}"],
  [ac_dbus_servicedir=;])
PKG_CHECK_MODULES([GIOMM], [giomm-2.4 >= 2.42])
AC_PATH_PROGS([XSLTPROC], [xsltproc])

if test "x$ac_dbus_servicedir" = x ; then
  AC_MSG_ERROR([could not find dbus-1 system_bus_service_dir])
fi
AC_SUBST(DBUS_SERVICEDIR, $ac_dbus_servicedir)

AC_CONFIG_FILES([Makefile interfaces/Makefile devices/Makefile drivers/Makefile src/Makefile])
AC_OUTPUT
