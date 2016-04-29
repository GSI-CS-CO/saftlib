#! /bin/sh
cd $(dirname "$0")
set -ex
git log > ChangeLog
aclocal -I m4
autoheader
autoconf
which libtoolize >/dev/null && libtoolize -c || glibtoolize -c
automake -a -c
