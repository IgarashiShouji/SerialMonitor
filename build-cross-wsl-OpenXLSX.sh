#! /bin/bash

[ ! -d OpenXLSX       ] && git clone https://github.com/troldal/OpenXLSX.git
[ ! -d OpenXLSX/build ] && mkdir -p OpenXLSX/build
cd OpenXLSX/build/
cmake -DCMAKE_TOOLCHAIN_FILE=../../mingw.cmake --target OpenXLSX --config Reselese ..
make
