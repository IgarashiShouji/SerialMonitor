#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
[ ! -d OpenXLSX       ] && git clone https://github.com/troldal/OpenXLSX.git
[ ! -d OpenXLSX/build ] && mkdir -p OpenXLSX/build
cd OpenXLSX/build/
case $SYSTEM in
  "MXE")
    echo "MXE Build System"
    cmake -DCMAKE_TOOLCHAIN_FILE=../../mingw.cmake --target OpenXLSX --config Reselese ..
    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1))
    ;;
  "MINGW")
    echo "MINGW System"
    cmake .. -G 'MSYS Makefiles'
    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1))
    ;;
  *)
    echo 'Linux System'
    # cmake --target OpenXLSX --config Reselese ..
    cmake ..
    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1))
    ;;
esac
