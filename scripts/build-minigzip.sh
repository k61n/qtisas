#!/bin/bash

echo Building minigzip library

cd $4
file="../../libs/$1-$2/libminigzip.a"

if [ -f "$file" ]; then
	echo "Minigzip library is already built: $file"
  exit 0
fi

rm -rf build
mkdir build
cd build/

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libminigzip.a ../../../libs/$1-$2

cd ..
rm -rf build

exit 0
