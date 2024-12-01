#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
if [ ! -d OpenXLSX ]; then
  git clone https://github.com/troldal/OpenXLSX.git
  cd OpenXLSX
  git checkout c5122314b5f936adb5709973d3a47b55f48a2317 -b test
  #git checkout 36640389bacffc9916a7903bd49f2b0165513d73 -b test
  #git checkout fd9fbcdc9b8437ca7e4111e59fc15b7260410417 -b test
  #git checkout d618e1de159cfb4fb3bf97934319fe8b7bed350e -b test
  #git checkout e0b2c22ed3cab511e7a376ef3fd59a6e2ca68d3b -b test
  #git checkout 30ad3fdbf93846260587f3baa4f22c7e0cba5970 -b test
  #git checkout 61b14e3ae45fdc44bb6c08dfafa0553f4341e79a -b test
  cd ..
fi
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
