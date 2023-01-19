AC_INIT([SAFTd], [0.0])
LT_INIT
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

PKG_CHECK_MODULES([SAFTBUS], [saftbus])
PKG_CHECK_MODULES([EB], [etherbone >= 2.1.0], [AC_DEFINE([HAVE_EB], [1], [Use EB])] , [AC_DEFINE([NO_EB], [1], [No etherbone was found, compiling without etherbone support])] )
PKG_CHECK_MODULES([SIGCPP], [sigc++-2.0])

#####################################
# the following is needed to extract build info (relevant for src/build.cpp src/build.hpp version.h)
#####################################
AC_CACHE_CHECK([for operating system], [ac_cv_OS], [os=`cat /etc/os-release | grep NAME | head -n 1 | cut -d"=" -f2`; os="${os%\"}"; ac_cv_OS="${os#\"}" ])
AC_DEFINE_UNQUOTED([OPERATING_SYSTEM], ["$ac_cv_OS"], [The operating system of the build system])
AC_DEFINE_UNQUOTED([HOSTNAME], ["${HOSTNAME:-unknown}"], [The hostname of the build system])
AC_DEFINE_UNQUOTED([USERNAME], ["${USERNAME:-${LOGNAME:-${USER:-unknown}}}"], [The username of the person compiling saftlib])
AC_PATH_PROGS([GIT], [git])
AM_CONDITIONAL([GIT_TREE], [test x$GIT != x -a -e ../.git])
#####################################
# end of build info stuff
#####################################


AC_CONFIG_FILES([Makefile saftlib.pc])
AC_OUTPUT