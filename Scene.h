#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H

#include <tuple>
#include <map>

#include <boost/shared_ptr.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"
#include "libcommon/Color.h"
#include "libcommon/Texture.h"

#include "Model.h"

namespace Scene {

extern const Common::Vector3 WorldForward;
extern const Common::Vector3 WorldUp;

enum class Reference {
	World,
	Local
};

class Camera : public Movable {
	public:
		Camera();
		void lookAt(const Common::Vector3& tgt, const Common::Vector3& right);
		void applyMovementKeys(float coeff);
		void setForwardMovement(float speed);
		void clearForwardMovement();
		void setSidewaysMovement(float speed);
		void clearSidewaysMovement();
		void setUpwardsMovement(float speed);
		void clearUpwardsMovement();
		void rotate(float yaw, float pitch);
		const Common::Vector3& getTargetVector() const;
		const Common::Vector3& getUpVector() const;

	private:
		void setMovementKey(const std::string& key, float forward,
				float up, float sideways);
		void clearMovementKey(const std::string& key);

		Common::Vector3 calculateMovement(const std::tuple<float, float, float>& tuple);

		Common::Vector3 mTarget;
		Common::Vector3 mUp;

		std::map<std::string, std::tuple<float, float, float>> mMovement;
		std::map<std::tuple<float, float, float>, Common::Vector3> mMovementCache;

		float mHRot;
		float mVRot;
};

class Light {
	public:
		Light(const Common::Color& col, bool on = true);
		void setState(bool on);
		bool isOn() const;
		const Common::Vector3& getColor() const;
		void setColor(const Common::Color& c);
		void setColor(const Common::Vector3& c);

	private:
		bool mOn;
		Common::Vector3 mColor;
};

class PointLight : public Light, public Movable {
	public:
		PointLight(const Common::Vector3& pos, const Common::Vector3& attenuation, const Common::Color& col, bool on = true);
		const Common::Vector3& getAttenuation() const;
		void setAttenuation(const Common::Vector3& v);

	private:
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
		Light& getAmbientLight();
		DirectionalLight& getDirectionalLight();
		PointLight& getPointLight();
		void addQuad(const Common::Vector3& p1,
				const Common::Vector3& p2,
				const Common::Vector3& p3,
				const Common::Vector3& p4,
				const Common::Color& c);
		void render();
		void addTexture(const std::string& name, const std::string& filename);
		void addModel(const std::string& name, const std::string& filename);
		boost::shared_ptr<Model> getModel(const std::string& name);
		boost::shared_ptr<MeshInstance> addMeshInstance(const std::string& name,
				const std::string& modelname,
				const std::string& texturename);

	private:
		void calculateModelMatrix(const MeshInstance& mi);
		void updateMVPMatrix(const MeshInstance& mi);
		void updateFrameMatrices(const Camera& cam);
		void bindAttributes();
		void setupModelData(const Model& model);
		boost::shared_ptr<Common::Texture> getModelTexture(const std::string& mname) const;

		float mScreenWidth;
		float mScreenHeight;

		GLuint mProgramObject;
		std::map<const char*, GLint> mUniformLocationMap;

		Camera mDefaultCamera;

		Light mAmbientLight;
		DirectionalLight mDirectionalLight;
		PointLight mPointLight;

		std::map<std::string, boost::shared_ptr<Common::Texture>> mTextures;

		Common::Matrix44 mInverseModelMatrix;
		Common::Matrix44 mModelMatrix;

		Common::Matrix44 mViewMatrix;
		Common::Matrix44 mPerspectiveMatrix;

		std::map<std::string, boost::shared_ptr<Model>> mModels;
		std::map<std::string, boost::shared_ptr<MeshInstance>> mMeshInstances;
		std::map<std::string, boost::shared_ptr<Common::Texture>> mMeshInstanceTextures;
};

}

#endif

