#include "Scene.h"

#include <cassert>

#include "HelperFunctions.h"

#include "libcommon/Texture.h"
#include "libcommon/Math.h"

using namespace Common;

namespace Scene {

const Vector3 WorldForward = Vector3(1, 0, 0);
const Vector3 WorldUp      = Vector3(0, 1, 0);

Camera::Camera()
	: mTarget(WorldForward),
	mUp(WorldUp),
	mHRot(0.0f),
	mVRot(0.0f)
{
}

void Camera::lookAt(const Common::Vector3& tgt, const Common::Vector3& right)
{
#if 0
	/* TODO: this doesn't work due to mHRot and mVRot. */
	auto v = tgt - mPosition;
	if(v.null() || right.null())
		return;

	auto t = v.normalized();
	auto u = right.cross(t.normalized());
	if(t.null() || u.null())
		return;

	mTarget = t;
	mUp = u;
#endif
}

void Camera::setMovementKey(const std::string& key, float forward,
		float up, float sideways)
{
	std::tuple<float, float, float> t(forward, up, sideways);
	mMovement[key] = t;
	mMovementCache[t] = calculateMovement(t);
}

Vector3 Camera::calculateMovement(const std::tuple<float, float, float>& v)
{
	Vector3 r;
	r += mTarget * std::get<0>(v);
	r += mUp * std::get<1>(v);
	r += mTarget.cross(mUp) * std::get<2>(v);
	return r;
}

void Camera::clearMovementKey(const std::string& key)
{
	auto t = mMovement[key];
	mMovementCache[t].zero();
	std::get<0>(t) = 0;
	std::get<1>(t) = 0;
	std::get<2>(t) = 0;
}

void Camera::applyMovementKeys(float coeff)
{
	for(auto p : mMovement) {
		mPosition += calculateMovement(p.second);
	}
}

void Camera::setForwardMovement(float speed)
{
	setMovementKey("Forward", speed, 0, 0);
}

void Camera::clearForwardMovement()
{
	clearMovementKey("Forward");
}

void Camera::setSidewaysMovement(float speed)
{
	setMovementKey("Sideways", 0, 0, speed);
}

void Camera::clearSidewaysMovement()
{
	clearMovementKey("Sideways");
}

void Camera::setUpwardsMovement(float speed)
{
	setMovementKey("Upwards", 0, speed, 0);
}

void Camera::clearUpwardsMovement()
{
	clearMovementKey("Upwards");
}

void Camera::rotate(float yaw, float pitch)
{
	mHRot += yaw;
	mVRot += pitch;

	Vector3 view = Math::rotate3D(WorldForward, mHRot, WorldUp).normalized();

	auto haxis = WorldUp.cross(view).normalized();

	mTarget = Math::rotate3D(view, -mVRot, haxis).normalized();
	mUp = mTarget.cross(haxis).normalized();

	for(auto p : mMovement) {
		mMovementCache[p.second] = calculateMovement(p.second);
	}
}

const Common::Vector3& Camera::getTargetVector() const
{
	return mTarget;
}

const Common::Vector3& Camera::getUpVector() const
{
	return mUp;
}

Light::Light(const Common::Color& col, bool on)
	: mOn(on)
{
	setColor(col);
}

void Light::setState(bool on)
{
	mOn = on;
}

bool Light::isOn() const
{
	return mOn;
}

const Common::Vector3& Light::getColor() const
{
	return mColor;
}

void Light::setColor(const Common::Color& c)
{
	mColor = Vector3(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
}

void Light::setColor(const Common::Vector3& c)
{
	mColor = c;
}

PointLight::PointLight(const Common::Vector3& pos, const Common::Vector3& attenuation, const Common::Color& col, bool on)
	: Light(col, on),
	Movable(pos),
	mAttenuation(attenuation)
{
}

const Common::Vector3& PointLight::getAttenuation() const
{
	return mAttenuation;
}

void PointLight::setAttenuation(const Common::Vector3& v)
{
	mAttenuation = v;
}

DirectionalLight::DirectionalLight(const Common::Vector3& dir, const Common::Color& col, bool on)
	: Light(col, true),
	mDirection(dir.normalized())
{
}

const Common::Vector3& DirectionalLight::getDirection() const
{
	return mDirection;
}

void DirectionalLight::setDirection(const Common::Vector3& dir)
{
	mDirection = dir.normalized();
}



Scene::Scene(float screenWidth, float screenHeight)
	: mScreenWidth(screenWidth),
	mScreenHeight(screenHeight),
	mAmbientLight(Color::White, false),
	mDirectionalLight(Vector3(1, 0, 0), Color::White, false),
	mPointLight(Vector3(), Vector3(), Color::White, false)
{
	GLuint vshader;
	GLuint fshader;
	GLint linked;

	GLenum glewerr = glewInit();
	if (glewerr != GLEW_OK) {
		std::cerr << "Unable to initialise GLEW.\n";
		throw std::runtime_error("Error initialising 3D");
	}
	if (!GLEW_VERSION_2_1) {
		std::cerr << "OpenGL 2.1 not supported.\n";
		throw std::runtime_error("Error initialising 3D");
	}

	vshader = HelperFunctions::loadShaderFromFile(GL_VERTEX_SHADER, "scene.vert");
	fshader = HelperFunctions::loadShaderFromFile(GL_FRAGMENT_SHADER, "scene.frag");

	mProgramObject = glCreateProgram();

	if(mProgramObject == 0) {
		std::cerr << "Unable to create program.\n";
		throw std::runtime_error("Error initialising 3D");
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
		throw std::runtime_error("Error initialising 3D");
	}

	HelperFunctions::enableDepthTest();
	glEnable(GL_TEXTURE_2D);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, screenWidth, screenHeight);

	glUseProgram(mProgramObject);

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

	for(auto& p : mUniformLocationMap) {
		p.second = glGetUniformLocation(mProgramObject, p.first);
	}

}

void Scene::bindAttributes()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindAttribLocation(mProgramObject, 0, "a_Position");
	glBindAttribLocation(mProgramObject, 1, "a_Texcoord");
	glEnableVertexAttribArray(2);
	glBindAttribLocation(mProgramObject, 2, "a_Normal");
}

