#include <unistd.h>
#include <cassert>
#include <fstream>
#include <iostream>

#include <map>
#include <vector>
#include <functional>
#include <stdexcept>

#include <SDL.h>
#include <SDL_image.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "libcommon/Math.h"
#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"
#include "libcommon/Texture.h"

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
GLfloat cubetexcoords[] = {0, 1,
	0, 0,
	1, 0,
	1, 1,
	1, 0,
	1, 1,
	1, 0,
	0, 0};


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

class Model {
	public:
		Model(const char* filename);
		const std::vector<GLfloat>& getVertexCoords() const;
		const std::vector<GLfloat>& getTexCoords() const;
		const std::vector<GLushort> getIndices() const;

	private:
		std::vector<GLfloat> mVertexCoords;
		std::vector<GLfloat> mTexCoords;
		std::vector<GLushort> mIndices;

		Assimp::Importer mImporter;
		const aiScene* mScene;
};

Model::Model(const char* filename)
{
	mScene = mImporter.ReadFile(filename,
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);
	if(!mScene) {
		std::cerr << "Unable to load model from " << filename << "\n";
		throw std::runtime_error("Error while loading model");
	}
	if(mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mNumMeshes) {
		std::cerr << "Model file " << filename << " is incomplete\n";
		throw std::runtime_error("Error while loading model");
	}

	aiMesh* mesh = mScene->mMeshes[0];
	if(!mesh->HasTextureCoords(0) || mesh->GetNumUVChannels() != 1) {
		std::cerr << "Model file " << filename << " has unsupported texture coordinates.\n";
		throw std::runtime_error("Error while loading model");
	}

	std::cout << mesh->mNumVertices << " vertices.\n";
	std::cout << mesh->mNumFaces << " faces.\n";

	for(int i = 0; i < mesh->mNumVertices; i++) {
		const aiVector3D& vertex = mesh->mVertices[i];
		mVertexCoords.push_back(vertex.x);
		mVertexCoords.push_back(vertex.y);
		mVertexCoords.push_back(vertex.z);

		const aiVector3D& texcoord = mesh->mTextureCoords[0][i];
		mTexCoords.push_back(texcoord.x);
		mTexCoords.push_back(texcoord.y);
	}

	for(int i = 0; i < mesh->mNumFaces; i++) {
		const aiFace& face = mesh->mFaces[i];
		if(face.mNumIndices != 3) {
			std::cerr << "Warning: number of indices should be three.\n";
			throw std::runtime_error("Error while loading model");
		} else {
			for(int j = 0; j < face.mNumIndices; j++) {
				mIndices.push_back(face.mIndices[j]);
			}
		}
	}
}

const std::vector<GLfloat>& Model::getVertexCoords() const
{
	return mVertexCoords;
}

const std::vector<GLfloat>& Model::getTexCoords() const
{
	return mTexCoords;
}

const std::vector<GLushort> Model::getIndices() const
{
	return mIndices;
}

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
		static Matrix44 cameraRotationMatrix(const Vector3& tgt, const Vector3& up);
		static GLuint loadTexture(const char* filename);

		GLuint mProgramObject;

	private:
		void init();
		bool handleInput();
		GLuint loadShader(GLenum type, const char* src);
		GLuint loadShaderFromFile(GLenum type, const char* filename);

		SDL_Surface* mScreen;
		bool mInit;
};

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

class Triangle : public App {
	public:
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void draw() override;
};

class Colors : public App {
	public:
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void postInit() override;
		virtual void draw() override;
};

class Rotate : public Colors {
	public:
		Rotate();
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void postInit() override;
		virtual void draw() override;
		virtual bool handleEvent(const SDL_Event& ev) override;

	protected:
		virtual Matrix44 calculateModelviewMatrix(const Matrix44& m) const;
		void updatePosition();
		void updateModelviewMatrix();

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
		virtual Matrix44 calculateModelviewMatrix(const Matrix44& m) const override;
};

class Camera : public Perspective {
	public:
		Camera();
		virtual bool handleEvent(const SDL_Event& ev) override;
		virtual void draw() override;

	protected:
		virtual Matrix44 calculateModelviewMatrix(const Matrix44& m) const override;
		void updateCamPos();

		Vector3 mCamPos;
		Vector3 mTarget;
		Vector3 mUp;
		float mPosStep;
		float mRotStep;
		float mHRot;
		float mVRot;
		std::map<SDLKey, Vector3> mCamPosDelta;
		std::map<SDLKey, std::function<Vector3 ()>> mControls;
		static Vector3 WorldForward;
		static Vector3 WorldUp;

	private:
		Vector3 forwardMovement();
		Vector3 sidewaysMovement();
		Vector3 upwardsMovement();
		void handleMouseMove(int xdiff, int ydiff);
};

class Textures : public Camera {
	public:
		Textures();
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void postInit() override;
		virtual void draw() override;

	protected:
		GLint mSTextLoc;
		GLuint mTexID;

