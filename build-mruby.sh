#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
if [ ! -d mruby ]; then
  git clone https://github.com/mruby/mruby.git
  cd mruby
# git checkout 3.2.0
  git checkout 3.3.0
else
  cd mruby
fi
case $SYSTEM in
  "MXE")
    echo "MXE Build System"
    export MRUBY_CONFIG=../cross-mingw-winetest.rb
    ruby minirake -m
    ;;
  "MINGW")
    echo "MINGW System"
#   export MRUBY_CONFIG=../mingw.rb
#   ruby minirake clean
    export MRUBY_CONFIG=../default.rb
    ruby minirake -m
    ;;
  *)
    echo 'Linux System'
    export MRUBY_CONFIG=../default.rb
    ruby minirake clean
    ruby minirake -m
    ;;
esac
