#!/bin/bash

echo Building qwt library

cd $4
file="../../libs/$1-$2/libqwt.a"

if [ -f "$file" ] ; then
  echo "Qwt library is already built: $file"
  exit 0
fi

rm -rf build
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while building the library
  exit 1
fi

cp libqwt.a ../../../libs/$1-$2

cd ..
rm -rf build

exit 0
