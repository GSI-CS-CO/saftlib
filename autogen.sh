#! /bin/sh
aclocal
autoheader
autoconf
libtoolize -c
automake -a -c
