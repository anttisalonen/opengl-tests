#include "Scene.h"

#include "libcommon/Math.h"
#include "libcommon/Clock.h"
#include "libcommon/DriverFramework.h"

static int screenWidth = 800;
static int screenHeight = 600;

using namespace Common;

class SceneCube : public Common::Driver {
	public:
		SceneCube();
		virtual bool handleKeyDown(float frameTime, SDLKey key) override;
		virtual bool handleKeyUp(float frameTime, SDLKey key) override;
		virtual bool handleMouseMotion(float frameTime, const SDL_MouseMotionEvent& ev) override;
		virtual bool prerenderUpdate(float frameTime) override;
		virtual void drawFrame() override;

	private:
		void handleMouseMove(float dx, float dy);
		Scene::Scene mScene;
		Scene::Camera& mCamera;
		float mPosStep;
		float mRotStep;
		bool mAmbientLightEnabled;
		bool mDirectionalLightEnabled;
		bool mPointLightEnabled;
		std::map<SDLKey, std::function<void (float)>> mControls;
};

SceneCube::SceneCube()
	: Common::Driver(screenWidth, screenHeight, "Cube"),
	mScene(Scene::Scene(800, 600)),
	mCamera(mScene.getDefaultCamera()),
	mPosStep(0.1f),
	mRotStep(0.02f),
	mAmbientLightEnabled(true),
	mDirectionalLightEnabled(true),
	mPointLightEnabled(true)
{
	mControls[SDLK_UP] = [&] (float p) { mCamera.setForwardMovement(p); };
	mControls[SDLK_PAGEUP] = [&] (float p) { mCamera.setUpwardsMovement(p); };
	mControls[SDLK_RIGHT] = [&] (float p) { mCamera.setSidewaysMovement(p); };
	mControls[SDLK_DOWN] = [&] (float p) { mCamera.setForwardMovement(-p); };
	mControls[SDLK_PAGEDOWN] = [&] (float p) { mCamera.setUpwardsMovement(-p); };
	mControls[SDLK_LEFT] = [&] (float p) { mCamera.setSidewaysMovement(-p); };

	mCamera = mScene.getDefaultCamera();
	mCamera.setPosition(Vector3(1.9f, 1.9f, -4.2f));
	mCamera.rotate(Math::degreesToRadians(90), 0);
	handleMouseMove(0, 0);

	mScene.addModel("Cube", "textured-cube.obj");
	mScene.addTexture("Snow", "snow.jpg");

	auto mi1 = mScene.addMeshInstance("Cube1", "Cube", "Snow");
	mi1->setPosition(Vector3(-0.1, 0.0f, 0.0f));

	auto mi2 = mScene.addMeshInstance("Cube2", "Cube", "Snow");
	mi2->setPosition(Vector3(3.0f, 3.0f, 0.0f));
	mi2->setRotationFromEuler(Vector3(Math::degreesToRadians(149),
				Math::degreesToRadians(150),
				Math::degreesToRadians(38)));

	mScene.getAmbientLight().setState(mAmbientLightEnabled);
	mScene.getDirectionalLight().setState(mDirectionalLightEnabled);
	mScene.getDirectionalLight().setDirection(Vector3(1, 1, 1));
	mScene.getDirectionalLight().setColor(Vector3(1, 0.8, 0.0));
	mScene.getPointLight().setState(mPointLightEnabled);
	mScene.getPointLight().setAttenuation(Vector3(0, 0, 3));
	mScene.getPointLight().setColor(Vector3(0.9, 0.2, 0.4));
}

bool SceneCube::handleKeyDown(float frameTime, SDLKey key)
{
	auto it = mControls.find(key);
	if(it != mControls.end()) {
		it->second(mPosStep);
	} else {
		if(key == SDLK_ESCAPE) {
			return true;
		}
		else if(key == SDLK_p) {
			std::cout << "Up: " << mCamera.getUpVector() << "\n";
			std::cout << "Target: " << mCamera.getTargetVector() << "\n";
			std::cout << "Position: " << mCamera.getPosition() << "\n";
		} else if(key == SDLK_F1) {
			mAmbientLightEnabled = !mAmbientLightEnabled;
			mScene.getAmbientLight().setState(mAmbientLightEnabled);
		} else if(key == SDLK_F2) {
			mDirectionalLightEnabled = !mDirectionalLightEnabled;
			mScene.getDirectionalLight().setState(mDirectionalLightEnabled);
		} else if(key == SDLK_F3) {
			mPointLightEnabled = !mPointLightEnabled;
			mScene.getPointLight().setState(mPointLightEnabled);
		}
	}

	return false;
}

bool SceneCube::handleKeyUp(float frameTime, SDLKey key)
{
	auto it = mControls.find(key);
	if(it != mControls.end()) {
		it->second(0.0f);
	}
	return false;
}

bool SceneCube::handleMouseMotion(float frameTime, const SDL_MouseMotionEvent& ev)
{
	if(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(1)) {
		handleMouseMove(ev.xrel, ev.yrel);
	}
	return false;
}

void SceneCube::handleMouseMove(float dx, float dy)
{
	mCamera.rotate(dx * mRotStep, dy * mRotStep);
}

bool SceneCube::prerenderUpdate(float frameTime)
{
	double time = Clock::getTime();
	if(mAmbientLightEnabled) {
		float timePoint = Math::degreesToRadians(fmodl(time * 20.0f, 360));
		float rvalue = 0.5f * (0.5f + 0.5f * sin(timePoint));
		float gvalue = 0.5f * (0.5f + 0.5f * sin(timePoint + 2.0f * PI / 3.0f));
		float bvalue = 0.5f * (0.5f + 0.5f * sin(timePoint + 4.0f * PI / 3.0f));
		mScene.getAmbientLight().setColor(Color(rvalue * 255, gvalue * 255, bvalue * 255));
	}

	if(mPointLightEnabled) {
		float pointLightTime = Math::degreesToRadians(fmodl(time * 80.0f, 360));
		Vector3 plpos(sin(pointLightTime), 0.5f, cos(pointLightTime));
		mScene.getPointLight().setPosition(plpos);
	}

	mCamera.applyMovementKeys(frameTime);

	return false;
}

void SceneCube::drawFrame()
{
	mScene.render();
}

int main(int argc, char** argv)
{
	try {
		SceneCube app;
		app.run();
	} catch(std::exception& e) {
		std::cerr << "std::exception: " << e.what() << "\n";
	} catch(...) {
		std::cerr << "Unknown exception.\n";
	}

	return 0;
}

