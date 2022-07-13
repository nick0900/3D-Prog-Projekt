#include "Lights.h"
#include <iostream>
#include "Renderer.h"

#include "Pipeline.h"

Shadowmap::Shadowmap(UINT resolution) : resolution(resolution)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	Texture2DDesc(textureDesc);
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &mapTexture)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	SRVDesc(srvDesc);

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(mapTexture, &srvDesc, &mapSRV)))
	{
		std::cerr << "Failed to set up own shadow map SRV" << std::endl;
	}
}

Shadowmap::~Shadowmap()
{
	mapTexture->Release();
	mapSRV->Release();
}

int Shadowmap::Resolution()
{
	return resolution;
}

LightBase::LightBase() : castShadows(false), shadowmap(nullptr) {}

LightBase::~LightBase()
{
	DeleteShadowMap();
}

void LightBase::GetParameters(LightBufferStruct* dest)
{
	*dest = LightBufferStruct(Type(), LightFOV(), LightFalloff(), lightDiffuse, lightSpecular, LightPosition(), LightDirection(), InverseTransformMatrix(), ProjMatrix());
	if (castShadows && (shadowmap != nullptr))
	{
		dest->mappingMode = shadowmap->ShadowmapMappingMode();
	}
}



ID3D11DepthStencilView** LightBase::ShadowmapDSVs()
{
	if (CastShadows()) return shadowmap->DSVArray();
	return nullptr;
}

MappingMode LightBase::ShadowMappingMode()
{
	if (CastShadows()) return shadowmap->ShadowmapMappingMode();
	return MappingMode::SingleMap;
}

void LightBase::SetupShadowmap(MappingMode mapType, UINT resolution)
{
	switch (mapType)	
	{
	case MappingMode::SingleMap:
		shadowmap = new ShadowMapSingle(resolution);
		break;

	case MappingMode::DoubleMap:
		shadowmap = new ShadowMapDouble(resolution);
		break;

	case MappingMode::CubeMap:
		shadowmap = new ShadowMapCube(resolution);
		break;
	}
}

void LightBase::DeleteShadowMap()
{
	if (shadowmap != nullptr)
	{
		delete shadowmap;
		shadowmap = nullptr;
	}
}

bool LightBase::CastShadows()
{
	return castShadows;
}

void LightBase::SetCastShadows(bool castShadows)
{
	this->castShadows = (shadowmap != nullptr) && castShadows;
}

ID3D11ShaderResourceView* LightBase::ShadowmapSRV()
{
	if (castShadows)
	{
		return shadowmap->mapSRV;
	}
	return nullptr;
}

LightBinder::LightBinder() : paramCapacity(SMALLEST_PARAM_CAPACITY)
{
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(GeneralLightInfoBuffer);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &LightsGeneralData)))
	{
		std::cerr << "Error: failed to create light binder general light info buffer" << std::endl;
	}

	paramsStructuredBuffer = AllocateParamBuffer(paramCapacity);
}

LightBinder::~LightBinder()
{
	LightsGeneralData->Release();
	paramsStructuredBuffer->Release();
}

void LightBinder::Bind()
{
	if ((boundLights.size() >= paramCapacity) || ((paramCapacity > SMALLEST_PARAM_CAPACITY) && (boundLights.size() < paramCapacity / 2)))
	{
		while (boundLights.size() >= paramCapacity)
		{
			paramCapacity *= 2;
		}
		while ((paramCapacity > SMALLEST_PARAM_CAPACITY) && (boundLights.size() < paramCapacity / 2))
		{
			paramCapacity /= 2;
		}

		paramsStructuredBuffer->Release();
		paramsStructuredBuffer = AllocateParamBuffer(paramCapacity);
	}
	
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	GeneralLightInfoBuffer infoBuffer[1] = { GeneralLightInfoBuffer(boundLights.size()) };

	Pipeline::ResourceManipulation::MapBuffer(LightsGeneralData, &mappedResource);
	memcpy(mappedResource.pData, infoBuffer, sizeof(GeneralLightInfoBuffer));
	Pipeline::ResourceManipulation::UnmapBuffer(LightsGeneralData);
	
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightGeneralInfoBuffer(LightsGeneralData);


	Pipeline::ResourceManipulation::MapBuffer(paramsStructuredBuffer, &mappedResource);
	
	LightBufferStruct* copyStager = new LightBufferStruct[boundLights.size()];
	std::vector<ID3D11ShaderResourceView*> shadowmapSRVs;

	for (int i = 0; i < boundLights.size(); i++)
	{
		boundLights[i]->GetParameters(copyStager + i);

		if (boundLights[i]->CastShadows())
		{
			copyStager[i].shadowMapIndex = shadowmapSRVs.size();
			shadowmapSRVs.push_back(boundLights[i]->ShadowmapSRV());
		}
	}

	memcpy(mappedResource.pData, copyStager, sizeof(LightBufferStruct) * boundLights.size());
	delete[] copyStager;
	Pipeline::ResourceManipulation::UnmapBuffer(paramsStructuredBuffer);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowmapResources(shadowmapSRVs.data(), shadowmapSRVs.size());
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightParameterStructuredBuffer(paramsStructuredBuffer);
}

