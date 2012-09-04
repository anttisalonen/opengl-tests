#ifndef SCENE_HELPERFUNCTIONS_H
#define SCENE_HELPERFUNCTIONS_H

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"

class HelperFunctions {
	public:
		static Common::Matrix44 translationMatrix(const Common::Vector3& v);
		static Common::Matrix44 rotationMatrixFromEuler(const Common::Vector3& v);
		static Common::Matrix44 perspectiveMatrix(float fov, int screenwidth, int screenheight);
		static Common::Matrix44 cameraRotationMatrix(const Common::Vector3& tgt, const Common::Vector3& up);

		static GLuint loadShader(GLenum type, const char* src);
		static GLuint loadShaderFromFile(GLenum type, const char* filename);

		static GLuint loadTexture(const char* filename);

		static void enableDepthTest();
};

#endif

