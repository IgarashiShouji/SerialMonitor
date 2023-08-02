#! /bin/bash

[ ! -d MyUtilities ] && git clone https://github.com/IgarashiShouji/MyUtilities.git
cd MyUtilities
make -f Makefile lib
