#! /bin/bash

[ ! -d OpenXLSX/build ] && mkdir -p OpenXLSX/build
cd OpenXLSX/build/
cmake -DCMAKE_TOOLCHAIN_FILE=../../mingw.cmake --target OpenXLSX --config Reselese ..
make
