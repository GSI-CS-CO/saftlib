#! /bin/sh
aclocal -I m4
autoheader
autoconf
libtoolize -c
automake -a -c
