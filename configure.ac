AC_INIT([mini-saftlib], [0.0])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

PKG_CHECK_MODULES([EB], [etherbone >= 2.1.0], [AC_DEFINE([HAVE_EB], [1], [Use EB])] , [AC_DEFINE([NO_EB], [1], [No etherbone was found, compiling without etherbone support])] )
PKG_CHECK_MODULES([SIGC], [sigc++-2.0 >= 2.10.0])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT