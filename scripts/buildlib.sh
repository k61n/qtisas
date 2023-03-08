#!/bin/bash

os=$1
arch=$2
cores=$3
name=$4

cd $5
file="../../libs/$os-$arch/$name/lib/lib$name.a"

if [ -f "$file" ]; then
  echo "${name^} is already built: $file"
  exit 0
fi

echo Building $name library

rm -rf tmp
mkdir tmp
cd tmp

install_path="../../../libs/$os-$arch/$name"
if [ "$name" = "tiff" ]; then
  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -Djpeg=OFF -Dlzma=OFF -DCMAKE_INSTALL_PREFIX=$install_path > configure.log 2>&1
else
  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$install_path > configure.log 2>&1
fi
cmake --build . --parallel $cores > build.log 2>&1
cmake --install . > install.log 2>&1

if [ $? -ne 0 ]; then
  echo Error building $name
  exit 1
fi

cd ..
rm -rf tmp

exit 0
