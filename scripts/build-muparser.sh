#!/bin/bash

echo Building muparser library

file="../lib/$1-$2/libmuparser.a"

if [ -f "$file" ]; then
	echo "Muparser library is already built: $file"
  exit 1
fi

cd $3
mkdir build
cd build

if [[ $1 == "Darwin" ]]; then
  clang++ -O3 -c -w -I ../include/ ../src/*.cpp
else
  g++ -O3 -c -w -I ../include/ ../src/*.cpp
fi

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

ar -rc libmuparser.a *.o

if [ $? -ne 0 ]; then
  echo Error while creating static library
  exit 1
fi

cp libmuparser.a ../../../lib/$1-$2/

cd ..
rm -rf build

exit 0
