#include <unistd.h>
#include <cassert>
#include <fstream>
#include <iostream>

#include <SDL.h>
#include <SDL_image.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Math.h"
#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"

using namespace Common;

static const int screenWidth = 800;
static const int screenHeight = 600;

GLfloat trianglevertices[] = {0.0f, 0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f};

GLfloat cubevertices[] = {-0.5, 0.5, 0.5,
	-0.5, -0.5, 0.5,
	0.5, -0.5, 0.5,
	0.5,  0.5, 0.5,
	-0.5, 0.5, -0.5,
	-0.5, -0.5, -0.5,
	0.5, -0.5, -0.5,
	0.5,  0.5, -0.5};
GLfloat cubecolors[] = {1.0, 1.0, 1.0, 1.0,
	1.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	1.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 0.0, 1.0};

GLushort cubeindices[] = {0, 1, 2,
                       0, 2, 3,
                       0, 3, 7,
                       0, 7, 4,
                       0, 4, 5,
                       0, 5, 1,
                       3, 2, 6,
                       3, 6, 7,
                       2, 1, 6,
                       1, 6, 5,
                       5, 6, 4,
                       4, 7, 6};

class App {
	public:
		App();
		virtual ~App() { }
		void run();
		virtual const char* getVertexShaderFilename() = 0;
		virtual const char* getFragmentShaderFilename() = 0;
		virtual void bindAttributes() = 0;
		virtual void draw() = 0;
		virtual void postInit() { }
		virtual bool handleEvent(const SDL_Event& ev);

	protected:
		static Matrix44 translationMatrix(const Vector3& v);
		static Matrix44 rotationMatrixFromEuler(const Vector3& v);
		static Matrix44 perspectiveMatrix(float fov);

		GLuint mProgramObject;

	private:
		void init();
		bool handleInput();
		GLuint loadShader(GLenum type, const char* src);
		GLuint loadShaderFromFile(GLenum type, const char* filename);

		SDL_Surface* mScreen;
		bool mInit;
};

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

class Triangle : public App {
	public:
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void draw() override;
};

const char* Triangle::getVertexShaderFilename()
{
	return "simple.vert";
}

const char* Triangle::getFragmentShaderFilename()
{
	return "simple.frag";
}

void Triangle::bindAttributes()
{
	glBindAttribLocation(mProgramObject, 0, "vPosition");
}

void Triangle::draw()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, trianglevertices);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

class Colors : public App {
	public:
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void postInit() override;
		virtual void draw() override;
};

const char* Colors::getVertexShaderFilename()
{
	return "colors.vert";
}

const char* Colors::getFragmentShaderFilename()
{
	return "colors.frag";
}

void Colors::bindAttributes()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindAttribLocation(mProgramObject, 0, "a_Position");
	glBindAttribLocation(mProgramObject, 1, "a_Color");
}

void Colors::postInit()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, cubevertices);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, cubecolors);
}

void Colors::draw()
{
	glDrawElements(GL_TRIANGLES, sizeof(cubeindices) / sizeof(GLushort),
			GL_UNSIGNED_SHORT, cubeindices);
}

class Rotate : public Colors {
	public:
		Rotate();
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void postInit() override;
		virtual void draw() override;
		virtual bool handleEvent(const SDL_Event& ev) override;

	protected:
		virtual Matrix44 calculateModelviewMatrix() const;
		GLint mMVPLoc;
		Vector3 mPos;
		Vector3 mRot;
		Vector3 mPosDelta;
		Vector3 mRotDelta;
};

class Perspective : public Rotate {
	public:
		Perspective();
	protected:
		virtual Matrix44 calculateModelviewMatrix() const override;
};

Perspective::Perspective()
{
	mPos.z = -2.0f;
}

Rotate::Rotate()
	: mPos(Vector3(-0.1f, 0.0, 0.1f)),
	mRot(Vector3(Math::degreesToRadians(149),
				Math::degreesToRadians(150),
				Math::degreesToRadians(38)))
{
}

const char* Rotate::getVertexShaderFilename()
{
	return "rotate.vert";
}

const char* Rotate::getFragmentShaderFilename()
{
	return "rotate.frag";
}

