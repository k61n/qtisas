#!/bin/bash

os=$1
arch=$2
cores=$3
cores=${cores:-1}
name=$4
libdir=$5
CC=$6
CXX=$7
cmake_version=$8
QT=$9
prefer_qt=${10}

cd $libdir
file="../../libs/$os-$arch/$name/lib/lib$name.a"

if [ -f "$file" ]; then
  echo "Library ${name} is already built: $file"
  exit 0
fi

echo Building $name library

rm -rf tmp
mkdir tmp
cd tmp

install_path="../../../libs/$os-$arch/$name"
case $name in
  "muparser")
    cmake .. -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DENABLE_SAMPLES=OFF -DENABLE_OPENMP=OFF -DCMAKE_INSTALL_PREFIX=$install_path -DCMAKE_INSTALL_LIBDIR=lib > configure.log 2>&1
    ;;
  "qtexengine"|"qwt"|"qwtplot3d")
    cmake .. -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_PREFIX_PATH=$QT -DPREFER_QT=$prefer_qt -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path -DCMAKE_INSTALL_LIBDIR=lib > configure.log 2>&1
    ;;
  "tamuanova")
    gsl="../../libs/$os-$arch/gsl"
    cmake .. -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DGSL_ROOT=$gsl -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path -DCMAKE_INSTALL_LIBDIR=lib > configure.log 2>&1
    ;;
  *)
    cmake .. -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path -DCMAKE_INSTALL_LIBDIR=lib > configure.log 2>&1
    ;;
esac

# `--install` available in cmake>=3.15
if [[ $(echo "$cmake_version 3.15" | awk '{if ($1 >= $2) print 1; else print 0}') -eq 1 ]]; then
  cmake --build . --parallel $cores > build.log 2>&1
  cmake --install . > install.log 2>&1
else
  # `--parallel` available in cmake>=3.12, but for simple condition will not be used for <3.15
  cmake --build . -- -j$cores > build.log 2>&1
  make install > install.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error building $name
  exit 1
fi

cd ..
rm -rf tmp

exit 0
