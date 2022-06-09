TARGET=smon
CPP=g++
AR=ar

CFLAGS_COMMON=-I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./OpenXLSX/OpenXLSX -I ./OpenXLSX/OpenXLSX/headers -I ./OpenXLSX/build/OpenXLSX -I ./OpenXLSX/Examples/external/nowide/include
LIBS_COMMON=-L ./Objects -L ./MyUtilities -L OpenXLSX/build/output/ -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lOpenXLSX
# on Linux
ifdef CROSSDEV
# x86_64-w64-mingw32
#CFLAGS=-g -I /usr/x86_64-w64-mingw32/include $(CFLAGS_COMMON) -pipe -O3 -march=native
#LIBS=-static -L ./Objects -L ./MyUtilities -L ./mruby/build/cross-mingw-winetest/lib -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lws2_32 -L OpenXLSX/build-mingw/output/ -lOpenXLSX
CFLAGS=-g -I /usr/x86_64-w64-mingw32/include $(CFLAGS_COMMON) -I ./mruby/build/cross-mingw-winetest/include -pipe -O3 -march=native
LIBS=-static -L ./mruby/build/cross-mingw-winetest/lib $(LIBS_COMMON) -lws2_32
AR_OBJS=Objects/SerialControl.o Objects/RTSControl-mingw.o
MAIN_DEPS=MyUtilities/libUtilities.a mruby/build/cross-mingw-winetest/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a Objects/libSerialMonitor.a Objects
CPP=x86_64-w64-mingw32-g++
AR=x86_64-w64-mingw32-ar
else
CFLAGS=-g $(CFLAGS_COMMON) -I ./mruby/build/host/include -pipe -O3 -march=native
LIBS=-L ./mruby/build/host/lib $(LIBS_COMMON) -lpthread
AR_OBJS=Objects/SerialControl.o Objects/RTSControl-linux.o
MAIN_DEPS=Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a Objects/libSerialMonitor.a Objects
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

OpenXLSX:
	git clone https://github.com/troldal/OpenXLSX.git

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
mruby/include/mruby.h: mruby/build/cross-mingw-winetest/lib/libmruby.a
OpenXLSX/build/output/libOpenXLSX.a: OpenXLSX
	/bin/bash build-OpenXLSX-cross.sh
else
mruby/build/host/lib/libmruby.a: mruby
	/bin/bash ./build-mruby.sh
mruby/include/mruby.h: mruby/build/host/lib/libmruby.a
OpenXLSX/build/output/libOpenXLSX.a: OpenXLSX
	/bin/bash build-OpenXLSX.sh
endif

Objects/libSerialMonitor.a: $(AR_OBJS)
	$(AR) rcs $@ $^

Objects/%.o: Source/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

MyUtilities/Include/Entity.hpp: MyUtilities
Objects/main.o: Source/main.cpp Include/Timer.hpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp mruby/include/mruby.h OpenXLSX/build/output/libOpenXLSX.a
Objects/SerialControl.o: Source/SerialControl.cpp Include/SerialControl.hpp Include/Timer.hpp MyUtilities/Include/Entity.hpp
Objects/RTSControl-linux.o: Source/RTSControl-linux.cpp Include/SerialControl.hpp Include/Timer.hpp MyUtilities/Include/Entity.hpp
