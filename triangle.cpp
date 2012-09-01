#include <unistd.h>
#include <cassert>
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
		App(int mode);
		void run();

	private:
		GLuint loadShader(GLenum type, const char* src);
		GLuint loadShaderFromFile(GLenum type, const char* filename);

		SDL_Surface* mScreen;
		GLuint mProgramObject;
		int mMode;
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

App::App(int mode)
	: mMode(mode)
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

	switch(mMode) {
		case 0:
			vshader = loadShaderFromFile(GL_VERTEX_SHADER, "simple.vert");
			fshader = loadShaderFromFile(GL_FRAGMENT_SHADER, "simple.frag");
			break;

		case 1:
			vshader = loadShaderFromFile(GL_VERTEX_SHADER, "colors.vert");
			fshader = loadShaderFromFile(GL_FRAGMENT_SHADER, "colors.frag");
			break;

		default:
			assert(0);
			break;
	}

	mProgramObject = glCreateProgram();

	if(mProgramObject == 0) {
		std::cerr << "Unable to create program.\n";
		exit(1);
	}

	glAttachShader(mProgramObject, vshader);
	glAttachShader(mProgramObject, fshader);

	switch(mMode) {
		case 0:
			glBindAttribLocation(mProgramObject, 0, "vPosition");
			break;

		case 1:
			glBindAttribLocation(mProgramObject, 0, "a_Position");
			glBindAttribLocation(mProgramObject, 1, "a_Color");
			break;

		default:
			assert(0);
			break;
	}
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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, screenWidth, screenHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(mProgramObject);

}

void App::run()
{
	switch(mMode) {
		case 0:
			{
				GLfloat vertices[] = {0.0f, 0.5f, 0.0f,
					-0.5f, -0.5f, 0.0f,
					0.5f, -0.5f, 0.0f};

				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
				glEnableVertexAttribArray(0);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			break;

		case 1:
			{
				GLfloat vertices[] = {-0.5, 0.5, 0.0,
					-0.5, -0.5, 0.0,
					0.5, -0.5, 0.0,
					0.5,  0.5, 0.0,
					-0.5, 0.5, 1.0,
					-0.5, -0.5, 1.0,
					0.5, -0.5, 1.0,
					0.5,  0.5, 1.0};
				GLfloat colors[] = {1.0, 1.0, 1.0, 1.0,
					1.0, 0.0, 1.0, 1.0,
					1.0, 0.0, 0.0, 1.0,
					1.0, 1.0, 0.0, 1.0,
					0.0, 1.0, 0.0, 1.0,
					0.0, 1.0, 1.0, 1.0,
					0.0, 0.0, 1.0, 1.0,
					0.0, 0.0, 0.0, 1.0};

				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colors);
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				GLushort indices[] = {0, 1, 2,
					0, 2, 3,
					0, 3, 7,
					0, 7, 4,
					0, 4, 5,
					0, 5, 1,
					3, 2, 6,
					3, 6, 7};
				glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLushort),
						GL_UNSIGNED_SHORT, indices);
			}
			break;

		default:
			assert(0);
			break;
	}

	SDL_GL_SwapBuffers();

	sleep(3);
}

void usage(const char* p)
{
	std::cerr << "Usage: " << p << " [--colors]\n";
}

int main(int argc, char** argv)
{
	int mode = 0;
	if(argc != 1) {
		if(argc != 2) {
			std::cerr << "Unknown parameters.\n";
			usage(argv[0]);
			exit(1);
		}
		if(!strcmp(argv[1], "--colors")) {
			mode = 1;
		} else {
			std::cerr << "Unknown parameters.\n";
			usage(argv[0]);
			exit(1);
		}
	}
	try {
		App app(mode);
		app.run();
	} catch(std::exception& e) {
		std::cerr << "std::exception: " << e.what() << "\n";
	} catch(...) {
		std::cerr << "Unknown exception.\n";
	}
	return 0;
}

