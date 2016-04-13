#! /bin/sh
JOBS=${JOBS:-4} # run 4 commands at once

cd $(dirname $(readlink -e "$0"))
set -ex
./autogen.sh
./configure --enable-maintainer-mode
make -j $JOBS distcheck
make debian

tarball=$(echo saftlib-*.tar.gz)
ver=${tarball##*-}
ver=${ver%%.tar.gz}
orig=saftlib_$ver.orig.tar.gz

rm -rf build
mkdir build
mv $tarball build/$orig
cd build
tar xvzf $orig
cp -a ../debian saftlib-${ver}/
cd saftlib-${ver}
debuild -eDEB_BUILD_OPTIONS="parallel=$JOBS"
