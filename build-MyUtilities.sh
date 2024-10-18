#! /bin/bash

source check-system.sh
[ ! -d MyUtilities ] && git clone https://github.com/IgarashiShouji/MyUtilities.git
cd MyUtilities
case $SYSTEM in
  "MXE")
    echo "MXE Build System"
    CROSSDEV=x86_64-w64-mingw32.static make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) -f Makefile lib
    ;;
#  "MINGW")
#    echo "MINGW System"
#    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) -f Makefile lib
#    ;;
  *)
    echo 'Linux System'
    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) -f Makefile lib
    ;;
esac