void Scene::setupModelData(const Model& model)
{
	GLuint vboids[4];
	glGenBuffers(4, vboids);
	struct attrib {
		const char* name;
		int elems;
		const std::vector<GLfloat>& data;
	};


	attrib attribs[] = { { "a_Position", 3, model.getVertexCoords() },
		{ "a_Texcoord", 2, model.getTexCoords() },
		{ "m_Normals", 3, model.getNormals() } };

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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.getIndices().size() * sizeof(GLushort),
			&model.getIndices()[0], GL_STATIC_DRAW);
}

boost::shared_ptr<Common::Texture> Scene::getModelTexture(const std::string& mname) const
{
	auto it = mMeshInstanceTextures.find(mname);
	if(it == mMeshInstanceTextures.end()) {
		assert(0);
		throw std::runtime_error("Couldn't find texture for model");
	} else {
		return it->second;
	}
}

Camera& Scene::getDefaultCamera()
{
	return mDefaultCamera;
}

void Scene::addSkyBox()
{
	/* TODO */
}

Light& Scene::getAmbientLight()
{
	return mAmbientLight;
}

DirectionalLight& Scene::getDirectionalLight()
{
	return mDirectionalLight;
}

PointLight& Scene::getPointLight()
{
	return mPointLight;
}

void Scene::addQuad(const Vector3& p1,
		const Vector3& p2,
		const Vector3& p3,
		const Vector3& p4,
		const Color& c)
{
	/* TODO */
}

void Scene::calculateModelMatrix(const MeshInstance& mi)
{
	auto translation = HelperFunctions::translationMatrix(mi.getPosition());
	auto rotation = mi.getRotation();
	mModelMatrix = rotation * translation;

	auto invTranslation(translation);
	invTranslation.m[3] = -invTranslation.m[3];
	invTranslation.m[7] = -invTranslation.m[7];
	invTranslation.m[11] = -invTranslation.m[11];

	auto invRotation = rotation.transposed();

	mInverseModelMatrix = invTranslation * invRotation;
}

void Scene::updateMVPMatrix(const MeshInstance& mi)
{
	calculateModelMatrix(mi);
	auto mvp = mModelMatrix * mViewMatrix * mPerspectiveMatrix;
	auto imvp = mInverseModelMatrix;

	glUniformMatrix4fv(mUniformLocationMap["u_MVP"], 1, GL_FALSE, mvp.m);
	glUniformMatrix4fv(mUniformLocationMap["u_inverseMVP"], 1, GL_FALSE, imvp.m);
}

