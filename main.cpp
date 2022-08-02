#include <Windows.h>
#include <iostream>
#include <d3d11.h>
#include <DirectXMath.h>
#include <chrono>
#include <vector>

#include "WindowHelper.h"
#include "BaseObject.h"
#include "Renderer.h"
#include "Pipeline.h"
#include "SharedResources.h"
#include "Camera.h"
#include "OBJParsing.h"
#include "Lights.h"
#include "QuadTree.h"

#define SceneStepRate 60
#define CameraPositionStepSize 0.05f
#define CameraOrientationStepSize 1.0f

void CameraUpdate(Camera& mainCamera);
void OnSceneStep();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	//Make sure width and height are multiples of 32 for compute shader to work properly
	const UINT WIDTH = 1024;
	const UINT HEIGHT = 576;
	HWND window;

	if (!SetupWindow(hInstance, WIDTH, HEIGHT, nCmdShow, window))
	{
		std::cerr << "Failed to setup window!" << std::endl;
		return -1;
	}

	Pipeline::SetupRender(WIDTH, HEIGHT, window);
	SharedResources::Setup();

	std::vector<Camera*> renderCameras;

	std::vector<Object*> sceneObjects;

	std::vector<LightBase*> sceneLights;

	QuadTree staticObjects = QuadTree(100, 3);

	Renderer renderer = Renderer(&sceneObjects, &staticObjects, &renderCameras, &sceneLights);

	STDOBJ hugin = STDOBJ("OBJ/Hugin.obj");

	hugin.Scale({ 0.05f, 0.05f, 0.05f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	hugin.Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	hugin.Translate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	//hugin.AddToQuadTree(&staticObjects);

	STDOBJTesselated huginSmooth = STDOBJTesselated("OBJ/Hugin smooth.obj", 8.0f, 5.0f, 0.0f, 0.75f);

	huginSmooth.Scale({ 3.0f, 3.0f, 3.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	huginSmooth.Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	huginSmooth.Translate({ 4.0f, 0.0f, -2.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	huginSmooth.AddToQuadTree(&staticObjects);

	STDOBJ cubes = STDOBJ("OBJ/simpleCube.obj");

	sceneObjects.push_back(&cubes);

	cubes.Translate({ 0.0f, -1.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	cubes.Scale({ 10.0f, 0.05f, 10.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	CameraPerspective mainCamera = CameraPerspective(WIDTH, HEIGHT, 0, 0, 90.0f, 0.1f, 20.0f);

	renderCameras.push_back(&mainCamera);

	mainCamera.Translate({ 3.0f, 6.0f, -5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	mainCamera.Rotate({ 30.0f, -30.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	AmbientLight ambientLight = AmbientLight({0.2f, 0.2f, 0.2f});

	sceneLights.push_back(&ambientLight);

	DirectionalLight testDirectionalLight = DirectionalLight({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f });

	testDirectionalLight.Translate({ 10.0f, 10.0f, 10.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	testDirectionalLight.Rotate({ 45.0f, -135.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	testDirectionalLight.CreateShadowMap(2000, 20.0f, 10.0f, 30.0f, &renderer);
	testDirectionalLight.SetCastShadows(true);

	sceneLights.push_back(&testDirectionalLight);

	
	SpotLight testSpotLight = SpotLight({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 45.0f, 10.0f);

	testSpotLight.CreateShadowMap(500, 5.0f, 30.0f, &renderer);
	testSpotLight.SetCastShadows(true);
	testSpotLight.Rotate({ 15.0f, 30.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	testSpotLight.Translate({ -5.0f, 6.0f, -7.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	//sceneLights.push_back(&testSpotLight);
	
	PointLight testPointLight = PointLight({0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f}, 10.0f);

	testPointLight.CreateShadowMap(500, 2.0f, 15.0f, &renderer);

	testPointLight.Translate({ 0.0f, 10.0f, -5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	testPointLight.SetCastShadows(true);

	sceneLights.push_back(&testPointLight);
	

	std::chrono::steady_clock timer;
	std::chrono::time_point<std::chrono::steady_clock> previous = timer.now();

	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		renderer.Render();
		renderer.Switch();

		std::chrono::time_point<std::chrono::steady_clock> now = timer.now();

		std::chrono::duration<double> deltaTime = now - previous;

		if (deltaTime.count() >= 1.0 / SceneStepRate)
		{
			previous = now;
			CameraUpdate(mainCamera);
			OnSceneStep();
		}
	}

	Pipeline::Release();
	SharedResources::Release();

	return 0;
}

void CameraUpdate(Camera& mainCamera)
{
	char input = GetInputTracker();
	if ((input & 0x01) > 0) //w
	{
		mainCamera.Translate({ 0.0f, 0.0f, CameraPositionStepSize }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND);
	}
	if ((input & 0x02) > 0) //a
	{
		mainCamera.Translate({ -CameraPositionStepSize, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND);
	}
	if ((input & 0x04) > 0) //s
	{
		mainCamera.Translate({ 0.0f, 0.0f, -CameraPositionStepSize }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND);
	}
	if ((input & 0x08) > 0) //d
	{
		mainCamera.Translate({ CameraPositionStepSize, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND);
	}


	if ((input & 0x10) > 0) //up arrow
	{
		mainCamera.Rotate({ -CameraOrientationStepSize, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);
	}
	if ((input & 0x20) > 0) //left arrow
	{
		mainCamera.Rotate({ 0.0f, -CameraOrientationStepSize, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);
	}
	if ((input & 0x40) > 0) //down arrow
	{
		mainCamera.Rotate({ CameraOrientationStepSize, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);
	}
	if ((input & 0x80) > 0) //right arrow
	{
		mainCamera.Rotate({ 0.0f, CameraOrientationStepSize, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);
	}
}

void OnSceneStep()
{
}
