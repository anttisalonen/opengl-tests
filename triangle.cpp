#include <unistd.h>
#include <fstream>
#include <iostream>

#include <SDL.h>
#include <SDL_image.h>
#include <GL/glew.h>
#include <GL/gl.h>

static const int screenWidth = 800;
static const int screenHeight = 600;

class App {
	public:
		App();
		void run();

	private:
		GLuint loadShader(GLenum type, const char* src);
		GLuint loadShaderFromFile(GLenum type, const char* filename);

		SDL_Surface* mScreen;
		GLuint mProgramObject;
};

GLuint App::loadShaderFromFile(GLenum type, const char* filename)
{
	std::ifstream ifs(filename);
	if(ifs.bad()) {
		return 0;
	}
	std::string content((std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
	return loadShader(type, content.c_str());
}

GLuint App::loadShader(GLenum type, const char* src)
{
	GLuint shader;
	GLint compiled;

	shader = glCreateShader(type);

	if(shader == 0)
		return 0;

	glShaderSource(shader, 1, &src, NULL);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if(infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			std::cerr << "Error compiling shader: " << infoLog << "\n";
			delete[] infoLog;
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

App::App()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		std::cerr << "Unable to init SDL: " << SDL_GetError() << "\n";
		exit(1);
	}
	mScreen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL);
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

	vshader = loadShaderFromFile(GL_VERTEX_SHADER, "simple.vert");
	fshader = loadShaderFromFile(GL_FRAGMENT_SHADER, "simple.frag");

	mProgramObject = glCreateProgram();

	if(mProgramObject == 0) {
		std::cerr << "Unable to create program.\n";
		exit(1);
	}

	glAttachShader(mProgramObject, vshader);
	glAttachShader(mProgramObject, fshader);

	glBindAttribLocation(mProgramObject, 0, "vPosition");

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
}

void App::run()
{
	GLfloat vertices[] = {0.0f, 0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f};

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, screenWidth, screenHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(mProgramObject);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	SDL_GL_SwapBuffers();

	sleep(4);
}

int main(void)
{
	try {
		App app;
		app.run();
	} catch(std::exception& e) {
		std::cerr << "std::exception: " << e.what() << "\n";
	} catch(...) {
		std::cerr << "Unknown exception.\n";
	}
	return 0;
}

