#!/bin/bash

os=$1
arch=$2
cores=$3
cores=${cores:-1}

cd $4
file1="../../libs/$os-$arch/gsl/lib/libgsl.a"
file2="../../libs/$os-$arch/gsl/lib/libgslcblas.a"

if [ -f "$file1" ] && [ -f "$file2" ]; then
  echo "Gsl libraries are already built: $file1 and $file2"
  exit 0
fi

echo Building gsl library

rm -rf tmp
mkdir tmp
cd tmp

install_path_relative="../../../libs/$os-$arch/gsl"

if [[ $os == "Darwin" ]]; then
  mkdir -p $install_path_relative
  install_path=$(realpath "$install_path_relative")
else
  install_path=$(readlink -f "$install_path_relative")
fi

../configure -q --disable-shared --disable-dependency-tracking

if [[ $os == "Darwin" ]]; then
  gmake -j $cores > gmake.log 2>&1
  gmake install DESTDIR=$install_path prefix= > install.log 2>&1
else
  make -j $cores > make.log 2>&1
  make install DESTDIR=$install_path prefix= > install.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error building gsl
  exit 1
fi

cd ..
rm -rf tmp

exit 0
