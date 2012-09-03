#include "App.h"

#include <iostream>
#include <fstream>

#include <SDL_image.h>

#include "libcommon/Math.h"
#include "libcommon/Texture.h"

using namespace Common;

Matrix44 App::perspectiveMatrix(float fov, int screenwidth, int screenheight)
{
	const float aspect_ratio = screenwidth / screenheight;
	const float znear = 0.1f;
	const float zfar = 200.0f;
	const float h = 1.0 / tan(Math::degreesToRadians(fov * 0.5f));
	const float neg_depth = znear - zfar;

	Matrix44 pers = Matrix44::Identity;
	pers.m[0 * 4 + 0] = h / aspect_ratio;
	pers.m[1 * 4 + 1] = h;
	pers.m[2 * 4 + 2] = (zfar + znear) / neg_depth;
	pers.m[2 * 4 + 3] = -1.0;
	pers.m[3 * 4 + 2] = 2.0 * zfar * znear / neg_depth;
	pers.m[3 * 4 + 3] = 0.0;
	return pers;
}

Matrix44 App::cameraRotationMatrix(const Vector3& tgt, const Vector3& up)
{
	Vector3 n(tgt.negated().normalized());
	auto u = up.normalized().cross(n);
	auto v = n.cross(u);
	auto m = Matrix44::Identity;
	m.m[0] = u.x;
	m.m[1] = v.x;
	m.m[2] = n.x;
	m.m[4] = u.y;
	m.m[5] = v.y;
	m.m[6] = n.y;
	m.m[8] = u.z;
	m.m[9] = v.z;
	m.m[10] = n.z;

	return m;
}

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

	vshader = loadShaderFromFile(GL_VERTEX_SHADER, getVertexShaderFilename());
	fshader = loadShaderFromFile(GL_FRAGMENT_SHADER, getFragmentShaderFilename());

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

GLuint App::loadTexture(const char* filename)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLuint texture = Texture::loadTexture(filename);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (GLEW_VERSION_3_0) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		/* TODO: add mipmap generation */
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	return texture;
}

Matrix44 App::translationMatrix(const Vector3& v)
{
	Matrix44 translation = Matrix44::Identity;
	translation.m[3 * 4 + 0] = v.x;
	translation.m[3 * 4 + 1] = v.y;
	translation.m[3 * 4 + 2] = v.z;
	return translation;
}

Matrix44 App::rotationMatrixFromEuler(const Vector3& v)
{
	Matrix44 rotation = Matrix44::Identity;
	float cx = cos(v.x);
	float cy = cos(v.y);
	float cz = cos(v.z);
	float sx = sin(v.x);
	float sy = sin(v.y);
	float sz = sin(v.z);

	rotation.m[0 * 4 + 0] = cy * cz;
	rotation.m[1 * 4 + 0] = -cx * sz + sx * sy * cz;
	rotation.m[2 * 4 + 0] = sx * sz + cx * sy * cz;
	rotation.m[0 * 4 + 1] = cy * sz;
	rotation.m[1 * 4 + 1] = cx * cz + sx * sy * sz;
	rotation.m[2 * 4 + 1] = -sx * cz + cx * sy * sz;
	rotation.m[0 * 4 + 2] = -sy;
	rotation.m[1 * 4 + 2] = sx * cy;
	rotation.m[2 * 4 + 2] = cx * cy;

	return rotation;
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


