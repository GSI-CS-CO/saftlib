AC_INIT([eb-plugin], [0.0])
LT_INIT
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

PKG_CHECK_MODULES([SAFTBUS], [saftbus])
PKG_CHECK_MODULES([EB], [etherbone >= 2.1.0], [AC_DEFINE([HAVE_EB], [1], [Use EB])] , [AC_DEFINE([NO_EB], [1], [No etherbone was found, compiling without etherbone support])] )

AC_CONFIG_FILES([Makefile])
AC_OUTPUT