#ifndef APP_H
#define APP_H

#include <map>

#include <SDL.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"

class App {
	public:
		App(int screenWidth, int screenHeight);
		virtual ~App() { }
		void run();
		virtual const char* getVertexShaderFilename() = 0;
		virtual const char* getFragmentShaderFilename() = 0;
		virtual void bindAttributes() = 0;
		virtual void draw() = 0;
		virtual void postInit() { }
		virtual bool handleEvent(const SDL_Event& ev);

	protected:

		GLuint mProgramObject;
		std::map<const char*, GLint> mUniformLocationMap;

	private:
		void init();
		bool handleInput();

		SDL_Surface* mScreen;
		bool mInit;
		int mScreenWidth;
		int mScreenHeight;
};

#endif

