CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lSDL2 -pthread
OBJS = \
    src/bootrom.o \
    src/bus.o \
    src/cartridge.o \
    src/frontend/sdl.o \
    src/gb.o \
    src/joypad.o \
    src/main.o \
    src/ppu.o \
    src/sm83.o \
    src/timer.o

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

heliage: $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -r src/*.o heliage
