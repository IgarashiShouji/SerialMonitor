#! /bin/bash

[ ! -d lz4 ] && git clone https://github.com/lz4/lz4.git
cd lz4
CC=x86_64-w64-mingw32-gcc-posix AR=x86_64-w64-mingw32-gcc-ar-posix make liblz4.a
