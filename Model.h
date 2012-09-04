#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"

class Model {
	public:
		Model(const char* filename);
		const std::vector<GLfloat>& getVertexCoords() const;
		const std::vector<GLfloat>& getTexCoords() const;
		const std::vector<GLushort> getIndices() const;
		const std::vector<GLfloat>& getNormals() const;
		bool isTextured() const;

	private:
		std::vector<GLfloat> mVertexCoords;
		std::vector<GLfloat> mTexCoords;
		std::vector<GLushort> mIndices;
		std::vector<GLfloat> mNormals;

		Assimp::Importer mImporter;
		const aiScene* mScene;
};

class MeshInstance {
	public:
		MeshInstance(const Model& m);
		const Common::Vector3& getPosition() const;
		const Common::Matrix44& getRotation() const;
		void setPosition(const Common::Vector3& v);
		void setRotationFromEuler(const Common::Vector3& v);
		void setRotation(const Common::Matrix44& m);
		const Model& getModel() const;

	private:
		const Model& mModel;
		Common::Vector3 mPosition;
		Common::Matrix44 mRotation;
};


#endif

