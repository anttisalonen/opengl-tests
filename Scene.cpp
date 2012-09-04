#include "Scene.h"

#include <cassert>

#include "HelperFunctions.h"

#include "libcommon/Texture.h"

using namespace Common;

namespace Scene {


Vector3 Scene::WorldForward = Vector3(1, 0, 0);
Vector3 Scene::WorldUp      = Vector3(0, 1, 0);


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

	for(auto& p : mUniformLocationMap) {
		p.second = glGetUniformLocation(mProgramObject, p.first);
	}

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

const Common::Texture& Scene::getModelTexture(const char* mname) const
{
	auto it = mTextures.find(mname);
	if(it == mTextures.end()) {
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

void Scene::setDirectionalLight(const Vector3& dir, const Color& col)
{
	mDirectionalLight.setDirection(dir);
	mDirectionalLight.setColor(col);
	mDirectionalLight.setState(true);
}

void Scene::setAmbientLight(const Color& col)
{
	mAmbientLight.setColor(col);
	mAmbientLight.setState(true);
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
		glUniform3f(mUniformLocationMap["u_pointLightColor"], col.r, col.g, col.b);
	}

	if(mDirectionalLight.isOn()) {
		glUniform3f(mUniformLocationMap["u_directionalLightColor"], 1.0f, 1.0f, 1.0f);
	}

	if(mAmbientLight.isOn()) {
		auto col = mAmbientLight.getColor();
		glUniform3f(mUniformLocationMap["u_ambientLight"], col.r, col.g, col.b);
	}

	for(auto mi : mMeshInstances) {
		if(mi.getModel().isTextured()) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, getModelTexture("TODO").getTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glUniform1i(mUniformLocationMap["s_texture"], 0);
		} else {
			/* TODO: read color data */
		}

		updateMVPMatrix(mi);

		if(mPointLight.isOn()) {
			// inverse translation matrix
			Vector3 plpos(mPointLight.getPosition());
			Vector3 plposrel = mi.getPosition() - plpos;
			glUniform3f(mUniformLocationMap["u_pointLightPosition"],
					plposrel.x, plposrel.y, plposrel.z);
		}

		if(mDirectionalLight.isOn()) {
			// inverse rotation matrix (normal matrix)
			Vector3 dir = mDirectionalLight.getDirection();
			auto col = mDirectionalLight.getColor();
			glUniform3f(mUniformLocationMap["u_directionalLightDirection"], dir.x, dir.y, dir.z);
			glUniform3f(mUniformLocationMap["u_directionalLightColor"], col.r, col.g, col.b);
		}

		glDrawElements(GL_TRIANGLES, mi.getModel().getIndices().size(),
				GL_UNSIGNED_SHORT, NULL);
	}
}


}

