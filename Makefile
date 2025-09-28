
CXX ?= g++
CXXFLAGS += -I. -Icalf-dsp -fPIC -std=gnu++11
LDFLAGS += -Wl,-no-undefined

all: phaser.so

phaser.so: phaser-lv2.cpp.o calf-dsp/audio_fx.cpp.o calf-dsp/phaser.cpp.o
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -shared -o $@

%.cpp.o: %.cpp
	$(CXX) $^ $(CXXFLAGS) -c -o $@

clean:
	rm -f *.so *.o */*.o