void LightBinder::AddLight(LightBase* light)
{
	boundLights.push_back(light);
}

void LightBinder::ClearLights()
{
	boundLights.clear();
}

ID3D11Buffer* LightBinder::AllocateParamBuffer(UINT capacity)
{
	ID3D11Buffer* temp;

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = capacity * sizeof(LightBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(LightBufferStruct);

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &temp)))
	{
		return nullptr;
	}
	return temp;
}

PointLight::PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float fallof) : fallof(fallof)
{
	for (int i = 0; i < 3; i++)
	{
		lightDiffuse[i] = diffuse[i];
		lightSpecular[i] = specular[i];
	}
}

DirectX::XMFLOAT4X4 PointLight::TransformMatrix()
{
	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

DirectX::XMFLOAT4X4 PointLight::InverseTransformMatrix()
{
	DirectX::XMMATRIX invScaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX invRotation = DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invScaling * invRotation * invTranslation));

	return output;
}

Camera* PointLight::ShadowMapCamera()
{
	if (!CastShadows) return;

	CameraPerspective* outputCamera = nullptr;

	switch (shadowmap->ShadowmapMappingMode())
	{
	case MappingMode::SingleMap:
		outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, 360.0f, nearPlane, farPlane);
		outputCamera->Rotate({ 90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
		outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
		break;

	case MappingMode::DoubleMap:
		outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, 180.0f, nearPlane, farPlane);
		outputCamera->Rotate({ 90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
		outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
		break;

	case MappingMode::CubeMap:
		outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, 90.0f, nearPlane, farPlane);
		outputCamera->Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
		outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
		break;
	}

	return outputCamera;
}

void PointLight::CreateShadowMap(MappingMode mapType, UINT resolution, float nearPlane, float farPlane)
{
	SetupShadowmap(mapType, resolution);
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

LightType PointLight::Type()
{
	return LightType::PointLight;
}

float PointLight::LightFOV()
{
	return 0.0f;
}

float PointLight::LightFalloff()
{
	return fallof;
}

std::array<float, 3> PointLight::LightDirection()
{
	return {0.0f, 0.0f, 0.0f};
}

std::array<float, 3> PointLight::LightPosition()
{
	return {position.x, position.y, position.z};
}

void PointLight::Render()
{
}

void PointLight::DepthRender()
{
}

DirectX::XMFLOAT4X4 PointLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output;

	if (CastShadows())
	{
		float fov = 0;
		switch (shadowmap->ShadowmapMappingMode())
		{
		case MappingMode::SingleMap:
			fov = 360.0f;
			break;

		case MappingMode::DoubleMap:
			fov = 180.0f;
			break;

		case MappingMode::CubeMap:
			fov = 90.0f;
		}
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixPerspectiveFovLH(fov, 1.0f, nearPlane, farPlane));
	}
	
	return output;
}

DirectionalLight::DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular)
{
	for (int i = 0; i < 3; i++)
	{
		lightDiffuse[i] = diffuse[i];
		lightSpecular[i] = specular[i];
	}
}

DirectX::XMFLOAT4X4 DirectionalLight::TransformMatrix()
{
	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

void DirectionalLight::Render()
{
}

void DirectionalLight::DepthRender()
{
}

DirectX::XMFLOAT4X4 DirectionalLight::InverseTransformMatrix()
{
	DirectX::XMMATRIX invScaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(-90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(rotation);

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invScaling * invRotation * invTranslation));

	return output;
}

LightType DirectionalLight::Type()
{
	return LightType::DirectionalLight;
}

float DirectionalLight::LightFOV()
{
	return 0.0f;
}

float DirectionalLight::LightFalloff()
{
	return 0.0f;
}

Camera* DirectionalLight::ShadowMapCamera()
{
	if (!CastShadows) return;

	CameraOrthographic* outputCamera = new CameraOrthographic(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, viewWidth, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMFLOAT4X4 inputRotation;
	DirectX::XMStoreFloat4x4(&inputRotation, rotation);
	outputCamera->Rotate(inputRotation, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void DirectionalLight::CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane)
{
	SetupShadowmap(MappingMode::SingleMap, resolution);
	this->viewWidth = viewWidth;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

std::array<float, 3> DirectionalLight::LightDirection()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);
	return {output.x, output.y, output.z};
}

std::array<float, 3> DirectionalLight::LightPosition()
{
	return {0.0f, 0.0f, 0.0f};
}

DirectX::XMFLOAT4X4 DirectionalLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output;

	if (CastShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixOrthographicLH(viewWidth, viewWidth, nearPlane, farPlane));
	}

	return output;
}



