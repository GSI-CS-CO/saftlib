#! /bin/sh
cd $(dirname $(readlink -e "$0"))
set -ex
git log > ChangeLog
aclocal -I m4
autoheader
autoconf
libtoolize -c
automake -a -c
