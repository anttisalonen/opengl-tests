#include "Model.h"

#include <stdexcept>
#include <iostream>

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

		if(mesh->HasNormals()) {
			const aiVector3D& normal = mesh->mNormals[i];
			mNormals.push_back(normal.x);
			mNormals.push_back(normal.y);
			mNormals.push_back(normal.z);
		}
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

const std::vector<GLfloat>& Model::getNormals() const
{
	return mNormals;
}

