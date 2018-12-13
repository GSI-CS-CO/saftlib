dnl Set these variables before each release:
m4_define(MAJOR,1)    dnl Increment if removed/changed properties/signals/methods since previous release
m4_define(MINOR,4)    dnl Increment if added properties/signals/methods; reset to 0 if MAJOR changed
m4_define(REVISION,3) dnl Increment on each release; reset to 0 if MAJOR/MINOR changed
m4_define(SONAME,3)   dnl Whenever MAJOR is incremented, add MINOR+1 to this variable

AC_PREREQ(2.63)

AC_INIT([saftlib], MAJOR.MINOR.REVISION)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([tools])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([subdir-objects dist-xz no-dist-gzip])
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)
AX_CXX_COMPILE_STDCXX(11,noext,optional)

dnl We need long long even before c++11 for UINT64_C, PRIx64, and friends
AC_MSG_CHECKING(whether compiler understands -Wall)
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Wall -Wmissing-declarations -Wno-long-long"
AC_TRY_COMPILE([],[],
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
  CXXFLAGS="$old_CXXFLAGS")

AC_PATH_PROG([UNAME], [uname], [false])
AC_PATH_PROG([LSB_RELEASE], [lsb_release], [false])
AC_PATH_PROG([SW_VERS], [sw_vers], [false])

AC_CACHE_CHECK([for operating system], [ac_cv_OS], [AS_CASE([`${UNAME} -s`],
  [Linux], [AS_IF(["${LSB_RELEASE}" -d -s 2>/dev/null >/dev/null],
                  [os=`"${LSB_RELEASE}" -d -s`; os="${os%\"}"; ac_cv_OS="${os#\"}"],
                  [AC_MSG_ERROR([lsb_release is required])])],
  [Darwin], [AS_IF(["${SW_VERS}" 2>/dev/null >/dev/null],
                   [ac_cv_OS="`${SW_VERS} -productName` `${SW_VERS} -productVersion`"],
                   [AC_MSG_ERROR([sw_vers is required])])],
  [ac_cv_OS="`${UNAME} -s`"])])

AC_DEFINE_UNQUOTED([OPERATING_SYSTEM], ["$ac_cv_OS"], [The operating system of the build system])
AC_DEFINE_UNQUOTED([HOSTNAME], ["${HOSTNAME:-unknown}"], [The hostname of the build system])
AC_DEFINE_UNQUOTED([USERNAME], ["${USERNAME:-${LOGNAME:-${USER:-unknown}}}"], [The username of the person compiling saftlib])

PKG_PROG_PKG_CONFIG([0.23])
PKG_CHECK_MODULES([EB], [etherbone >= 2.1.0])
PKG_CHECK_MODULES([GIOMM], [giomm-2.4 >= 2.28.2 gio-unix-2.0])
AC_PATH_PROGS([XSLTPROC], [xsltproc])
if test "x$XSLTPROC" = "x"; then
  AC_MSG_ERROR([could not find xsltproc -- needed to processs interfaces])
fi

dnl Generate documentation, only if we can
AC_PATH_PROGS([GDBUS_CODEGEN], [gdbus-codegen])
AC_PATH_PROGS([DOCBOOK2PDF], [docbook2pdf])
AC_PATH_PROGS([DOCBOOK2MAN], [docbook2man])
AC_PATH_PROGS([GIT], [git])
AM_CONDITIONAL([INTERFACE_DOCS], [test x$DOCBOOK2PDF != x -a x$GDBUS_CODEGEN != x])
AM_CONDITIONAL([REBUILD_MAN_PAGES], [test x$DOCBOOK2MAN != x])
AM_CONDITIONAL([GIT_TREE], [test x$GIT != x -a -e .git])
if test -e .git -a "x$DOCBOOK2MAN" = "x"; then
  AC_MSG_ERROR([Building from a git checkout requires docbook2man])
fi

AC_SUBST([SAFTLIB_MAJOR], MAJOR)
AC_SUBST([SAFTLIB_MINOR], MINOR)
AC_SUBST([SAFTLIB_REVISION], REVISION)
AC_SUBST([SAFTLIB_API], [$((SONAME+MINOR))]) dnl API incremented on every change
AC_SUBST([SAFTLIB_SONAME], SONAME)

AC_CONFIG_FILES([Makefile interfaces/Makefile interfaces/h.xsl saftlib.pc])
AC_OUTPUT
