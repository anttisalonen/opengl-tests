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

#include "libcommon/Math.h"
#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"
#include "libcommon/Texture.h"
#include "libcommon/Clock.h"

#include "Model.h"
#include "App.h"

using namespace Common;

static const int screenWidth = 800;
static const int screenHeight = 600;

class MeshInstance {
	public:
		MeshInstance(const Model& m);
		const Vector3& getPosition() const;
		const Matrix44& getRotation() const;
		void setPosition(const Vector3& v);
		void setRotationFromEuler(const Vector3& v);
		void setRotation(const Matrix44& m);
		const Model& getModel() const;

	private:
		const Model& mModel;
		Vector3 mPosition;
		Matrix44 mRotation;
};

MeshInstance::MeshInstance(const Model& m)
	: mModel(m)
{
}

const Vector3& MeshInstance::getPosition() const
{
	return mPosition;
}

const Matrix44& MeshInstance::getRotation() const
{
	return mRotation;
}

void MeshInstance::setPosition(const Vector3& v)
{
	mPosition = v;
}

void MeshInstance::setRotationFromEuler(const Vector3& v)
{
	mRotation = App::rotationMatrixFromEuler(v);
}

void MeshInstance::setRotation(const Matrix44& m)
{
	mRotation = m;
}

const Model& MeshInstance::getModel() const
{
	return mModel;
}

class Camera : public App {
	public:
		Camera();
		virtual bool handleEvent(const SDL_Event& ev) override;
		virtual const char* getVertexShaderFilename() override;
		virtual const char* getFragmentShaderFilename() override;
		virtual void postInit() override;
		virtual void bindAttributes() override;
		virtual void draw() override;


	protected:
		void calculateModelMatrix(const MeshInstance& mi);
		void updateCamPos();
		void updateFrameMatrices();
		void updateMVPMatrix(const MeshInstance& mi);
		void enableDepthTest();

		Vector3 mCamPos;
		Vector3 mTarget;
		Vector3 mUp;
		float mPosStep;
		float mRotStep;
		float mHRot;
		float mVRot;
		std::map<SDLKey, Vector3> mCamPosDelta;
		std::map<SDLKey, std::function<Vector3 ()>> mControls;
		std::vector<MeshInstance> mMeshInstances;

		Model mModel;

		bool mAmbientLightEnabled;
		bool mDirectionalLightEnabled;
		bool mPointLightEnabled;

		GLuint mTexID;

		Matrix44 mInverseModelMatrix;
		Matrix44 mModelMatrix;

		Matrix44 mViewMatrix;
		Matrix44 mPerspectiveMatrix;

		static Vector3 WorldForward;
		static Vector3 WorldUp;

	private:
		Vector3 forwardMovement();
		Vector3 sidewaysMovement();
		Vector3 upwardsMovement();
		void handleMouseMove(int xdiff, int ydiff);

	private:
		void setupTexturing();

};

void Camera::enableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void Camera::calculateModelMatrix(const MeshInstance& mi)
{
	auto translation = translationMatrix(mi.getPosition());
	auto rotation = mi.getRotation();
	mModelMatrix = rotation * translation;

	auto invTranslation(translation);
	invTranslation.m[3] = -invTranslation.m[3];
	invTranslation.m[7] = -invTranslation.m[7];
	invTranslation.m[11] = -invTranslation.m[11];

	auto invRotation = rotation.transposed();

	mInverseModelMatrix = invTranslation * invRotation;
}

void Camera::updateMVPMatrix(const MeshInstance& mi)
{
	calculateModelMatrix(mi);
	auto mvp = mModelMatrix * mViewMatrix * mPerspectiveMatrix;
	auto imvp = mInverseModelMatrix;

	glUniformMatrix4fv(mUniformLocationMap["u_MVP"], 1, GL_FALSE, mvp.m);
	glUniformMatrix4fv(mUniformLocationMap["u_inverseMVP"], 1, GL_FALSE, imvp.m);
}

Vector3 Camera::WorldForward = Vector3(1, 0, 0);
Vector3 Camera::WorldUp      = Vector3(0, 0, 1);

Camera::Camera()
	: App(screenWidth, screenHeight),
	mCamPos(Vector3(-2.2f, 0.0f, 0.0f)),
	mTarget(WorldForward),
	mUp(WorldUp),
	mPosStep(0.1f),
	mRotStep(0.02f),
	mHRot(0.0f),
	mVRot(0.0f),
	mModel(Model("textured-cube.obj")),
	mAmbientLightEnabled(true),
	mDirectionalLightEnabled(true),
	mPointLightEnabled(true)
{
	assert(mModel.getVertexCoords().size());
	assert(mModel.getTexCoords().size());
	assert(mModel.getIndices().size());
	assert(mModel.getNormals().size());

	mControls[SDLK_UP] = [&] () { return forwardMovement(); };
	mControls[SDLK_PAGEUP] = [&] () { return upwardsMovement(); };
	mControls[SDLK_RIGHT] = [&] () { return sidewaysMovement(); };
	mControls[SDLK_DOWN] = [&] () { return forwardMovement() * -1.0f; };
	mControls[SDLK_PAGEDOWN] = [&] () { return upwardsMovement() * -1.0f; };
	mControls[SDLK_LEFT] = [&] () { return sidewaysMovement() * -1.0f; };
	handleMouseMove(0, 0);

	MeshInstance m(mModel);
	m.setPosition(Vector3(-0.1f, 0.0f, 0.1f));
	m.setRotation(Matrix44::Identity);
	mMeshInstances.push_back(m);

	MeshInstance m2(mModel);
	m.setPosition(Vector3(3.0f, 3.0f, 0.0f));
	m.setRotationFromEuler(Vector3(Math::degreesToRadians(149),
				Math::degreesToRadians(150),
				Math::degreesToRadians(38)));
	mMeshInstances.push_back(m);

	mUniformLocationMap["u_MVP"] = -1;
	mUniformLocationMap["u_inverseMVP"] = -1;

	mUniformLocationMap["s_texture"] = -1;

	mUniformLocationMap["u_ambientLight"] = -1;

	mUniformLocationMap["u_directionalLightDirection"] = -1;
	mUniformLocationMap["u_directionalLightColor"] = -1;

	mUniformLocationMap["u_pointLightPosition"] = -1;
	mUniformLocationMap["u_pointLightAttenuation"] = -1;
	mUniformLocationMap["u_pointLightColor"] = -1;

	mUniformLocationMap["u_ambientLightEnabled"] = -1;
	mUniformLocationMap["u_directionalLightEnabled"] = -1;
	mUniformLocationMap["u_pointLightEnabled"] = -1;
}