void Scene::updateFrameMatrices(const Camera& cam)
{
	mPerspectiveMatrix = HelperFunctions::perspectiveMatrix(90.0f, mScreenWidth, mScreenHeight);
	auto camrot = HelperFunctions::cameraRotationMatrix(cam.getTargetVector(), cam.getUpVector());
	auto camtrans = HelperFunctions::translationMatrix(cam.getPosition().negated());
	mViewMatrix = camtrans * camrot;
}

void Scene::render()
{
	glUniform1i(mUniformLocationMap["u_ambientLightEnabled"], mAmbientLight.isOn());
	glUniform1i(mUniformLocationMap["u_directionalLightEnabled"], mDirectionalLight.isOn());
	glUniform1i(mUniformLocationMap["u_pointLightEnabled"], mPointLight.isOn());

	updateFrameMatrices(mDefaultCamera);

	if(mPointLight.isOn()) {
		auto at = mPointLight.getAttenuation();
		auto col = mPointLight.getColor();
		glUniform3f(mUniformLocationMap["u_pointLightAttenuation"], at.x, at.y, at.z);
		glUniform3f(mUniformLocationMap["u_pointLightColor"], col.x, col.y, col.z);
	}

	if(mDirectionalLight.isOn()) {
		auto col = mDirectionalLight.getColor();
		glUniform3f(mUniformLocationMap["u_directionalLightColor"], col.x, col.y, col.z);
	}

	if(mAmbientLight.isOn()) {
		auto col = mAmbientLight.getColor();
		glUniform3f(mUniformLocationMap["u_ambientLight"], col.x, col.y, col.z);
	}

	for(auto& mi : mMeshInstances) {
		/* TODO: add support for vertex colors. */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getModelTexture(mi.first)->getTexture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glUniform1i(mUniformLocationMap["s_texture"], 0);

		updateMVPMatrix(*mi.second);

		if(mPointLight.isOn()) {
			// inverse translation matrix
			Vector3 plpos(mPointLight.getPosition());
			Vector3 plposrel = mi.second->getPosition() - plpos;
			glUniform3f(mUniformLocationMap["u_pointLightPosition"],
					plposrel.x, plposrel.y, plposrel.z);
		}

		if(mDirectionalLight.isOn()) {
			// inverse rotation matrix (normal matrix)
			Vector3 dir = mDirectionalLight.getDirection();
			glUniform3f(mUniformLocationMap["u_directionalLightDirection"], dir.x, dir.y, dir.z);
		}

		glDrawElements(GL_TRIANGLES, mi.second->getModel().getIndices().size(),
				GL_UNSIGNED_SHORT, NULL);
	}
}

void Scene::addTexture(const std::string& name, const std::string& filename)
{
	if(mTextures.find(name) != mTextures.end()) {
		throw std::runtime_error("Tried adding an already existing texture");
	} else {
		mTextures.insert({name, HelperFunctions::loadTexture(filename)});
	}
}

void Scene::addModel(const std::string& name, const std::string& filename)
{
	if(mModels.find(name) != mModels.end()) {
		throw std::runtime_error("Tried adding a model with an already existing name");
	} else {
		auto m = boost::shared_ptr<Model>(new Model(filename));
		mModels.insert({name, m});
		setupModelData(*m);
	}
}

boost::shared_ptr<Model> Scene::getModel(const std::string& name)
{
	auto it = mModels.find(name);
	if(it == mModels.end()) {
		throw std::runtime_error("Tried getting a non-existing model\n");
	} else {
		return it->second;
	}
}

boost::shared_ptr<MeshInstance> Scene::addMeshInstance(const std::string& name,
		const std::string& modelname, const std::string& texturename)
{
	if(mMeshInstances.find(name) != mMeshInstances.end()) {
		throw std::runtime_error("Tried adding a mesh instance with an already existing name");
	}

	auto modelit = mModels.find(modelname);
	if(modelit == mModels.end())
		throw std::runtime_error("Tried getting a non-existing model\n");

	auto textit = mTextures.find(texturename);
	if(textit == mTextures.end())
		throw std::runtime_error("Tried getting a non-existing texture\n");

	auto mi = boost::shared_ptr<MeshInstance>(new MeshInstance(*modelit->second));
	mMeshInstances.insert({name, mi});

	mMeshInstanceTextures.insert({name, textit->second});

	return mi;
}

}

