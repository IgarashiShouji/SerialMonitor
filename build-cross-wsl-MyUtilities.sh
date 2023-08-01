#! /bin/bash

[ ! -d MyUtilities ] && git clone https://github.com/IgarashiShouji/MyUtilities.git
cd MyUtilities
CROSSDEV=x86_64-w64-mingw32 make -f Makefile lib
