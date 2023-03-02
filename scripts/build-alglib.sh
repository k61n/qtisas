#!/bin/bash

echo Building alglib library

cd $4
file="../../libs/$1-$2/libalglib.a"

if [ -f "$file" ]; then
  echo "Alglib library is already built: $file"
  exit 0
fi

rm -rf build
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while creating static library
  exit 1
fi

if [ ! -d "../../../libs/$1-$2" ]; then
  mkdir -p ../../../libs/$1-$2
fi

cp libalglib.a ../../../libs/$1-$2/

cd ..
rm -rf build

exit 0
