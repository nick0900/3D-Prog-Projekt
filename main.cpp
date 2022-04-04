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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
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

	Renderer renderer = Renderer(&sceneObjects, &renderCameras);

	STDOBJ hugin = STDOBJ("OBJ/Hugin.obj");

	sceneObjects.push_back(&hugin);

	hugin.Scale({ 0.05f, 0.05f, 0.05f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	hugin.Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	STDOBJ cubes = STDOBJ("OBJ/simpleCube.obj");

	sceneObjects.push_back(&cubes);

	Camera mainCamera = Camera(WIDTH, HEIGHT, 0, 0, 2.5f, 0.1f, 20.0f);

	renderCameras.push_back(&mainCamera);

	mainCamera.Translate({ 0.0f, 3.0f, -2.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	std::chrono::steady_clock timer;
	std::chrono::time_point<std::chrono::steady_clock> previous = timer.now();

	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		renderer.Render();
		Pipeline::Switch();
	}

	Pipeline::Release();
	SharedResources::Release();

	return 0;
}