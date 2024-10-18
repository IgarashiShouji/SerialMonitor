#! /bin/bash

SYSTEM="$1"
if [ -z "${SYSTEM}" ]; then
  SYSTEM=`grep WSL /proc/version | sed -e 's/^.*WSL.*$/WSL/g'`
  [ "WSL" != "$SYSTEM" ] && SYSTEM=`grep MINGW /proc/version | sed -e 's/^.*MINGW.*$/MINGW/g'`
fi
echo ${SYSTEM}
