CXX      = clang++
CXXFLAGS = -std=c++11 -Wall -Werror $(shell sdl-config --cflags) -O2
LDFLAGS  = $(shell sdl-config --libs) -lSDL_image -lSDL_ttf -lGL -lGLEW -lassimp
AR       = ar

default: triangle cube SceneCube


COMMONDIR = libcommon
COMMONSRCS = $(shell (find $(COMMONDIR) \( -name '*.cpp' -o -name '*.h' \)))

COMMONLIB = $(COMMONDIR)/libcommon.a

$(COMMONLIB): $(COMMONSRCS)
	make -C $(COMMONDIR)


GLCOMMONSRCS = Model.cpp App.cpp HelperFunctions.cpp
GLCOMMONOBJS = $(GLCOMMONSRCS:.cpp=.o)
GLCOMMONLIB = libglcommon.a

$(GLCOMMONLIB): $(GLCOMMONOBJS)
	$(AR) rcs $(GLCOMMONLIB) $(GLCOMMONOBJS)

LIBSCENESRCS = Scene.cpp
LIBSCENEOBJS = $(LIBSCENESRCS:.cpp=.o)
LIBSCENELIB = libscene.a

$(LIBSCENELIB): $(LIBSCENEOBJS)
	$(AR) rcs $(LIBSCENELIB) $(LIBSCENEOBJS)



triangle: $(COMMONLIB) $(GLCOMMONLIB) triangle.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o triangle triangle.cpp $(GLCOMMONLIB) $(COMMONLIB)

cube: $(COMMONLIB) $(GLCOMMONLIB) cube.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o cube cube.cpp $(GLCOMMONLIB) $(COMMONLIB)

SceneCube: $(COMMONLIB) $(GLCOMMONLIB) $(LIBSCENELIB) SceneCube.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o SceneCube SceneCube.cpp $(LIBSCENELIB) $(GLCOMMONLIB) $(COMMONLIB)

clean:
	rm -rf cube
	rm -rf triangle
	rm -rf SceneCube
	rm -rf libcommon/*.a
	rm -rf libcommon/*.o
	rm -rf *.o
	rm -rf $(GLCOMMONLIB)

