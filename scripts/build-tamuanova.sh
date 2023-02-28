#!/bin/bash

echo Building tamu-anova library

file="../libs/$1-$2/libtamuanova.a"

if [ -f "$file" ]; then
	echo "Tamu-anova library is already built: $file"
  exit 1
fi

cd $4
mkdir build
rsync -av --exclude=build . build/ > /dev/null 2>&1
cd build/

./configure -q --disable-shared CFLAGS=-I"../../gsl/include/"

if [[ $1 == "Darwin" ]]; then
  gmake -s -j $3 > gmake.log 2>&1
else
  make -s -j $3 > make.log 2>&1
fi

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libtamuanova.a ../../../libs/$1-$2

cd ..
rm -rf build

exit 0
