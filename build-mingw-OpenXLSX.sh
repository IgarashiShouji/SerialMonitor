#! /bin/bash

[ ! -d OpenXLSX       ] && git clone https://github.com/troldal/OpenXLSX.git
[ ! -d OpenXLSX/build ] && mkdir -p OpenXLSX/build
cd OpenXLSX/; pwd; git apply ../mingw-OpenXLSX.path
cd build/;    pwd; cmake .. -G 'MSYS Makefiles'
make
