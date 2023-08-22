TARGET=smon
CPP=g++
AR=ar

CFLAGS_COMMON=-I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./OpenXLSX/OpenXLSX -I ./OpenXLSX/OpenXLSX/headers -I ./OpenXLSX/build/OpenXLSX -I ./OpenXLSX/Examples/external/nowide/include
LIBS_COMMON=-L ./Objects -L ./MyUtilities -L OpenXLSX/build/output/ -L ./lz4/lib/ -lSerialMonitor -lUtilities -lmruby -lOpenXLSX -llz4

CFLAGS=-g $(CFLAGS_COMMON) -I ./mruby/build/host/include -pipe -O3 -march=native
LIBS=-L ./mruby/build/host/lib $(LIBS_COMMON) -lboost_program_options -lboost_filesystem -lpthread
AR_OBJS=Objects/SerialControl-linux.o Objects/default_options.o Objects/default_script.o
MAIN_DEPS=Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a Objects/libSerialMonitor.a Objects
CPPFLAGS=-std=c++17 $(CFLAGS)

all: $(Objects) $(TARGET)

clean:
	rm -rf $(TARGET) Objects/*.[ao]

$(TARGET): Objects/main.o $(MAIN_DEPS)
	$(CPP) $(CPPFLAGS) $< -o $@ $(LIBS)

Objects:
	mkdir -p Objects

document: Doxygen/html/index.html

Doxygen:
	mkdir -p Doxygen
Doxygen/html/index.html: Doxygen Doxyfile Source/main.cpp Source/SerialControl-linux.cpp Include/SerialControl.hpp
	doxygen Doxyfile

MyUtilities/libUtilities.a:
	/bin/bash build-MyUtilities.sh
mruby/build/host/lib/libmruby.a:
	/bin/bash build-mruby.sh
OpenXLSX/build/output/libOpenXLSX.a:
	/bin/bash build-OpenXLSX.sh
lz4/lib/liblz4.a:
	/bin/bash build-lz4.sh

Objects/libSerialMonitor.a: $(AR_OBJS)
	$(AR) rcs $@ $^

Objects/%.o: Source/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<
Objects/%.o: Source/%.c
	$(CPP) $(CFLAGS) -c -o $@ $<

Objects/main.o:                Source/main.cpp                Include/SerialControl.hpp MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a lz4/lib/liblz4.a
Objects/SerialControl-linux.o: Source/SerialControl-linux.cpp Include/SerialControl.hpp MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a lz4/lib/liblz4.a
Objects/default_options.o:     Source/default_options.c
Objects/default_script.o:      Source/default_script.c
Source/default_options.c:      Source/default_options.rb mruby/build/host/lib/libmruby.a
	./mruby/bin/mrbc -Bdefault_options $<
Source/default_script.c:      Source/default_script.rb  mruby/build/host/lib/libmruby.a
	./mruby/bin/mrbc -Bdefault_script $<
