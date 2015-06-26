AC_PREREQ(2.63)

AC_INIT([saftlib], [0.1])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([tools])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)
AX_CXXFLAGS_WARN_ALL

PKG_PROG_PKG_CONFIG([0.23])
PKG_CHECK_MODULES([EB], [etherbone >= 0.1])
PKG_CHECK_MODULES([GIOMM], [giomm-2.4 >= 2.28.2])
AC_PATH_PROGS([XSLTPROC], [xsltproc])
if test "x$XSLTPROC" = "x"; then
  AC_MSG_ERROR([could not find xsltproc -- needed to processs interfaces])
fi

# Generate documentation, only if we can
AC_PATH_PROGS([GDBUS_CODEGEN], [gdbus-codegen])
AC_PATH_PROGS([DOCBOOK2PDF], [docbook2pdf])
AM_CONDITIONAL([INTERFACE_DOCS], [test x$DOCBOOK2PDF != x -a x$GDBUS_CODEGEN != x])

AC_CONFIG_FILES([Makefile interfaces/Makefile])
AC_OUTPUT
