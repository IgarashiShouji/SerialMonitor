TARGET=smon
ifdef MSYSTEM
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -pipe -O3
LIBS=-static -L ./Objects -L ./MyUtilities -lSerialMonitor -lUtilities -lboost_program_options-mt -lboost_system-mt -lboost_thread-mt -lws2_32
else
CFLAGS=-g -I ./Include -I ./MyUtilities/Include -pipe -O3 -march=native
LIBS=-L ./Objects -L ./MyUtilities -lSerialMonitor -lUtilities -lboost_program_options -lboost_system -lboost_thread -lpthread
endif
CPPFLAGS=-std=c++14 -DBOOST_THREAD_USE_LIB $(CFLAGS)

all: Doxygen/html/index.html $(TARGET)

clean:
	rm -rf $(TARGET) Objects/*.[ao]

$(TARGET): Objects/main.o MyUtilities/libUtilities.a Objects/libSerialMonitor.a Objects
	g++ $(CPPFLAGS) -o $@ $< $(LIBS)

Objects:
	mkdir -p Objects

MyUtilities:
	git clone https://github.com/IgarashiShouji/MyUtilities.git

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
Objects/main.o: Source/main.cpp Include/CyclicTimer.hpp Include/SerialControl.hpp MyUtilities/Include/Entity.hpp Objects
Objects/CyclicTimer.o: Source/CyclicTimer.cpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp Objects
Objects/SerialControl.o: Source/SerialControl.cpp Include/SerialControl.hpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp Objects
Objects/RTSControl-linux.o: Source/RTSControl-linux.cpp Include/SerialControl.hpp Include/CyclicTimer.hpp MyUtilities/Include/Entity.hpp Objects
