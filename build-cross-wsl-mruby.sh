#! /bin/bash

[ ! -d mruby ] && git clone https://github.com/mruby/mruby.git
cd mruby
git checkout 3.2.0
export MRUBY_CONFIG=../cross-mingw-winetest.rb
ruby minirake -m
