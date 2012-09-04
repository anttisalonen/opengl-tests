#include "App.h"

#include <iostream>
#include <fstream>

#include <SDL_image.h>

#include "libcommon/Math.h"
#include "libcommon/Texture.h"

#include "HelperFunctions.h"

using namespace Common;

App::App(int screenWidth, int screenHeight)
	: mInit(false),
	mScreenWidth(screenWidth),
	mScreenHeight(screenHeight)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		std::cerr << "Unable to init SDL: " << SDL_GetError() << "\n";
		exit(1);
	}
	mScreen = SDL_SetVideoMode(mScreenWidth, mScreenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL);
	if(!mScreen) {
		std::cerr << "Unable to set video mode\n";
		exit(1);
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(IMG_Init(IMG_INIT_PNG) == -1) {
		std::cerr << "Unable to init SDL_image: " << IMG_GetError() << "\n";
		exit(1);
	}
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_WM_SetCaption("OpenGL tests", nullptr);
}


void App::init()
{
	GLuint vshader;
	GLuint fshader;
	GLint linked;

	GLenum glewerr = glewInit();
	if (glewerr != GLEW_OK) {
		std::cerr << "Unable to initialise GLEW.\n";
		exit(1);
	}
	if (!GLEW_VERSION_2_1) {
		std::cerr << "OpenGL 2.1 not supported.\n";
		exit(1);
	}

	vshader = HelperFunctions::loadShaderFromFile(GL_VERTEX_SHADER, getVertexShaderFilename());
	fshader = HelperFunctions::loadShaderFromFile(GL_FRAGMENT_SHADER, getFragmentShaderFilename());

	mProgramObject = glCreateProgram();

	if(mProgramObject == 0) {
		std::cerr << "Unable to create program.\n";
		exit(1);
	}

	glAttachShader(mProgramObject, vshader);
	glAttachShader(mProgramObject, fshader);

	bindAttributes();
	glLinkProgram(mProgramObject);

	glGetProgramiv(mProgramObject, GL_LINK_STATUS, &linked);

	if(!linked) {
		GLint infoLen = 0;
		glGetProgramiv(mProgramObject, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetProgramInfoLog(mProgramObject, infoLen, NULL, infoLog);
			std::cerr << "Error linking program: " << infoLog << "\n";
			delete[] infoLog;
		} else {
			std::cerr << "Unknown error when linking program.\n";
		}

		glDeleteProgram(mProgramObject);
		exit(1);
	}

	postInit();

	for(auto& p : mUniformLocationMap) {
		p.second = glGetUniformLocation(mProgramObject, p.first);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, mScreenWidth, mScreenHeight);

	glUseProgram(mProgramObject);
}

void App::run()
{
	if(!mInit) {
		mInit = true;
		init();
	}

	while(1) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(handleInput()) {
			break;
		}
		draw();
		SDL_GL_SwapBuffers();
	}

}

bool App::handleEvent(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			switch(ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					return true;

				default:
					break;
			}
			break;

		case SDL_QUIT:
			return true;

		default:
			break;
	}

	return false;
}

bool App::handleInput()
{
	SDL_Event event;

	while(SDL_PollEvent(&event)) {
		if(handleEvent(event)) {
			return true;
		}
	}

	return false;
}


