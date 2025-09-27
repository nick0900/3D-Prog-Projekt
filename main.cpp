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
#include "ParticleSystems.h"

#define SceneStepRate 60
#define CameraPositionStepSize 0.05f
#define CameraOrientationStepSize 1.0f

void CameraUpdate(Camera& mainCamera);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	//--------------------------------Setup--------------------------------//
	
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


	//set up object vectors for renderers to use

	//these objects will always be rendered, even if not in view
	std::vector<Object*> dynamicObjects;

	//the lights the renderer will use
	std::vector<LightBase*> sceneLights;

	//Particle systems are rendered last by the renderers for their depth test to work properly
	std::vector<ParticleSystem*> particleSystems;

	//set up a quadtree for culling of static objects. First value is the width of the whole scene and the second value is the partitioning depth
	QuadTree staticObjects = QuadTree(100, 3);

	//the renderers will save a pointer to a pointer of the quadtree. this is so you can switch the used tree. 
	//once you have added an object to the tree there is no functionality to move or remove that object. so you have to reconstruct the whole tree if you need to make changes. 
	//hence why the quad tree is only used for static objects!
	QuadTree* staticObjectsPtr = &staticObjects;

	
	//get a view for the backbuffer
	ID3D11UnorderedAccessView* backbufferUAV;

	if (!Pipeline::GetBackbufferUAV(backbufferUAV))
	{
		std::cerr << "Failed to create backbuffer unordered access view!" << std::endl;
	}


	//setup all the different renderers needed with correct resource sizes
	DeferredRenderer mainRenderer = DeferredRenderer(WIDTH, HEIGHT, &dynamicObjects, &staticObjectsPtr, &sceneLights, &particleSystems);
	DepthRenderer shadowmapSingleRenderer = DepthRenderer(&dynamicObjects, &staticObjectsPtr);
	OmniDistanceRenderer shadowmapCubeRenderer = OmniDistanceRenderer(500, &dynamicObjects, &staticObjectsPtr);
	DeferredRenderer reflectionRenderer = DeferredRenderer(WIDTH, WIDTH, &dynamicObjects, &staticObjectsPtr, &sceneLights, &particleSystems);

	//---------------------------------------------------------------------//










	//---------------------------Camera config-----------------------------//
	

	CameraPerspective quadtreeDemoCamera = CameraPerspective(160, 160, 864, 0, 90.0f, 1.0f, 70.0f);

	quadtreeDemoCamera.Translate({ 0.0f, 6.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);


	CameraPerspectiveDebug mainCamera = CameraPerspectiveDebug(WIDTH, HEIGHT, 0, 0, 70.0f, 0.1f, 70.0f, &quadtreeDemoCamera); //use this camera to see outside the quadtreedemocamera frustum

	mainCamera.Translate({ 3.0f, 6.0f, -5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	mainCamera.Rotate({ 30.0f, -30.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	//The main camera is controlled by wasd and the arrow keys
	//press 3 to toggle quadtree demon

	//---------------------------------------------------------------------//










	//--------------------------------Scene--------------------------------//
	//------------------------------Art Piece------------------------------//

	//this is a model of a little robot, the regular version uses flat shading for all its normal configuration. this one however uses smooth shading for certain parts which tesselation can take advantage of to smooth out the shapes.
	//I have this model configured so that the tesselation is quite obvious, especially on the pistons under the body which seem to bloat as you get closer
	STDOBJTesselated huginSmoothNoHead = STDOBJTesselated("OBJ/Hugin smooth no head.obj", 8.0f, 5.0f, 0.0f, 0.75f);

	huginSmoothNoHead.Scale({ 3.0f, 3.0f, 3.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	huginSmoothNoHead.Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	huginSmoothNoHead.Translate({ 0.0f, 0.0f, 1.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	//huginSmoothNoHead.AddToQuadTree(&staticObjects);

	STDOBJMirror ico = STDOBJMirror("OBJ/IcoSphere.obj", HEIGHT, &reflectionRenderer, 0.1f, 20.0f);

	ico.Translate({ 0.0f, 3.4f, 0.9f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	ico.Scale({ 3.1f, 3.1f, 3.1f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	ico.AddToQuadTree(&staticObjects);

	//you can delete reandom particles in the galaxy by pressing 1 and replace them with red stars by pressing 2. you cannot add more than the initilized count.
	Galaxy galaxy = Galaxy("textures/star.png");
	particleSystems.push_back(&galaxy);

	galaxy.InitializeParticles(3000);

	galaxy.Translate({ 0.0f, 8.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	
	//---------------------------------------------------------------------//






	//--------------------------------Scene--------------------------------//
	//---------------------------quad tree demo----------------------------//

	STDOBJ* cornerCubes[81];

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			int index = j + i * 9;
			cornerCubes[index] = new STDOBJ("OBJ/simpleCube.obj");

			cornerCubes[index]->Translate({-50.0f + i * 12.5f, 0.0f, -50.0f + j * 12.5f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

			cornerCubes[index]->AddToQuadTree(&staticObjects);
		}
	}

	//---------------------------------------------------------------------//






	//--------------------------------Scene--------------------------------//
	//-----------------------------Miscelanious----------------------------//

	//the robot with original flat shading. made to rotate later in the render loop
	STDOBJ hugin = STDOBJ("OBJ/Hugin.obj");

	hugin.Scale({ 0.05f, 0.05f, 0.05f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	hugin.Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	hugin.Translate({ 10.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	dynamicObjects.push_back(&hugin);

	//another showcase of tesselation but the head is also tesselated and smooth shaded
	STDOBJTesselated huginSmooth = STDOBJTesselated("OBJ/Hugin smooth.obj", 8.0f, 5.0f, 0.0f, 0.75f);

	huginSmooth.Scale({ 3.0f, 3.0f, 3.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	huginSmooth.Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	huginSmooth.Translate({ 4.0f, 0.0f, -2.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	huginSmooth.AddToQuadTree(&staticObjects);

	//cube transformed into a floor plane to showcase transformations and casted shadows
	STDOBJ cube = STDOBJ("OBJ/simpleCube.obj");

	cube.Translate({ 0.0f, -1.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	cube.Scale({ 10.0f, 0.05f, 10.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	cube.AddToQuadTree(&staticObjects);

	//---------------------------------------------------------------------//











	//-------------------------------Lights--------------------------------//

	AmbientLight ambientLight = AmbientLight({0.2f, 0.2f, 0.2f});

	sceneLights.push_back(&ambientLight);



	std::vector<LightBaseStaging*> stagingLights;
	SpotDirLightArray lightArray = SpotDirLightArray(4, 1000, shadowmapSingleRenderer, &stagingLights);
	sceneLights.push_back(&lightArray);

	
	DirectionalLightStaging testDirectionalLight = DirectionalLightStaging({ 0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f });

	testDirectionalLight.Translate({ 10.0f, 10.0f, 5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	testDirectionalLight.Rotate({ 45.0f, -135.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	testDirectionalLight.SetupShadowmapping(lightArray, 20.0f, 1.0f, 50.0f);
	testDirectionalLight.SetCastShadow(true);

	stagingLights.push_back(&testDirectionalLight);

	DirectionalLightStaging testDirectionalLight2 = DirectionalLightStaging({ 0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f });

	testDirectionalLight2.Translate({ -10.0f, 10.0f, 5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	testDirectionalLight2.Rotate({ 45.0f, 135.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);

	testDirectionalLight2.SetupShadowmapping(lightArray, 20.0f, 1.0f, 50.0f);
	testDirectionalLight2.SetCastShadow(true);

	stagingLights.push_back(&testDirectionalLight2);
	
	
	SpotLightStaging testSpotLight = SpotLightStaging({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 45.0f, 10.0f);

	testSpotLight.SetupShadowmapping(lightArray, 5.0f, 30.0f);
	testSpotLight.SetCastShadow(true);
	testSpotLight.Rotate({ 15.0f, 30.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	testSpotLight.Translate({ -5.0f, 6.0f, -7.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	
	stagingLights.push_back(&testSpotLight);

	SpotLightStaging testSpotLight2 = SpotLightStaging({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 45.0f, 10.0f);

	testSpotLight2.SetupShadowmapping(lightArray, 5.0f, 30.0f);
	testSpotLight2.SetCastShadow(true);
	testSpotLight2.Rotate({ 15.0f, -30.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	testSpotLight2.Translate({ 5.0f, 6.0f, -7.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	stagingLights.push_back(&testSpotLight2);


	PointLight testPointLight = PointLight({0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f}, 10.0f);

	testPointLight.CreateShadowMap(500, 2.0f, 70.0f, &shadowmapCubeRenderer);

	testPointLight.Translate({ 7.0f, 10.0f, -5.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	testPointLight.SetCastShadows(true);

	//sceneLights.push_back(&testPointLight);
	
	//----------------------------------------------------------------------//






	//------------------------update loop-----------------------------------//

	std::chrono::steady_clock timer;
	std::chrono::time_point<std::chrono::steady_clock> previous = timer.now();

	bool key3Hold = false;
	bool secondaryCamera = false;

	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Pipeline::Clean::UnorderedAccessView(backbufferUAV);
		mainRenderer.CameraDeferredRender(&mainCamera, backbufferUAV);
		
		if (secondaryCamera)
		{
			mainRenderer.CameraDeferredRender(&quadtreeDemoCamera, backbufferUAV);
		}
		
		Pipeline::Switch();

		//frame counter used for lights to know if they have updated their shadowmap.
		Pipeline::IncrementCounter();

		std::chrono::time_point<std::chrono::steady_clock> now = timer.now();

		std::chrono::duration<double> deltaTime = now - previous;

		//fixed time delta update
		if (deltaTime.count() >= 1.0 / SceneStepRate)
		{
			previous = now;

			//Reflection render
			ico.ReflectionRender();

			CameraUpdate(mainCamera);
			galaxy.UpdateParticles();
			
			hugin.Rotate({ 0.0f, 1.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);
			quadtreeDemoCamera.Rotate({ 0.0f, 1.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_LOCAL, OBJECT_TRANSFORM_APPEND, OBJECT_ROTATION_UNIT_DEGREES);

			UINT input = GetInputTracker();
			if ((input & 0x100) > 0) //1 key
			{
				galaxy.RemoveParticles();
			}
			if ((input & 0x200) > 0) //2 key
			{
				galaxy.AddParticles();
			}
			if ((input & 0x400) > 0) //3 key
			{
				if (!key3Hold)
				{
					key3Hold = true;
					
					secondaryCamera = !secondaryCamera;
					mainCamera.SetDebug(!mainCamera.DebugOn());
				}
			}
			else
			{
				key3Hold = false;
			}
		}
	}
	//------------------------------------------------------------------------//





	//release resources
	for (STDOBJ* object : cornerCubes)
	{
		delete object;
	}

	Pipeline::Release();
	SharedResources::Release();
	backbufferUAV->Release();

	return 0;
}

void CameraUpdate(Camera& mainCamera)
{
	UINT input = GetInputTracker();
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
