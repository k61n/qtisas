#!/bin/bash

echo Building gls library

file1="../lib/$1-$2/libgsl.a"
file2="../lib/$1-$2/libgslcblas.a"

if [ -f "$file1" ] && [ -f "$file2" ]; then
  echo "Gls libraries are already built: $file1 and $file2"
  exit 1
fi

cd $4

mkdir build
cd build

../configure -q --disable-shared --disable-dependency-tracking

if [[ $1 == "Darwin" ]]; then
  gmake -s -j $3 > gmake.log 2>&1
else
  make -s -j $3 > make.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp .libs/libgsl.a ../../../lib/$1-$2
cp cblas/.libs/libgslcblas.a ../../../lib/$1-$2

cd ..
rm -rf build

exit 0
