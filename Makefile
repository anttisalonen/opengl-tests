CXX      = clang++
CXXFLAGS = -std=c++11 -Wall -Werror $(shell sdl-config --cflags) -O2
LDFLAGS  = $(shell sdl-config --libs) -lSDL_image -lGL -lGLEW -lassimp

default: triangle cube

COMMONDIR = libcommon
COMMONSRCS = $(shell (find $(COMMONDIR) \( -name '*.cpp' -o -name '*.h' \)))

COMMONLIB = $(COMMONDIR)/libcommon.a

$(COMMONLIB): $(COMMONSRCS)
	make -C $(COMMONDIR)

triangle: triangle.cpp $(COMMONLIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o triangle triangle.cpp $(COMMONLIB)

CUBESRCFILES = cube.cpp Model.cpp App.cpp

cube: $(CUBESRCFILES) $(COMMONLIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o cube $(CUBESRCFILES) $(COMMONLIB)

clean:
	rm -rf cube
	rm -rf triangle
	rm -rf libcommon/*.a
	rm -rf libcommon/*.o
	rm -rf *.o

