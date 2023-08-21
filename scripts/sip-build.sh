#!/bin/bash

sipbuild=$1
path=$2
qmake=$3
builddir=$4

cd $path
${sipbuild} --no-compile --qmake $qmake --build-dir ${builddir}
