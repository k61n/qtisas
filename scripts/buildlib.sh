#!/bin/bash

os=$1
arch=$2
cores=$3
cores=${cores:-1}
name=$4
libdir=$5
GEN=$6
MK=$7
CC=$8
CXX=$9
cmake_version=${10}
gsl=${11}
QT=${12}
prefer_qt=${13}

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
  "qtexengine"|"qwt"|"qwtplot3d")
    args="-DCMAKE_PREFIX_PATH=$QT -DPREFER_QT=$prefer_qt"
    ;;
  "tamuanova")
    args="-DGSL_ROOT=$gsl"
    ;;
esac
cmake .. -G$GEN -DCMAKE_MAKE_PROGRAM=$MK -DCMAKE_C_COMPILER=$CC -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path -DCMAKE_INSTALL_LIBDIR=lib $args > configure.log 2>&1

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
