TARGET=smon
CPP=g++
AR=ar

CFLAGS_COMMON=-I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./OpenXLSX/OpenXLSX -I ./OpenXLSX/OpenXLSX/headers -I ./OpenXLSX/build/OpenXLSX -I ./OpenXLSX/Examples/external/nowide/include
LIBS_COMMON=-L ./Objects -L ./MyUtilities -L OpenXLSX/build/output/ -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lOpenXLSX
#LIBS_COMMON=-L ./Objects -L ./MyUtilities -L OpenXLSX/build/output/ -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lOpenXLSX
# on Linux
CFLAGS=-g $(CFLAGS_COMMON) -I ./mruby/build/host/include -pipe -O3 -march=native
LIBS=-L ./mruby/build/host/lib $(LIBS_COMMON) -lpthread
AR_OBJS=Objects/SerialControlLinux.o Objects/default_options.o Objects/default_script.o
MAIN_DEPS=Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a OpenXLSX/build/output/libOpenXLSX.a Objects/libSerialMonitor.a Objects
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
							Source/main.cpp Include/SerialControl.hpp \
							Source/SerialControlLinux.cpp Include/SerialControl.hpp \
							Include/SerialControl.hpp
	doxygen Doxyfile

MyUtilities/libUtilities.a: MyUtilities
	cd MyUtilities; make -f Makefile lib

mruby/build/host/lib/libmruby.a: mruby
	/bin/bash ./build-mruby.sh
mruby/include/mruby.h: mruby/build/host/lib/libmruby.a
OpenXLSX/build/output/libOpenXLSX.a:
	/bin/bash build-OpenXLSX.sh

Objects/libSerialMonitor.a: $(AR_OBJS)
	$(AR) rcs $@ $^

Objects/%.o: Source/%.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<
Objects/%.o: Source/%.c
	$(CPP) $(CFLAGS) -c -o $@ $<

MyUtilities/Include/Entity.hpp: MyUtilities
Objects/main.o: Source/main.cpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp mruby/include/mruby.h OpenXLSX/build/output/libOpenXLSX.a
Objects/SerialControlLinux.o: Source/SerialControlLinux.cpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp
Objects/default_options.o: Source/default_options.c
Objects/default_script.o: Source/default_script.c
Source/default_options.c: Source/default_options.rb mruby/build/host/lib/libmruby.a
	./mruby/bin/mrbc -Bdefault_options $<
Source/default_script.c: Source/default_script.rb   mruby/build/host/lib/libmruby.a
	./mruby/bin/mrbc -Bdefault_script $<
