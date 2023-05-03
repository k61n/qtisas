#!/bin/bash

os=$1
arch=$2
cores=$3
cores=${cores:-1}
libdir=$4
CC=$5
CXX=$6

cd $libdir
file1="../../libs/$os-$arch/gsl/lib/libgsl.a"
file2="../../libs/$os-$arch/gsl/lib/libgslcblas.a"

if [ -f "$file1" ] && [ -f "$file2" ]; then
  echo "Gsl libraries are already built: $file1 and $file2"
  exit 0
fi

echo Building gsl library

git reset --hard origin/master &> /dev/null
git checkout release-1-15 &> /dev/null
git clean -f -d &> /dev/null
autoreconf -i > autoconf.log 2>&1

rm -rf tmp
mkdir tmp
cd tmp

install_path_relative="../../../libs/$os-$arch/gsl"

mkdir -p $install_path_relative
install_path=$(readlink -f "$install_path_relative")

CC=$CC CXX=$CXX ../configure -q --disable-shared --disable-dependency-tracking > configure.log 2>&1

if [[ $os == "Darwin" ]]; then
  gmake -j$cores MAKEINFO=true > gmake.log 2>&1
  gmake install DESTDIR=$install_path prefix= MAKEINFO=true > install.log 2>&1
else
  make -j$cores MAKEINFO=true > make.log 2>&1
  make install DESTDIR=$install_path prefix= MAKEINFO=true > install.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error building gsl
  exit 1
fi

cd ..
git reset --hard origin/master &> /dev/null
git clean -f -d &> /dev/null

exit 0
