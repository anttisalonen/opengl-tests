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
#include "libcommon/Clock.h"

#include "Model.h"
#include "App.h"
#include "HelperFunctions.h"

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

class Triangle : public App {
	public:
		Triangle();
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void draw() override;
};

class Colors : public App {
	public:
		Colors();
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
		void enableDepthTest();

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
		void setupTexturing();

		GLuint mTexID;
		Model mModel;
		bool mUseVBOs;
};

class AmbientLight : public Textures {
	public:
		AmbientLight();
		virtual const char* getFragmentShaderFilename() override;
		virtual void postInit() override;
		virtual void draw() override;
};

class DirectionalLight : public AmbientLight {
	public:
		DirectionalLight();
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void bindAttributes() override;
		virtual void postInit() override;
		virtual void draw() override;
};

class PointLight : public DirectionalLight {
	public:
		PointLight();
		virtual bool handleEvent(const SDL_Event& ev) override;
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void draw() override;

	protected:
		bool mAmbientLightEnabled;
		bool mDirectionalLightEnabled;
		bool mPointLightEnabled;
};

Triangle::Triangle()
	: App(screenWidth, screenHeight)
{
}

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

Colors::Colors()
	: App(screenWidth, screenHeight)
{
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
	mUniformLocationMap["u_MVP"] = -1;
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
	enableDepthTest();
}

void Rotate::enableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

Matrix44 Rotate::calculateModelviewMatrix(const Matrix44& m) const
{
	auto translation = HelperFunctions::translationMatrix(mPos);
	auto rotation = HelperFunctions::rotationMatrixFromEuler(mRot);
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

	glUniformMatrix4fv(mUniformLocationMap["u_MVP"], 1, GL_FALSE, modelview.m);
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
	auto pers = HelperFunctions::perspectiveMatrix(90.0f, screenWidth, screenHeight);
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
	auto pers = HelperFunctions::perspectiveMatrix(90.0f, screenWidth, screenHeight);
	auto camrot = HelperFunctions::cameraRotationMatrix(mTarget, mUp);
	auto camtrans = HelperFunctions::translationMatrix(mCamPos.negated());
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
	: mModel(Model("textured-cube.obj")),
	mUseVBOs(false)
{
	assert(mModel.getVertexCoords().size());
	assert(mModel.getTexCoords().size());
	assert(mModel.getIndices().size());
	mUniformLocationMap["s_texture"] = -1;
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
	enableDepthTest();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &mModel.getVertexCoords()[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, &mModel.getTexCoords()[0]);
	setupTexturing();
}

void Textures::setupTexturing()
{
	mTexID = HelperFunctions::loadTexture("snow.jpg");
	glEnable(GL_TEXTURE_2D);
}

void Textures::draw()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(mUniformLocationMap["s_texture"], 0);

	updateCamPos();
	updatePosition();
	updateModelviewMatrix();
	if(mUseVBOs) {
		glDrawElements(GL_TRIANGLES, mModel.getIndices().size(),
				GL_UNSIGNED_SHORT, NULL);
	} else {
		glDrawElements(GL_TRIANGLES, mModel.getIndices().size(),
				GL_UNSIGNED_SHORT, &mModel.getIndices()[0]);
	}
}

void Textures::bindAttributes()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindAttribLocation(mProgramObject, 0, "a_Position");
	glBindAttribLocation(mProgramObject, 1, "a_Texcoord");
}

AmbientLight::AmbientLight()
{
	mUniformLocationMap["u_ambientLight"] = -1;
}

const char* AmbientLight::getFragmentShaderFilename()
{
	return "ambient.frag";
}

void AmbientLight::postInit()
{
	Textures::postInit();
}

void AmbientLight::draw()
{
	double time = Clock::getTime();
	float timePoint = Math::degreesToRadians(fmodl(time * 20.0f, 360));
	float rvalue = sin(timePoint);
	float gvalue = sin(timePoint + 2.0f * PI / 3.0f);
	float bvalue = sin(timePoint + 4.0f * PI / 3.0f);
	glUniform3f(mUniformLocationMap["u_ambientLight"], rvalue, gvalue, bvalue);
	Textures::draw();
}

DirectionalLight::DirectionalLight()
{
	assert(mModel.getNormals().size());

	mUniformLocationMap["u_directionalLightDirection"] = -1;
	mUniformLocationMap["u_directionalLightColor"] = -1;
}

