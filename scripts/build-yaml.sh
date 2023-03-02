#!/bin/bash

echo Building yaml library

cd $4
file="../../libs/$1-$2/libyaml-cpp.a"

if [ -f "$file" ]; then
  echo "Yaml-cpp library is already built: $file"
  exit 0
fi

mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libyaml-cpp.a ../../../libs/$1-$2

cd ..
rm -rf build

exit 0
