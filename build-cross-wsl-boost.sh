#! /bin/bash

#[ ! -d boost ] && git clone --recursive https://github.com/boostorg/boost.git
#[ ! -d boost ] && git clone https://github.com/boostorg/boost.git
if [ ! -d boost ]; then
  git clone https://github.com/boostorg/boost.git
  cd boost
  git checkout boost-1.82.0 -b boost-1.82.0
  #git submodule update --init tools/* libs/program_options libs/system libs/thread libs/date_time libs/serialization
  git submodule update --init tools/* libs/*
else
  cd boost
fi
echo "using gcc :  : x86_64-w64-mingw32-g++ ;" > user-config.jam
./bootstrap.sh
./b2 -j16 --user-config=./user-config.jam --prefix=../boost-x64 target-os=windows address-model=64 link=static variant=release install --with-program_options --with-system --with-thread --with-date_time --with-serialization