const char* DirectionalLight::getVertexShaderFilename()
{
	return "directional.vert";
}

const char* DirectionalLight::getFragmentShaderFilename()
{
	return "directional.frag";
}

void DirectionalLight::postInit()
{
	mUseVBOs = true;

	GLuint vboids[4];
	glGenBuffers(4, vboids);
	struct attrib {
		const char* name;
		int elems;
		const std::vector<GLfloat>& data;
	};


	attrib attribs[] = { { "a_Position", 3, mModel.getVertexCoords() },
		{ "a_Texcoord", 2, mModel.getTexCoords() },
		{ "m_Normals", 3, mModel.getNormals() } };

	int i = 0;
	for(auto& a : attribs) {
		glBindBuffer(GL_ARRAY_BUFFER, vboids[i]);
		glBufferData(GL_ARRAY_BUFFER, a.data.size() * sizeof(GLfloat), &a.data[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, a.elems, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindAttribLocation(mProgramObject, i, a.name);
		i++;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboids[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mModel.getIndices().size() * sizeof(GLushort),
			&mModel.getIndices()[0], GL_STATIC_DRAW);

	enableDepthTest();
	setupTexturing();
}

void DirectionalLight::draw()
{
	glUniform3f(mUniformLocationMap["u_directionalLightDirection"], -1.0f, -1.0f, -1.0f);
	glUniform3f(mUniformLocationMap["u_directionalLightColor"], 1.0f, 1.0f, 1.0f);
	AmbientLight::draw();
}

void DirectionalLight::bindAttributes()
{
	AmbientLight::bindAttributes();
	glEnableVertexAttribArray(2);
	glBindAttribLocation(mProgramObject, 2, "a_Normal");
}

PointLight::PointLight()
	: mAmbientLightEnabled(true),
	mDirectionalLightEnabled(true),
	mPointLightEnabled(true)
{
	mUniformLocationMap["u_pointLightPosition"] = -1;
	mUniformLocationMap["u_pointLightAttenuation"] = -1;
	mUniformLocationMap["u_pointLightColor"] = -1;
	mUniformLocationMap["u_ambientLightEnabled"] = -1;
	mUniformLocationMap["u_directionalLightEnabled"] = -1;
	mUniformLocationMap["u_pointLightEnabled"] = -1;
}

bool PointLight::handleEvent(const SDL_Event& ev)
{
	if(DirectionalLight::handleEvent(ev))
		return true;

	if(ev.type == SDL_KEYDOWN) {
		if(ev.key.keysym.sym == SDLK_F1) {
			mAmbientLightEnabled = !mAmbientLightEnabled;
		} else if(ev.key.keysym.sym == SDLK_F2) {
			mDirectionalLightEnabled = !mDirectionalLightEnabled;
		} else if(ev.key.keysym.sym == SDLK_F3) {
			mPointLightEnabled = !mPointLightEnabled;
		}
	}

	return false;
}

const char* PointLight::getVertexShaderFilename()
{
	return "pointlight.vert";
}

const char* PointLight::getFragmentShaderFilename()
{
	return "pointlight.frag";
}

void PointLight::draw()
{
	glUniform1i(mUniformLocationMap["u_ambientLightEnabled"], mAmbientLightEnabled);
	glUniform1i(mUniformLocationMap["u_directionalLightEnabled"], mDirectionalLightEnabled);
	glUniform1i(mUniformLocationMap["u_pointLightEnabled"], mPointLightEnabled);

	double time = Clock::getTime();
	float timePoint = Math::degreesToRadians(fmodl(time * 160.0f, 360));
	float px = sin(timePoint);
	float py = cos(timePoint);
	glUniform3f(mUniformLocationMap["u_pointLightPosition"], px, py, 0.5f);
	glUniform3f(mUniformLocationMap["u_pointLightAttenuation"], 0.0f, 0.0f, 6.0f);
	glUniform3f(mUniformLocationMap["u_pointLightColor"], 1.0f, 1.0f, 1.0f);

	DirectionalLight::draw();
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
		} else if(!strcmp(argv[1], "--ambient")) {
			app = new AmbientLight();
		} else if(!strcmp(argv[1], "--directional")) {
			app = new DirectionalLight();
		} else if(!strcmp(argv[1], "--pointlight")) {
			app = new PointLight();
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

