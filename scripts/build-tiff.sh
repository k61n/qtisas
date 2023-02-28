#!/bin/bash

echo Building tiff library

file="../lib/$1-$2/libtiff.a"

if [ -f "$file" ]; then
	echo "Tiff library is already built: $file"
  exit 1
fi

cd $4
mkdir tmp
cd tmp/

../configure -q --disable-shared --disable-dependency-tracking --disable-jpeg --disable-lzma > configure.log 2>&1

if [[ $1 == "Darwin" ]]; then
  gmake -s -j $3 > gmake.log 2>&1
else
  make -s -j $3 > make.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libtiff/.libs/libtiff.a ../../../lib/$1-$2

cd ..
rm -rf tmp

exit 0
