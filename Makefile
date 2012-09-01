CXX      = clang++
CXXFLAGS = -std=c++11 -Wall -Werror $(shell sdl-config --cflags) -O2
LDFLAGS  = $(shell sdl-config --libs) -lSDL_image -lGL -lGLEW

default: triangle

triangle: triangle.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o triangle triangle.cpp

