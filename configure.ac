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

AC_MSG_CHECKING(whether compiler understands -Wall)
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Wall"
AC_TRY_COMPILE([],[],
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
  CXXFLAGS="$old_CXXFLAGS")

AC_CACHE_CHECK([for lsb_release supporting -d -s], [ac_cv_path_LSB_RELEASE_CMD],
 [AC_PATH_PROGS_FEATURE_CHECK([LSB_RELEASE_CMD], [lsb_release],
  [[$ac_path_LSB_RELEASE_CMD -d -s > /dev/null && ac_cv_path_LSB_RELEASE_CMD=$ac_path_LSB_RELEASE_CMD]],
  [AC_MSG_ERROR([could not find acceptable lsb_release])])])
AC_CACHE_CHECK([for operating system], [ac_cv_OPERATING_SYSTEM],
  [ac_cv_OPERATING_SYSTEM=$($ac_cv_path_LSB_RELEASE_CMD -d -s)])

AC_DEFINE_UNQUOTED([OPERATING_SYSTEM], ["$ac_cv_OPERATING_SYSTEM"], [The operating system of the build system])
AC_DEFINE_UNQUOTED([HOSTNAME], ["${HOSTNAME:=unknown}"], [The hostname of the build system])
AC_DEFINE_UNQUOTED([USERNAME], ["${USERNAME:=unknown}"], [The username of the person compiling saftlib])

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
AC_PATH_PROGS([GIT], [git])
AM_CONDITIONAL([INTERFACE_DOCS], [test x$DOCBOOK2PDF != x -a x$GDBUS_CODEGEN != x])
AM_CONDITIONAL([GIT_TREE], [test x$GIT != x -a -d .git])

AC_CONFIG_FILES([Makefile interfaces/Makefile])
AC_OUTPUT
