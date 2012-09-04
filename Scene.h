#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H

#include <map>

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"
#include "libcommon/Color.h"
#include "libcommon/Texture.h"

#include "Model.h"

namespace Scene {

enum class Reference {
	World,
	Local
};

class Camera {
	public:
		void setPosition(const Common::Vector3& p);
		const Common::Vector3& getPosition() const;
		void lookAt(const Common::Vector3& p);
		void move(Reference ref, const Common::Vector3& v);
		void setMovementKey(const char* key, Reference ref, const Common::Vector3& v);
		bool clearMovementKey(const char* key, Reference ref);
		void applyMovementKeys(float coeff);
		void setForwardMovement(Reference ref, float speed);
		void clearForwardMovement(Reference ref);
		void setSidewaysMovement(Reference ref, float speed);
		void clearSidewaysMovement(Reference ref);
		void rotate(float yaw, float pitch);
		const Common::Vector3& getTargetVector() const;
		const Common::Vector3& getUpVector() const;
};

class Light {
	public:
		Light(const Common::Color& col, bool on = true);
		void setState(bool on);
		bool isOn() const;
		const Common::Color& getColor() const;
		void setColor(const Common::Color& c);

	private:
		bool mOn;
		Common::Color mColor;
};

class PointLight : public Light {
	public:
		PointLight(const Common::Vector3& pos, const Common::Vector3& attenuation, const Common::Color& col, bool on = true);
		const Common::Vector3& getPosition() const;
		void setPosition(const Common::Vector3& v);
		const Common::Vector3& getAttenuation() const;
		void setAttenuation(const Common::Vector3& v);

	private:
		Common::Vector3 mPosition;
		Common::Vector3 mAttenuation;
};

class DirectionalLight : public Light {
	public:
		DirectionalLight(const Common::Vector3& dir, const Common::Color& col, bool on = true);
		const Common::Vector3& getDirection() const;
		void setDirection(const Common::Vector3& dir);

	private:
		Common::Vector3 mDirection;
};

class Scene {
	public:
		Scene(float screenWidth, float screenHeight);
		Camera& getDefaultCamera();
		void addSkyBox();
		void setDirectionalLight(const Common::Vector3& dir, const Common::Color& col);
		void setAmbientLight(const Common::Color& col);
		void addQuad(const Common::Vector3& p1,
				const Common::Vector3& p2,
				const Common::Vector3& p3,
				const Common::Vector3& p4,
				const Common::Color& c);
		void render();

	private:
		void calculateModelMatrix(const MeshInstance& mi);
		void updateMVPMatrix(const MeshInstance& mi);
		void updateFrameMatrices(const Camera& cam);
		void bindAttributes();
		void setupModelData(const Model& model);
		const Common::Texture& getModelTexture(const char* mname) const;

		float mScreenWidth;
		float mScreenHeight;

		GLuint mProgramObject;
		std::map<const char*, GLint> mUniformLocationMap;

		Camera mDefaultCamera;

		Light mAmbientLight;
		DirectionalLight mDirectionalLight;
		PointLight mPointLight;

		std::vector<MeshInstance> mMeshInstances;
		std::map<const char*, Common::Texture> mTextures;

		Common::Matrix44 mInverseModelMatrix;
		Common::Matrix44 mModelMatrix;

		Common::Matrix44 mViewMatrix;
		Common::Matrix44 mPerspectiveMatrix;

		static Common::Vector3 WorldForward;
		static Common::Vector3 WorldUp;

};

}

#endif