void Camera::updateFrameMatrices()
{
	mPerspectiveMatrix = perspectiveMatrix(90.0f, screenWidth, screenHeight);
	auto camrot = cameraRotationMatrix(mTarget, mUp);
	auto camtrans = translationMatrix(mCamPos.negated());
	mViewMatrix = camtrans * camrot;
}

void Camera::updateCamPos()
{
	for(auto& p : mCamPosDelta) {
		mCamPos += p.second;
	}
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
					} else if(ev.key.keysym.sym == SDLK_F1) {
						mAmbientLightEnabled = !mAmbientLightEnabled;
					} else if(ev.key.keysym.sym == SDLK_F2) {
						mDirectionalLightEnabled = !mDirectionalLightEnabled;
					} else if(ev.key.keysym.sym == SDLK_F3) {
						mPointLightEnabled = !mPointLightEnabled;
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

void Camera::setupTexturing()
{
	mTexID = App::loadTexture("snow.jpg");
	glEnable(GL_TEXTURE_2D);
}

const char* Camera::getVertexShaderFilename()
{
	return "cube.vert";
}

const char* Camera::getFragmentShaderFilename()
{
	return "pointlight.frag";
}

void Camera::postInit()
{
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

void Camera::bindAttributes()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindAttribLocation(mProgramObject, 0, "a_Position");
	glBindAttribLocation(mProgramObject, 1, "a_Texcoord");
	glEnableVertexAttribArray(2);
	glBindAttribLocation(mProgramObject, 2, "a_Normal");
}

void Camera::draw()
{
	glUniform1i(mUniformLocationMap["u_ambientLightEnabled"], mAmbientLightEnabled);
	glUniform1i(mUniformLocationMap["u_directionalLightEnabled"], mDirectionalLightEnabled);
	glUniform1i(mUniformLocationMap["u_pointLightEnabled"], mPointLightEnabled);

	double time = Clock::getTime();
	updateFrameMatrices();

	float pointLightTime = Math::degreesToRadians(fmodl(time * 160.0f, 360));
	if(mPointLightEnabled) {
		glUniform3f(mUniformLocationMap["u_pointLightAttenuation"], 0.0f, 0.0f, 6.0f);
		glUniform3f(mUniformLocationMap["u_pointLightColor"], 1.0f, 1.0f, 1.0f);
	}

	if(mDirectionalLightEnabled) {
		glUniform3f(mUniformLocationMap["u_directionalLightColor"], 1.0f, 1.0f, 1.0f);
	}

	{
		float timePoint = Math::degreesToRadians(fmodl(time * 20.0f, 360));
		float rvalue = sin(timePoint) * 0.5f;
		float gvalue = sin(timePoint + 2.0f * PI / 3.0f) * 0.5f;
		float bvalue = sin(timePoint + 4.0f * PI / 3.0f) * 0.5f;
		glUniform3f(mUniformLocationMap["u_ambientLight"], rvalue, gvalue, bvalue);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(mUniformLocationMap["s_texture"], 0);

	updateCamPos();
	for(auto mi : mMeshInstances) {
		updateMVPMatrix(mi);

		if(mPointLightEnabled) {
			// inverse translation matrix
			Vector3 plpos(sin(pointLightTime), cos(pointLightTime), 0.5f);
			Vector3 plposrel = mi.getPosition() - plpos;
			glUniform3f(mUniformLocationMap["u_pointLightPosition"],
					plposrel.x, plposrel.y, plposrel.z);
		}

		if(mDirectionalLightEnabled) {
			// inverse rotation matrix (normal matrix)
			Vector3 dir(-1, -1, -1);
			glUniform3f(mUniformLocationMap["u_directionalLightDirection"], dir.x, dir.y, dir.z);
			glUniform3f(mUniformLocationMap["u_directionalLightColor"], 1.0f, 1.0f, 1.0f);
		}

		glDrawElements(GL_TRIANGLES, mi.getModel().getIndices().size(),
				GL_UNSIGNED_SHORT, NULL);
	}
}

void usage(const char* p)
{
	std::cerr << "Usage: " << p << " [--colors]\n";
}

int main(int argc, char** argv)
{
	App* app = nullptr;
	app = new Camera();

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

