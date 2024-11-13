#! /bin/bash --norc

if [ ! -d QR-Code-generator ]; then
  git clone https://github.com/nayuki/QR-Code-generator.git
  cd QR-Code-generator
  git checkout -b v1.8.0
else
  cd QR-Code-generator
fi
cd cpp
case $SYSTEM in
  "MXE")
    echo "MXE Build System"
    CXX=x86_64-w64-mingw32.static-g++ AR=x86_64-w64-mingw32.static-ar make
    ;;
  *)
    echo 'Linux System'
    make
    ;;
esac
