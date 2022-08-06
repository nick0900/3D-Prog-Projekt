#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <array>

#include "Pipeline.h"
#include "BaseObject.h"
#include "Camera.h"
#include "Lights.h"
#include "QuadTree.h"
#include "ParticleSystems.h"

class DepthRenderer
{
public:
	DepthRenderer(std::vector<Object*>* dynamicObjects, QuadTree** staticObjects);

	void CameraDepthRender(ID3D11DepthStencilView* dsv, Camera* view);

private:
	std::vector<Object*>* dynamicObjects;
	QuadTree** staticObjects;
};

class OmniDistanceRenderer
{
public:
	OmniDistanceRenderer(UINT resolution, std::vector<Object*>* dynamicObjects, QuadTree** staticObjects);
	virtual ~OmniDistanceRenderer();

	void OmniDistanceRender(ID3D11RenderTargetView* rtv[6], Camera* view);

	UINT Resolution();

private:
	UINT resolution;

	ID3D11Texture2D* dsTexture;
	ID3D11DepthStencilView* dsView;

	std::vector<Object*>* dynamicObjects;
	QuadTree** staticObjects;

	void CameraDistanceRender(ID3D11RenderTargetView* rtv, Camera* view);
};

class DeferredRenderer
{
	public:
		DeferredRenderer(UINT widthRes, UINT heightRes, std::vector<Object*>* dynamicObjects, QuadTree** staticObjects, std::vector<LightBase*>* sceneLights, std::vector<ParticleSystem*>* particles);
		~DeferredRenderer();

		void CameraDeferredRender(Camera* renderView, ID3D11UnorderedAccessView* targetUAV);
		void OmniCameraDeferredRender(Camera* renderView, ID3D11UnorderedAccessView* targetUAVs[6]);

	private:
		bool DeferredSetup();

		bool CreateDepthStencil();

		UINT widthRes;
		UINT heightRes;

		std::vector<LightBase*>* sceneLights;

		std::vector<Object*>* dynamicObjects;

		std::vector<ParticleSystem*>* particles;

		QuadTree** staticObjects;

		ID3D11Texture2D* dsTexture;
		ID3D11DepthStencilView* dsView;
		ID3D11ShaderResourceView* depthSRV;

		//Position is reconstructed from depthstencil
		//first buffer contain normals in first three elements and the shinynessfactor as the fourth.
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
};
