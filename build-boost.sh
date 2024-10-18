#! /bin/bash

SYSTEM=`bash --norc ./check-system.sh $1`
if [ ! -d boost ]; then
  git clone https://github.com/boostorg/boost.git
  cd boost
# git checkout boost-1.82.0 -b boost-1.82.0
# git submodule update --init tools/* libs/system libs/program_options libs/thread libs/date_time libs/serialization libs/filesystem libs/asio libs/winapi libs/type*
# git submodule update --init tools/* libs/accumulators libs/algorithm libs/align libs/any libs/array libs/asio libs/assert libs/assign libs/atomic libs/beast libs/bimap libs/bind libs/callable_traits libs/charconv libs/chrono libs/circular_buffer libs/cobalt libs/compat libs/compatibility libs/compute libs/concept_check libs/config libs/container libs/container_hash libs/context libs/contract libs/conversion libs/convert libs/core libs/coroutine libs/coroutine2 libs/crc libs/date_time libs/describe libs/detail libs/dll libs/dynamic_bitset libs/endian libs/exception libs/fiber libs/filesystem libs/flyweight libs/foreach libs/format libs/function libs/function_types libs/functional libs/fusion libs/geometry libs/gil libs/graph libs/graph_parallel libs/hana libs/headers libs/heap libs/histogram libs/hof libs/icl libs/integer libs/interprocess libs/intrusive libs/io libs/iostreams libs/iterator libs/json libs/lambda libs/lambda2 libs/leaf libs/lexical_cast libs/local_function libs/locale libs/lockfree libs/log libs/logic libs/math libs/metaparse libs/move libs/mp11 libs/mpi libs/mpl libs/msm libs/multi_array libs/multi_index libs/multiprecision libs/mysql libs/nowide libs/numeric libs/optional libs/outcome libs/parameter libs/parameter_python libs/pfr libs/phoenix libs/poly_collection libs/polygon libs/pool libs/predef libs/preprocessor libs/process libs/program_options libs/property_map libs/property_map_parallel libs/property_tree libs/proto libs/ptr_container libs/python libs/qvm libs/random libs/range libs/ratio libs/rational libs/redis libs/regex libs/safe_numerics libs/scope libs/scope_exit libs/serialization libs/signals2 libs/smart_ptr libs/sort libs/spirit libs/stacktrace libs/statechart libs/static_assert libs/static_string libs/stl_interfaces libs/system libs/test libs/thread libs/throw_exception libs/timer libs/tokenizer libs/tti libs/tuple libs/type_erasure libs/type_index libs/type_traits libs/typeof libs/units libs/unordered libs/url libs/utility libs/uuid libs/variant libs/variant2 libs/vmd libs/wave libs/winapi libs/xpressive libs/yap
  git submodule update --init tools/* libs/*
  PWD_BASE=`pwd`
  for path in `find . -name '.git'`
  do
    cd `echo $path | sed -e 's/.git//'`
    git checkout boost-1.82.0 -b boost-1.82.0
    cd $PWD_BASE
  done
  pwd
else
  cd boost
fi
case $SYSTEM in
  "MXE")
    echo "MXE Build System"
    echo "using gcc :  : x86_64-w64-mingw32.static-g++;" > user-config.jam
#    [ ! -e b2 ] && ./bootstrap.sh
#    ./b2 -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) --user-config=./user-config.jam --prefix=../boost-x64 target-os=windows address-model=64 link=static variant=release install --with-program_options --with-system --with-thread --with-date_time --with-serialization --with-filesystem
    ;;
  "MINGW")
    echo "MINGW System"
#    [ ! -e b2.exe ] && ./bootstrap.sh mingw
#    ./b2.exe -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) --prefix=../boost-x64 target-os=windows address-model=64 link=static variant=release install --with-program_options --with-system --with-thread --with-date_time --with-serialization --with-filesystem
    ;;
  *)
    echo 'Linux System'
    [ ! -e b2 ] && ./bootstrap.sh
    ./b2 -j$(($(grep processor /proc/cpuinfo | wc -l) + 1)) --prefix=../boost-x64 target-os=linux address-model=64 link=static variant=release install --with-program_options --with-system --with-thread --with-date_time --with-serialization --with-filesystem
    ;;
esac
