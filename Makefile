TARGET=smon
CPP=g++
AR=ar

ifdef MSYSTEM
# mingw on msys2
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./mruby/build/host/include -pipe -O3
LIBS=-L ./Objects -L ./MyUtilities -L ./mruby/build/host/lib -lSerialMonitor -lUtilities -lmruby -lboost_program_options-mt -lboost_system-mt -lws2_32 -lregex
AR_OBJS=Objects/SerialControl.o Objects/RTSControl-mingw.o
MAIN_DEPS=Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a Objects/libSerialMonitor.a Objects
else
# on Linux
ifdef CROSSDEV
# x86_64-w64-mingw32
CFLAGS=-g -I /usr/x86_64-w64-mingw32/include -I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./mruby/build/cross-mingw-winetest/include -pipe -O3 -march=native
LIBS=-static -L ./Objects -L ./MyUtilities -L ./mruby/build/cross-mingw-winetest/lib -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lws2_32
AR_OBJS=Objects/SerialControl.o Objects/RTSControl-mingw.o
MAIN_DEPS=MyUtilities/libUtilities.a mruby/build/cross-mingw-winetest/lib/libmruby.a mruby Objects/libSerialMonitor.a Objects
CPP=x86_64-w64-mingw32-g++
AR=x86_64-w64-mingw32-ar
else
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./mruby/build/host/include -pipe -O3 -march=native
LIBS=-L ./Objects -L ./MyUtilities -L ./mruby/build/host/lib -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lpthread
AR_OBJS=Objects/SerialControl.o Objects/RTSControl-linux.o
MAIN_DEPS=Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a Objects/libSerialMonitor.a Objects
endif
endif
CPPFLAGS=-std=c++17 $(CFLAGS)

all: $(Objects) $(TARGET)

clean:
	rm -rf $(TARGET) Objects/*.[ao]

$(TARGET): Objects/main.o $(MAIN_DEPS)
	$(CPP) $(CPPFLAGS) $< -o $@ $(LIBS)

Objects:
	mkdir -p Objects

MyUtilities:
	git clone https://github.com/IgarashiShouji/MyUtilities.git

mruby:
	git clone https://github.com/mruby/mruby.git

document: Doxygen/html/index.html

Doxygen:
	mkdir -p Doxygen

Doxygen/html/index.html: Doxyfile Doxygen \
							Source/main.cpp Include/Timer.hpp Include/SerialControl.hpp \
							Source/SerialControl.cpp Include/SerialControl.hpp \
							Source/RTSControl-mingw.cpp Include/SerialControl.hpp Include/Timer.hpp 
	doxygen Doxyfile

MyUtilities/libUtilities.a: MyUtilities
	cd MyUtilities; make -f Makefile lib

ifdef CROSSDEV
mruby/build/cross-mingw-winetest/lib/libmruby.a: mruby
	/bin/bash ./build-mruby-cross.sh
else
mruby/build/host/lib/libmruby.a: mruby
	/bin/bash ./build-mruby.sh
endif

Objects/libSerialMonitor.a: $(AR_OBJS)
	$(AR) rcs $@ $^

Objects/%.o: Source/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

MyUtilities/Include/Entity.hpp: MyUtilities
mruby/include/mruby.h: mruby
Objects/main.o: Source/main.cpp Include/Timer.hpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp mruby/include/mruby.h
Objects/SerialControl.o: Source/SerialControl.cpp Include/SerialControl.hpp Include/Timer.hpp MyUtilities/Include/Entity.hpp
Objects/RTSControl-linux.o: Source/RTSControl-linux.cpp Include/SerialControl.hpp Include/Timer.hpp MyUtilities/Include/Entity.hpp
