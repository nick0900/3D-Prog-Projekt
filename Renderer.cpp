#include <Windows.h>
#include <iostream>

#include "Renderer.h"

#include "BaseObject.h"
#include "Pipeline.h"
#include "SharedResources.h"
#include "Lights.h"



Renderer::Renderer(std::vector<Object*>* sceneObjects, std::vector<Camera*>* renderCameras, std::vector<LightBase*>* sceneLights) :
	backBufferRTV(nullptr), 
	backBufferUAV(nullptr), 
	dsTexture(nullptr), 
	dsView(nullptr), 
	depthSRV(nullptr),
	
	gBufferNormal(nullptr), 
	gBufferAmbient(nullptr),
	gBufferDiffuse(nullptr),
	gBufferSpecular(nullptr),

	normalRTV(nullptr),
	ambientRTV(nullptr),
	diffuseRTV(nullptr),
	specularRTV(nullptr),

	normalSRV(nullptr),
	ambientSRV(nullptr),
	diffuseSRV(nullptr),
	specularSRV(nullptr),

	renderCameras(renderCameras),
	sceneObjects(sceneObjects),
	sceneLights(sceneLights)
{
	CreateBackBufferViews();
	DeferredSetup();
}

Renderer::~Renderer()
{
	backBufferRTV->Release();
	backBufferUAV->Release();
	dsTexture->Release();
	dsView->Release();
	depthSRV->Release();

	gBufferNormal->Release();
	gBufferAmbient->Release();
	gBufferDiffuse->Release();
	gBufferSpecular->Release();

	normalRTV->Release();
	ambientRTV->Release();
	diffuseRTV->Release();
	specularRTV->Release();

	normalSRV->Release();
	ambientSRV->Release();
	diffuseSRV->Release();
	specularSRV->Release();
}

void Renderer::Render()
{
	ReflectionUpdate();

	for (Camera* camera : *renderCameras)
	{
		CameraDeferredRender(camera);
	}
}

void Renderer::Switch()
{
	Pipeline::Switch();
	Pipeline::IncrementCounter();
}

void Renderer::CameraDepthRender(ID3D11DepthStencilView* dsv, Camera* view)
{
	Pipeline::ShadowMapping::ClearPixelShader();

	view->SetActiveCamera();

	Pipeline::ShadowMapping::BindDepthStencil(dsv);

	for (Object* object : *sceneObjects)
	{
		object->DepthRender();
	}
}

bool Renderer::DeferredSetup()
{
	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = Pipeline::BackBufferWidth();
	textureDesc.Height = Pipeline::BackBufferHeight();
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;



	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &gBufferNormal)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateRenderTargetView(gBufferNormal, nullptr, &normalRTV)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(gBufferNormal, nullptr, &normalSRV)))
	{
		return false;
	}
	


	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &gBufferAmbient)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateRenderTargetView(gBufferAmbient, nullptr, &ambientRTV)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(gBufferAmbient, nullptr, &ambientSRV)))
	{
		return false;
	}



	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &gBufferDiffuse)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateRenderTargetView(gBufferDiffuse, nullptr, &diffuseRTV)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(gBufferDiffuse, nullptr, &diffuseSRV)))
	{
		return false;
	}



	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &gBufferSpecular)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateRenderTargetView(gBufferSpecular, nullptr, &specularRTV)))
	{
		return false;
	}

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(gBufferSpecular, nullptr, &specularSRV)))
	{
		return false;
	}

	return true;
}

void Renderer::CameraDeferredRender(Camera* renderView)
{
	renderView->SetActiveCamera();

	Pipeline::Deferred::GeometryPass::Clear::DepthStencilView(dsView);
	Pipeline::Deferred::GeometryPass::PixelShader::Bind::GBuffers(normalRTV, ambientRTV, diffuseRTV, specularRTV, dsView);

	for (Object* object : *sceneObjects)
	{
		object->Render();
	}

	Pipeline::Deferred::GeometryPass::PixelShader::Clear::GBuffers();

	Pipeline::Deferred::LightPass::ComputeShader::Bind::DepthBuffer(depthSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::NormalBuffer(normalSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::AmbientBuffer(ambientSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::DiffuesBuffer(diffuseSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::SpecularBuffer(specularSRV);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::BackBufferUAV(backBufferUAV);

	for (LightBase* light : *sceneLights)
	{
		light->Bind();
		Pipeline::Deferred::LightPass::ComputeShader::Dispatch32X18(renderView->ViewportWidth(), renderView->ViewportHeight(), renderView->ViewportTopLeftX(), renderView->ViewportTopLeftY());
	}

	Pipeline::Deferred::LightPass::ComputeShader::Clear::ComputeSRVs();
}

void Renderer::ReflectionUpdate()
{
}

bool Renderer::CreateBackBufferViews()
{
	if (!Pipeline::GetBackbufferRTV(backBufferRTV))
	{
		std::cerr << "Failed to create backbuffer render target view!" << std::endl;
		return false;
	}

	if (!Pipeline::GetBackbufferUAV(backBufferUAV))
	{
		std::cerr << "Failed to create backbuffer unordered access view!" << std::endl;
		return false;
	}

	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = Pipeline::BackBufferWidth();
	textureDesc.Height = Pipeline::BackBufferHeight();
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &dsTexture)))
	{
		std::cerr << "Failed to create depth stencil texture!" << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_DSV texDSV;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2D = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(dsTexture, &dsvDesc, &dsView)))
	{
		std::cerr << "Failed to create depth stencil view!" << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	D3D11_TEX2D_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2D = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(dsTexture, &srvDesc, &depthSRV)))
	{
		std::cerr << "Failed to create depth SRV!" << std::endl;
		return false;
	}
	return true;
}
