#! /bin/bash

[ ! -d OpenXLSX/build ] && mkdir -p OpenXLSX/build
cd OpenXLSX/build/
cmake --target OpenXLSX --config Reselese ..
make
