#!/bin/bash

echo Building alglib library

file="../lib/$1-$2/libalglib.a"

if [ -f "$file" ]; then
  echo "Alglib library is already built: $file"
  exit 1
fi

cd $3

rm -rf build
mkdir build
cp src/* build
cd build

if [[ $1 == "Darwin" ]]; then
  clang++ -O3 -c -w -I. *.cpp
else
  g++ -O3 -c -w -I. *.cpp
fi

if [ $? -ne 0 ]; then
  echo Error while compiling
  exit 1
fi

ar -rc libalglib.a *.o

if [ $? -ne 0 ]; then
  echo Error while creating static library
  exit 1
fi

if [ ! -d "../../../lib/$1-$2" ]; then
  mkdir -p ../../../lib/$1-$2
fi

cp libalglib.a ../../../lib/$1-$2/

cd ..
rm -rf build

exit 0
