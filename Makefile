TARGET=smon
ifdef MSYSTEM
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -pipe -O3
LIBS=-static -L ./Objects -L ./MyUtilities -lSerialMonitor -lUtilities -lboost_program_options-mt -lboost_system-mt -lboost_thread-mt -lws2_32
else
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -I ./mruby/include -I ./mruby/build/host/include -pipe -O3 -march=native
LIBS=-L ./Objects -L ./MyUtilities -L ./mruby/build/host/lib -lSerialMonitor -lUtilities -lmruby -lboost_program_options -lboost_system -lboost_thread -lpthread
endif
CPPFLAGS=-std=c++14 -DBOOST_THREAD_USE_LIB $(CFLAGS)

all: $(Objects) $(TARGET)

clean:
	rm -rf $(TARGET) Objects/*.[ao]

$(TARGET): Objects/main.o MyUtilities/libUtilities.a mruby/build/host/lib/libmruby.a Objects/libSerialMonitor.a Objects
	g++ $(CPPFLAGS) -o $@ $< $(LIBS)

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
							Source/main.cpp Include/CyclicTimer.hpp Include/SerialControl.hpp \
							Source/CyclicTimer.cpp Include/CyclicTimer.hpp \
							Source/SerialControl.cpp Include/SerialControl.hpp \
							Source/RTSControl-mingw.cpp Include/SerialControl.hpp Include/CyclicTimer.hpp 
	doxygen Doxyfile

MyUtilities/libUtilities.a: MyUtilities
	cd MyUtilities; make -f Makefile all

mruby/build/host/lib/libmruby.a: mruby
	cd mruby; ruby minirake

ifdef MSYSTEM
Objects/libSerialMonitor.a: Objects/CyclicTimer.o Objects/SerialControl.o Objects/RTSControl-mingw.o
	ar rcs $@ $^
else
Objects/libSerialMonitor.a: Objects/CyclicTimer.o Objects/SerialControl.o Objects/RTSControl-linux.o
	ar rcs $@ $^
endif

Objects/%.o: Source/%.cpp
	g++ $(CPPFLAGS) -c -o $@ $<

MyUtilities/Include/Entity.hpp: MyUtilities
Objects/main.o: Source/main.cpp Include/CyclicTimer.hpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp
Objects/CyclicTimer.o: Source/CyclicTimer.cpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp
Objects/SerialControl.o: Source/SerialControl.cpp Include/SerialControl.hpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp
Objects/RTSControl-linux.o: Source/RTSControl-linux.cpp Include/SerialControl.hpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp
