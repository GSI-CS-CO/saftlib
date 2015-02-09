AC_PREREQ(2.63)

AC_INIT([saftlib], [0.1])
AC_CONFIG_SRCDIR([src/service.cpp])
AC_CONFIG_AUX_DIR(tools)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

if test "x$USE_MAINTAINER_MODE" != "xno"; then
  CXXFLAGS="$CXXFLAGS -Wall -O0 -g"
fi

PKG_PROG_PKG_CONFIG([0.25])
PKG_CHECK_MODULES([EB], [etherbone])
PKG_CHECK_MODULES([DBUS], [dbus-glib-1], 
  [ac_dbus_confdir=`$PKG_CONFIG --variable=system_bus_services_dir dbus-1`],
  [ac_dbus_confdir=;])

if test "x$ac_dbus_confdir" = x ; then
  AC_MSG_ERROR([could not find dbus-1 system_bus_service_dir])
fi

AC_SUBST(DBUS_CONFDIR, $ac_dbus_confdir)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
