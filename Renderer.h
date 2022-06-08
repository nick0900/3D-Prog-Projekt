#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

#include "Pipeline.h"
#include "BaseObject.h"
#include "Camera.h"

enum MappingMode
{
	SingleMap,
	DoubleMap,
	CubeMap
};

class Renderer
{
	public:
		Renderer(std::vector<Object*>* sceneObjects, std::vector<Camera*>* renderCameras);
		~Renderer();

		void Render();

		void Switch();

	private:
		bool DeferredSetup();
		void CameraDeferredRender(Camera* renderView);

		void CameraDepthMapRender(ID3D11DepthStencilView** mapDSV, ID3D11Buffer* viewBuffer, MappingMode mode);

		void ShadowMapUpdate();
		void ReflectionUpdate();

		bool CreateBackBufferViews();

		ID3D11RenderTargetView* backBufferRTV;
		ID3D11UnorderedAccessView* backBufferUAV;
		ID3D11Texture2D* dsTexture;
		ID3D11DepthStencilView* dsView;
		ID3D11ShaderResourceView* depthSRV;

		//Position is reconstructed from depthstencil
		//first buffer contain normals in first three elements and the shinynessfactor as the fourth. shinyness needs to be reinterpered as int for use
		//last three buffers contain ambient, diffuse and specular values respectively in the first trhee elements. each buffer contain one part of the color in their fourth element.
		ID3D11Texture2D* gBufferNormal;
		ID3D11Texture2D* gBufferAmbient;
		ID3D11Texture2D* gBufferDiffuse;
		ID3D11Texture2D* gBufferSpecular;

		ID3D11RenderTargetView* normalRTV;
		ID3D11RenderTargetView* ambientRTV;
		ID3D11RenderTargetView* diffuseRTV;
		ID3D11RenderTargetView* specularRTV;

		ID3D11ShaderResourceView* normalSRV;
		ID3D11ShaderResourceView* ambientSRV;
		ID3D11ShaderResourceView* diffuseSRV;
		ID3D11ShaderResourceView* specularSRV;

		//Camera shadowMapView;
		//Camera environmentMapView;

		std::vector<Camera*>* renderCameras;

		std::vector<LightBase*>* sceneLights;

		std::vector<Object*>* sceneObjects;
};