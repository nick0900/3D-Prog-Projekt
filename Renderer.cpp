#include <Windows.h>
#include <iostream>
#include <DirectXCollision.h>

#include "Renderer.h"

#include "BaseObject.h"
#include "Pipeline.h"
#include "SharedResources.h"
#include "Lights.h"


DepthRenderer::DepthRenderer(std::vector<Object*>* dynamicObjects, QuadTree** staticObjects) : dynamicObjects(dynamicObjects), staticObjects(staticObjects)
{
}

void DepthRenderer::CameraDepthRender(ID3D11DepthStencilView* dsv, Camera* view)
{
	Pipeline::ShadowMapping::ClearPixelShader();

	view->SetActiveCamera();

	Pipeline::Clean::DepthStencilView(dsv);
	Pipeline::ShadowMapping::BindDepthStencil(dsv);

	for (Object* object : *dynamicObjects)
	{
		object->DepthRender();
	}

	DirectX::BoundingFrustum viewFrustum;
	view->ViewFrustum(viewFrustum);
	std::vector<Object*> containedStaticObjects;
	(*staticObjects)->GetContainedInFrustum(&viewFrustum, containedStaticObjects);
	for (Object* object : containedStaticObjects)
	{
		object->DepthRender();
	}

	Pipeline::ShadowMapping::UnbindDepthStencil();
}

OmniDistanceRenderer::OmniDistanceRenderer(UINT resolution, std::vector<Object*>* dynamicObjects, QuadTree** staticObjects) : resolution(resolution), dynamicObjects(dynamicObjects), staticObjects(staticObjects)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &dsTexture)))
	{
		std::cerr << "Failed to set up depth stencil texture" << std::endl;
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
		std::cerr << "Error: Failed to set up DSV" << std::endl;
	}
}

OmniDistanceRenderer::~OmniDistanceRenderer()
{
	dsTexture->Release();
	dsView->Release();
}

void OmniDistanceRenderer::OmniDistanceRender(ID3D11RenderTargetView* rtv[6], Camera* view)
{
	view->Rotate({ 0.0f, 90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[0], view);

	view->Rotate({ 0.0f, -90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[1], view);

	view->Rotate({ -90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[2], view);

	view->Rotate({ 90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[3], view);

	view->Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[4], view);

	view->Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	CameraDistanceRender(rtv[5], view);
}

UINT OmniDistanceRenderer::Resolution()
{
	return resolution;
}

void OmniDistanceRenderer::CameraDistanceRender(ID3D11RenderTargetView* rtv, Camera* view)
{
	view->SetActiveCamera();

	Pipeline::Clean::DepthStencilView(dsView);
	Pipeline::ShadowMapping::BindDistanceBuffer(rtv, dsView);

	for (Object* object : *dynamicObjects)
	{
		object->DepthRender();
	}

	DirectX::BoundingFrustum viewFrustum;
	view->ViewFrustum(viewFrustum);
	std::vector<Object*> containedStaticObjects;
	(*staticObjects)->GetContainedInFrustum(&viewFrustum, containedStaticObjects);
	for (Object* object : containedStaticObjects)
	{
		object->DepthRender();
	}

	Pipeline::ShadowMapping::UnbindDistanceBuffer();
}

DeferredRenderer::DeferredRenderer(UINT widthRes, UINT heightRes, std::vector<Object*>* dynamicObjects, QuadTree** staticObjects, std::vector<LightBase*>* sceneLights, std::vector<ParticleSystem*>* particles) :
widthRes(widthRes),
heightRes(heightRes),
dynamicObjects(dynamicObjects),
staticObjects(staticObjects),
sceneLights(sceneLights),
particles(particles)
{
	CreateDepthStencil();
	DeferredSetup();
}

DeferredRenderer::~DeferredRenderer()
{
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

void DeferredRenderer::CameraDeferredRender(Camera* renderView, ID3D11UnorderedAccessView* targetUAV)
{
	renderView->SetActiveCamera();

	Pipeline::Clean::DepthStencilView(dsView);
	Pipeline::Clean::RenderTargetView(normalRTV);
	Pipeline::Clean::RenderTargetView(ambientRTV);
	Pipeline::Clean::RenderTargetView(diffuseRTV);
	Pipeline::Clean::RenderTargetView(specularRTV);

	Pipeline::Deferred::GeometryPass::PixelShader::Bind::GBuffers(normalRTV, ambientRTV, diffuseRTV, specularRTV, dsView);

	for (Object* object : *dynamicObjects)
	{
		object->Render();
	}

	DirectX::BoundingFrustum viewFrustum;
	renderView->ViewFrustum(viewFrustum);
	std::vector<Object*> containedStaticObjects;
	(*staticObjects)->GetContainedInFrustum(&viewFrustum, containedStaticObjects);
	for (Object* object : containedStaticObjects)
	{
		object->Render();
	}

	for (ParticleSystem* partSys : *particles)
	{
		partSys->Render();
	}

	Pipeline::Deferred::GeometryPass::PixelShader::Clear::GBuffers();

	Pipeline::Clean::UnorderedAccessView(targetUAV);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::BackBufferUAV(targetUAV);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::DepthBuffer(depthSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::NormalBuffer(normalSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::AmbientBuffer(ambientSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::DiffuesBuffer(diffuseSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::SpecularBuffer(specularSRV);

	for (LightBase* light : *sceneLights)
	{
		light->Bind();
		renderView->SetActiveCamera();
		Pipeline::Deferred::LightPass::ComputeShader::Settings::BindBuffer();
		Pipeline::Deferred::LightPass::ComputeShader::Dispatch32X32(renderView->ViewportWidth(), renderView->ViewportHeight(), renderView->ViewportTopLeftX(), renderView->ViewportTopLeftY());
	}

	Pipeline::Deferred::LightPass::ComputeShader::ColorDispatch32X32(renderView->ViewportWidth(), renderView->ViewportHeight(), renderView->ViewportTopLeftX(), renderView->ViewportTopLeftY());

	Pipeline::Deferred::LightPass::ComputeShader::Clear::ComputeSRVs();
	Pipeline::Deferred::LightPass::ComputeShader::Clear::TargetUAV();
}

void DeferredRenderer::OmniCameraDeferredRender(Camera* renderView, ID3D11UnorderedAccessView* targetUAVs[6])
{
	renderView->Rotate({ 0.0f, 90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[0]);

	renderView->Rotate({ 0.0f, -90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[1]);

	renderView->Rotate({ -90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[2]);

	renderView->Rotate({ 90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[3]);

	renderView->Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[4]);

	renderView->Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	renderView->UpdateTransformBuffer();

	CameraDeferredRender(renderView, targetUAVs[5]);
}

bool DeferredRenderer::DeferredSetup()
{
	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = widthRes;
	textureDesc.Height = heightRes;
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

bool DeferredRenderer::CreateDepthStencil()
{
	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = widthRes;
	textureDesc.Height = heightRes;
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
