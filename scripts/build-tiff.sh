#!/bin/bash

echo Building tiff library

cd $4
file="../../libs/$1-$2/libtiff.a"

if [ -f "$file" ]; then
	echo "Tiff library is already built: $file"
  exit 0
fi

mkdir tmp
cd tmp/

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libtiff/libtiff.a ../../../libs/$1-$2

cd ..
rm -rf tmp

exit 0
