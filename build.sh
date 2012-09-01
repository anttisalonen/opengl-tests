#!/bin/bash

set -e
set -u
set -v

CXXFLAGS="-std=c++11 -Wall -Werror `sdl-config --cflags` -O2"
LDFLAGS="`sdl-config --libs` -lSDL_image -lGL -lGLEW"
g++ $CXXFLAGS $LDFLAGS -o triangle triangle.cpp