void Rotate::postInit()
{
	Colors::postInit();
	mMVPLoc = glGetUniformLocation(mProgramObject, "u_MVP");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

Matrix44 App::perspectiveMatrix(float fov)
{
	const float aspect_ratio = screenWidth / screenHeight;
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

Matrix44 Perspective::calculateModelviewMatrix() const
{
	auto pers = perspectiveMatrix(90.0f);
	auto translation = translationMatrix(mPos);
	auto rotation = rotationMatrixFromEuler(mRot);
	return rotation * translation * pers;
}

Matrix44 Rotate::calculateModelviewMatrix() const
{
	auto translation = translationMatrix(mPos);
	auto rotation = rotationMatrixFromEuler(mRot);
	return rotation * translation;
}

void Rotate::draw()
{
	mPos += mPosDelta;
	mRot += mRotDelta;

	auto modelview = calculateModelviewMatrix();

	glUniformMatrix4fv(mMVPLoc, 1, GL_FALSE, modelview.m);

	Colors::draw();
}

bool Rotate::handleEvent(const SDL_Event& ev)
{
	if(App::handleEvent(ev))
		return true;

	static const float posd = 0.10f;
	static const float rotd = 0.05f;

	switch(ev.type) {
		case SDL_KEYDOWN:
			switch(ev.key.keysym.sym) {
				case SDLK_w: mPosDelta.y += posd; break;
				case SDLK_s: mPosDelta.y -= posd; break;
				case SDLK_d: mPosDelta.x += posd; break;
				case SDLK_a: mPosDelta.x -= posd; break;
				case SDLK_q: mPosDelta.z += posd; break;
				case SDLK_e: mPosDelta.z -= posd; break;
				case SDLK_UP:       mRotDelta.x += rotd; break;
				case SDLK_DOWN:     mRotDelta.x -= rotd; break;
				case SDLK_RIGHT:    mRotDelta.y += rotd; break;
				case SDLK_LEFT:     mRotDelta.y -= rotd; break;
				case SDLK_PAGEUP:   mRotDelta.z += rotd; break;
				case SDLK_PAGEDOWN: mRotDelta.z -= rotd; break;
				case SDLK_p:
				    std::cout << "Position: " << mPos << "\nRotation: " << mRot << "\n";
				default: break;
			}
			break;

		case SDL_KEYUP:
			switch(ev.key.keysym.sym) {
				case SDLK_w: case SDLK_s: mPosDelta.y = 0.0f; break;
				case SDLK_d: case SDLK_a: mPosDelta.x = 0.0f; break;
				case SDLK_q: case SDLK_e: mPosDelta.z = 0.0f; break;
				case SDLK_UP:     case SDLK_DOWN:     mRotDelta.x = 0.0f; break;
				case SDLK_RIGHT:  case SDLK_LEFT:     mRotDelta.y = 0.0f; break;
				case SDLK_PAGEUP: case SDLK_PAGEDOWN: mRotDelta.z = 0.0f; break;
				default: break;
			}
			break;

		default:
			break;
	}

	return false;
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

App::App()
	: mInit(false)
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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, screenWidth, screenHeight);

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

void usage(const char* p)
{
	std::cerr << "Usage: " << p << " [--colors]\n";
}

int main(int argc, char** argv)
{
	App* app = nullptr;
	if(argc != 1) {
		if(argc != 2) {
			std::cerr << "Unknown parameters.\n";
			usage(argv[0]);
			exit(1);
		}
		if(!strcmp(argv[1], "--colors")) {
			app = new Colors();
		} else if(!strcmp(argv[1], "--rotate")) {
			app = new Rotate();
		} else if(!strcmp(argv[1], "--perspective")) {
			app = new Perspective();
		} else {
			std::cerr << "Unknown parameters.\n";
			usage(argv[0]);
			exit(1);
		}
	}

	if(!app) {
		app = new Triangle();
	}

	try {
		app->run();
	} catch(std::exception& e) {
		std::cerr << "std::exception: " << e.what() << "\n";
	} catch(...) {
		std::cerr << "Unknown exception.\n";
	}

	delete app;

	return 0;
}

