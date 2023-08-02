#! /bin/bash

[ ! -d lz4 ] && git clone https://github.com/lz4/lz4.git
cd lz4
make liblz4.a
