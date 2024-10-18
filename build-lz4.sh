#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
[ ! -d lz4 ] && git clone https://github.com/lz4/lz4.git
cd lz4
case $SYSTEM in
  "MXE")
    echo "MXE build System"
    CC=x86_64-w64-mingw32.static-gcc AR=x86_64-w64-mingw32.static-ar make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) liblz4.a
    ;;
  "MINGW")
    echo "MINGW System"
    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) liblz4.a
    ;;
  *)
    echo 'Linux System'
#    make -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) liblz4.a
    ;;
esac