		Model mModel;
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

Matrix44 Rotate::calculateModelviewMatrix(const Matrix44& m) const
{
	auto translation = translationMatrix(mPos);
	auto rotation = rotationMatrixFromEuler(mRot);
	return rotation * translation * m;
}

void Rotate::updatePosition()
{
	mPos += mPosDelta;
	mRot += mRotDelta;
}

void Rotate::updateModelviewMatrix()
{
	auto modelview = calculateModelviewMatrix(Matrix44::Identity);

	glUniformMatrix4fv(mMVPLoc, 1, GL_FALSE, modelview.m);
}

void Rotate::draw()
{
	updatePosition();
	updateModelviewMatrix();
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

Perspective::Perspective()
{
	mPos.z = -2.0f;
}

Matrix44 Perspective::calculateModelviewMatrix(const Matrix44& m) const
{
	auto pers = perspectiveMatrix(90.0f);
	return Rotate::calculateModelviewMatrix(pers);
}

Vector3 Camera::WorldForward = Vector3(1, 0, 0);
Vector3 Camera::WorldUp      = Vector3(0, 0, 1);

Camera::Camera()
	: mCamPos(Vector3(-2.2f, 0.0f, 0.0f)),
	mTarget(WorldForward),
	mUp(WorldUp),
	mPosStep(0.1f),
	mRotStep(0.02f),
	mHRot(0.0f),
	mVRot(0.0f)
{
	mPos = Vector3(0, 0, 0);
	mRot = Vector3(0, 0, 0.8);
	mControls[SDLK_UP] = [&] () { return forwardMovement(); };
	mControls[SDLK_PAGEUP] = [&] () { return upwardsMovement(); };
	mControls[SDLK_RIGHT] = [&] () { return sidewaysMovement(); };
	mControls[SDLK_DOWN] = [&] () { return forwardMovement() * -1.0f; };
	mControls[SDLK_PAGEDOWN] = [&] () { return upwardsMovement() * -1.0f; };
	mControls[SDLK_LEFT] = [&] () { return sidewaysMovement() * -1.0f; };
	handleMouseMove(0, 0);
}

Matrix44 Camera::calculateModelviewMatrix(const Matrix44& m) const
{
	auto pers = perspectiveMatrix(90.0f);
	auto camrot = cameraRotationMatrix(mTarget, mUp);
	auto camtrans = translationMatrix(mCamPos.negated());
	return Rotate::calculateModelviewMatrix(camtrans * camrot * pers);
}

void Camera::updateCamPos()
{
	for(auto& p : mCamPosDelta) {
		mCamPos += p.second;
	}
}

void Camera::draw()
{
	updateCamPos();
	Rotate::draw();
}

Vector3 Camera::forwardMovement()
{
	return mTarget.normalized() * mPosStep;
}

Vector3 Camera::sidewaysMovement()
{
	return mTarget.cross(mUp).normalized() * mPosStep;
}

Vector3 Camera::upwardsMovement()
{
	return mUp.normalized() * mPosStep;
}

bool Camera::handleEvent(const SDL_Event& ev)
{
	if(App::handleEvent(ev))
		return true;

	switch(ev.type) {
		case SDL_KEYDOWN:
			{
				auto it = mControls.find(ev.key.keysym.sym);
				if(it != mControls.end()) {
					mCamPosDelta[ev.key.keysym.sym] = it->second();
				} else {
					if(ev.key.keysym.sym == SDLK_p) {
						std::cout << "Up: " << mUp << "\n";
						std::cout << "Target: " << mTarget << "\n";
						std::cout << "Position: " << mCamPos << "\n";
					}
				}
			}
			break;

		case SDL_KEYUP:
			{
				auto it = mControls.find(ev.key.keysym.sym);
				if(it != mControls.end()) {
					mCamPosDelta[ev.key.keysym.sym] = Vector3();
				}
			}
			break;

		case SDL_MOUSEMOTION:
			if(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(1)) {
				handleMouseMove(ev.motion.xrel, ev.motion.yrel);
				Uint8 *keystate = SDL_GetKeyState(NULL);
				for(auto& p : mControls) {
					if(keystate[p.first]) {
						mCamPosDelta[p.first] = p.second();
					}
				}
			}
			break;

		default:
			break;
	}

	return false;
}

void Camera::handleMouseMove(int xdiff, int ydiff)
{
	mHRot += xdiff * mRotStep;
	mVRot += ydiff * mRotStep;

	Vector3 view = Math::rotate3D(WorldForward, mHRot, WorldUp).normalized();

	auto haxis = WorldUp.cross(view).normalized();

	mTarget = Math::rotate3D(view, -mVRot, haxis).normalized();
	mUp = mTarget.cross(haxis).normalized();
}

Textures::Textures()
	: mModel(Model("textured-cube.obj"))
{
	assert(mModel.getVertexCoords().size());
	assert(mModel.getTexCoords().size());
	assert(mModel.getIndices().size());
}

const char* Textures::getVertexShaderFilename()
{
	return "textures.vert";
}

const char* Textures::getFragmentShaderFilename()
{
	return "textures.frag";
}

void Textures::postInit()
{
	mMVPLoc = glGetUniformLocation(mProgramObject, "u_MVP");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &mModel.getVertexCoords()[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, &mModel.getTexCoords()[0]);
	mSTextLoc = glGetUniformLocation(mProgramObject, "s_texture");
	mTexID = App::loadTexture("snow.jpg");
	glEnable(GL_TEXTURE_2D);
}

void Textures::draw()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(mSTextLoc, 0);

	updateCamPos();
	updatePosition();
	updateModelviewMatrix();
	glDrawElements(GL_TRIANGLES, mModel.getIndices().size(),
			GL_UNSIGNED_SHORT, &mModel.getIndices()[0]);
}

void Textures::bindAttributes()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindAttribLocation(mProgramObject, 0, "a_Position");
	glBindAttribLocation(mProgramObject, 1, "a_Texcoord");
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
		} else if(!strcmp(argv[1], "--camera")) {
			app = new Camera();
		} else if(!strcmp(argv[1], "--textures")) {
			app = new Textures();
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

