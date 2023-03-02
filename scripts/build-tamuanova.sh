#!/bin/bash

echo Building tamu-anova library

cd $4
file="../../libs/$1-$2/libtamuanova.a"

if [ -f "$file" ]; then
	echo "Tamu-anova library is already built: $file"
  exit 0
fi

mkdir build
cd build/

cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF > configure.log 2>&1
cmake --build . --parallel $3 > build.log 2>&1

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

cp libtamuanova.a ../../../libs/$1-$2

cd ..
rm -rf build

exit 0
