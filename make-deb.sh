#! /bin/sh
JOBS=${JOBS:-4} # run 4 commands at once

cd $(dirname $(readlink -e "$0"))
set -ex
./autogen.sh
./configure --enable-maintainer-mode
make -j $JOBS distcheck
make debian

tarball=$(echo saftlib-*.tar.xz)
ver=${tarball##*-}
ver=${ver%%.tar.xz}
orig=saftlib_$ver.orig.tar.xz

rm -rf build
mkdir build
mv $tarball build/$orig
cd build
tar xvJf $orig
cp -a ../debian saftlib-${ver}/
cd saftlib-${ver}
debuild -eDEB_BUILD_OPTIONS="parallel=$JOBS"
