#!/bin/bash

echo Building qtexengine library

file="../lib/$1-$2/libqtexengine.a"

if [ -f "$file" ] ; then
  echo "Qtexengine library already exists: $file"
  exit 1
fi

cd $4

mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while building the library
  exit 1
fi

cp libqtexengine.a ../../../lib/$1-$2

cd ..
rm -rf build

exit 0
