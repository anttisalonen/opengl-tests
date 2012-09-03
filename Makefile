CXX      = clang++
CXXFLAGS = -std=c++11 -Wall -Werror $(shell sdl-config --cflags) -O2
LDFLAGS  = $(shell sdl-config --libs) -lSDL_image -lGL -lGLEW -lassimp
AR       = ar

default: triangle cube


COMMONDIR = libcommon
COMMONSRCS = $(shell (find $(COMMONDIR) \( -name '*.cpp' -o -name '*.h' \)))

COMMONLIB = $(COMMONDIR)/libcommon.a

$(COMMONLIB): $(COMMONSRCS)
	make -C $(COMMONDIR)


GLCOMMONSRCS = Model.cpp App.cpp
GLCOMMONOBJS = $(GLCOMMONSRCS:.cpp=.o)
GLCOMMONLIB = libglcommon.a

$(GLCOMMONLIB): $(GLCOMMONOBJS)
	$(AR) rcs $(GLCOMMONLIB) $(GLCOMMONOBJS)



triangle: $(COMMONLIB) $(GLCOMMONLIB) triangle.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o triangle triangle.cpp $(GLCOMMONLIB) $(COMMONLIB)

cube: $(COMMONLIB) $(GLCOMMONLIB) cube.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o cube cube.cpp $(GLCOMMONLIB) $(COMMONLIB)

clean:
	rm -rf cube
	rm -rf triangle
	rm -rf libcommon/*.a
	rm -rf libcommon/*.o
	rm -rf *.o
	rm -rf $(GLCOMMONLIB)

