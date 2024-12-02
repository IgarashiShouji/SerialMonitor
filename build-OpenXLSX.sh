#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
if [ ! -d OpenXLSX ]; then
  git clone https://github.com/troldal/OpenXLSX.git
  cd OpenXLSX
  #git checkout 1f58d038aac9319026c0fd5b4efe4af2cd4b45ba -b test

  git checkout b80da42d1454f361c29117095ebe1989437db390 -b test
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