SpotLight::SpotLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof) : lightFOV(lightFOV), fallof(fallof)
{
	for (int i = 0; i < 3; i++)
	{
		lightDiffuse[i] = diffuse[i];
		lightSpecular[i] = specular[i];
	}
}

LightType SpotLight::Type()
{
	return LightType::SpotLight;
}

DirectX::XMFLOAT4X4 SpotLight::TransformMatrix()
{
	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

DirectX::XMFLOAT4X4 SpotLight::InverseTransformMatrix()
{
	DirectX::XMMATRIX invScaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(rotation);

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invScaling * invRotation * invTranslation));

	return output;
}

float SpotLight::LightFOV()
{
	return lightFOV;
}

float SpotLight::LightFalloff()
{
	return fallof;
}

std::array<float, 3> SpotLight::LightDirection()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);
	return { output.x, output.y, output.z };
}

std::array<float, 3> SpotLight::LightPosition()
{
	return { position.x, position.y, position.z };
}

Camera* SpotLight::ShadowMapCamera()
{
	if (!CastShadows) return;

	CameraPerspective* outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, lightFOV, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMFLOAT4X4 inputRotation;
	DirectX::XMStoreFloat4x4(&inputRotation, rotation);
	outputCamera->Rotate(inputRotation, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void SpotLight::Render()
{
}

void SpotLight::DepthRender()
{
}

DirectX::XMFLOAT4X4 SpotLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output;

	if (CastShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixPerspectiveFovLH(lightFOV, 1.0f, nearPlane, farPlane));
	}

	return output;
}

void SpotLight::CreateShadowMap(UINT resolution, float nearPlane, float farPlane)
{
	SetupShadowmap(MappingMode::SingleMap, resolution);
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

ShadowMapSingle::ShadowMapSingle(UINT resolution) : Shadowmap(resolution)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_DSV texDSV;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2D = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(mapTexture, &dsvDesc, &mapDSV)))
	{
		std::cerr << "Error: Failed to set up shadowmap DSV" << std::endl;
	}
}

ShadowMapSingle::~ShadowMapSingle()
{
	mapDSV->Release();
}

MappingMode ShadowMapSingle::ShadowmapMappingMode()
{
	return MappingMode::SingleMap;
}

ID3D11DepthStencilView** ShadowMapSingle::DSVArray()
{
	return &mapDSV;
}

void ShadowMapSingle::Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc)
{
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
}

void ShadowMapSingle::SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	D3D11_TEX2D_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2D = texSRV;
}

ShadowMapDouble::ShadowMapDouble(UINT resolution) : Shadowmap(resolution)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_ARRAY_DSV texDSV;
	texDSV.ArraySize = 1;
	texDSV.FirstArraySlice = 0;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2DArray = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(mapTexture, &dsvDesc, &mapDSV[0])))
	{
		std::cerr << "Error: Failed to set up shadowmap DSVs" << std::endl;
	}

	dsvDesc.Texture2DArray.FirstArraySlice = 1;
	if (FAILED(Pipeline::Device()->CreateDepthStencilView(mapTexture, &dsvDesc, &mapDSV[1])))
	{
		std::cerr << "Error: Failed to set up shadowmap DSVs" << std::endl;
	}
}

ShadowMapDouble::~ShadowMapDouble()
{
	mapDSV[0]->Release();
	mapDSV[1]->Release();
}

MappingMode ShadowMapDouble::ShadowmapMappingMode()
{
	return MappingMode::DoubleMap;
}

ID3D11DepthStencilView** ShadowMapDouble::DSVArray()
{
	return mapDSV;
}

void ShadowMapDouble::Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc)
{
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 2;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
}

void ShadowMapDouble::SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_SRV texSRV;
	texSRV.ArraySize = 2;
	texSRV.FirstArraySlice = 0;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2DArray = texSRV;
}

ShadowMapCube::ShadowMapCube(UINT resolution) : Shadowmap(resolution)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_ARRAY_DSV texDSV;
	texDSV.ArraySize = 1;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2DArray = texDSV;
	
	for (int i = 0; i < 6; i++)
	{
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		if (FAILED(Pipeline::Device()->CreateDepthStencilView(mapTexture, &dsvDesc, &mapDSV[i])))
		{
			std::cerr << "Error: Failed to set up shadowmap DSVs" << std::endl;
		}
	}
}

ShadowMapCube::~ShadowMapCube()
{
	for (int i = 0; i < 6; i++)
	{
		mapDSV[i]->Release();
	}
}

MappingMode ShadowMapCube::ShadowmapMappingMode()
{
	return MappingMode::CubeMap;
}

ID3D11DepthStencilView** ShadowMapCube::DSVArray()
{
	return mapDSV;
}

void ShadowMapCube::Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc)
{
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 6;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
}

void ShadowMapCube::SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_SRV texSRV;
	texSRV.ArraySize = 6;
	texSRV.FirstArraySlice = 0;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2DArray = texSRV;
}
