#ifndef SCENE_HELPERFUNCTIONS_H
#define SCENE_HELPERFUNCTIONS_H

#include <string>

#include <boost/shared_ptr.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"
#include "libcommon/Texture.h"

class HelperFunctions {
	public:
		static Common::Matrix44 translationMatrix(const Common::Vector3& v);
		static Common::Matrix44 rotationMatrixFromEuler(const Common::Vector3& v);
		static Common::Matrix44 perspectiveMatrix(float fov, int screenwidth, int screenheight);
		static Common::Matrix44 cameraRotationMatrix(const Common::Vector3& tgt, const Common::Vector3& up);

		static GLuint loadShader(GLenum type, const char* src);
		static GLuint loadShaderFromFile(GLenum type, const char* filename);

		static boost::shared_ptr<Common::Texture> loadTexture(const std::string& filename);

		static void enableDepthTest();
};

#endif

