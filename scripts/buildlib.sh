#!/bin/bash

os=$1
arch=$2
cores=$3
cores=${cores:-1}
name=$4

cd $5
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
    git checkout v2.3.4 &> /dev/null
    cmake .. -DENABLE_SAMPLES=OFF -DENABLE_OPENMP=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path > configure.log 2>&1
    ;;
  "yaml-cpp")
    git checkout release-0.3.0 &> /dev/null
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path > configure.log 2>&1
    ;;
  *)
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path > configure.log 2>&1
    ;;
esac
cmake --build . --parallel $cores > build.log 2>&1
cmake --install . > install.log 2>&1
case $name in
  "muparser"|"yaml-cpp")
    git checkout origin/master &> /dev/null
    git clean -f -d &> /dev/null
    ;;
  *)
    ;;
esac

if [ $? -ne 0 ]; then
  echo Error building $name
  exit 1
fi

cd ..
rm -rf tmp

exit 0
