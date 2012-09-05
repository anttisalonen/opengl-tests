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
		Model(const std::string& filename);
		const std::vector<GLfloat>& getVertexCoords() const;
		const std::vector<GLfloat>& getTexCoords() const;
		const std::vector<GLushort> getIndices() const;
		const std::vector<GLfloat>& getNormals() const;

	private:
		std::vector<GLfloat> mVertexCoords;
		std::vector<GLfloat> mTexCoords;
		std::vector<GLushort> mIndices;
		std::vector<GLfloat> mNormals;

		Assimp::Importer mImporter;
		const aiScene* mScene;
};

class Movable {
	public:
		Movable();
		Movable(const Common::Vector3& pos);
		void setPosition(const Common::Vector3& p);
		const Common::Vector3& getPosition() const;
		void move(const Common::Vector3& v);

	protected:
		Common::Vector3 mPosition;
};

class MeshInstance : public Movable {
	public:
		MeshInstance(const Model& m);
		const Common::Matrix44& getRotation() const;
		void setRotationFromEuler(const Common::Vector3& v);
		void setRotation(const Common::Matrix44& m);
		const Model& getModel() const;

	private:
		const Model& mModel;
		Common::Matrix44 mRotation;
};


#endif

