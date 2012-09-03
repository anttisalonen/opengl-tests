#ifndef APP_H
#define APP_H

#include <map>

#include <SDL.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "libcommon/Vector3.h"
#include "libcommon/Matrix44.h"

class App {
	public:
		App(int screenWidth, int screenHeight);
		virtual ~App() { }
		void run();
		virtual const char* getVertexShaderFilename() = 0;
		virtual const char* getFragmentShaderFilename() = 0;
		virtual void bindAttributes() = 0;
		virtual void draw() = 0;
		virtual void postInit() { }
		virtual bool handleEvent(const SDL_Event& ev);

	protected:
		static Common::Matrix44 translationMatrix(const Common::Vector3& v);
		static Common::Matrix44 rotationMatrixFromEuler(const Common::Vector3& v);
		static Common::Matrix44 perspectiveMatrix(float fov, int screenwidth, int screenheight);
		static Common::Matrix44 cameraRotationMatrix(const Common::Vector3& tgt, const Common::Vector3& up);
		static GLuint loadTexture(const char* filename);

		GLuint mProgramObject;
		std::map<const char*, GLint> mUniformLocationMap;

	private:
		void init();
		bool handleInput();
		GLuint loadShader(GLenum type, const char* src);
		GLuint loadShaderFromFile(GLenum type, const char* filename);

		SDL_Surface* mScreen;
		bool mInit;
		int mScreenWidth;
		int mScreenHeight;
};

#endif

