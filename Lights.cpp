#include "Lights.h"
#include <iostream>
#include "Renderer.h"

#include "Pipeline.h"


LightBase::LightBase(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) : lightDiffuse(diffuse), lightSpecular(specular), castShadows(false), shadowmap(nullptr), parametersModifyed(false), parameterBuffer(nullptr)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create light transform Buffer" << std::endl;
	}
}

LightBase::~LightBase()
{
	DeleteShadowMap();
}

void LightBase::SetDiffuse(std::array<float, 3> lightDiffuse)
{
	for (int i = 0; i < 3; i++)
	{
		this->lightDiffuse[i] = lightDiffuse[i];
	}
	parametersModifyed = true;
}

std::array<float, 3> LightBase::Diffuse()
{
	return lightDiffuse;
}

void LightBase::SetSpecular(std::array<float, 3> lightSpecular)
{
	for (int i = 0; i < 3; i++)
	{
		this->lightSpecular[i] = lightSpecular[i];
	}
	parametersModifyed = true;
}

std::array<float, 3> LightBase::Specular()
{
	return lightSpecular;
}

void LightBase::Bind()
{
	if (parametersModifyed)
	{
		UpdateParameterBuffer();

		parametersModifyed = false;
	}

	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightParameterBuffer(parameterBuffer);

	if (castShadows)
	{
		shadowmap->Bind();
	}

	SharedResources::BindComputeShader(LightShader());
}

void LightBase::DeleteShadowMap()
{
	if (shadowmap != nullptr)
	{
		delete shadowmap;
		shadowmap = nullptr;
	}
}

bool LightBase::CastingShadows()
{
	return castShadows;
}

void LightBase::SetCastShadows(bool castShadows)
{
	this->castShadows = (shadowmap != nullptr) && castShadows;
}

void LightBase::flagParameterChange()
{
	parametersModifyed = true;
}

void LightBase::OnModyfied()
{
	if (castShadows)
	{
		shadowmap->flagShadowChange();
	}
}



PointLight::PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float fallof) : falloff(fallof), LightBase(diffuse, specular), nearPlane(0), farPlane(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	PointLightBuffer bufferStruct = PointLightBuffer(falloff, Diffuse(), Specular(), { position.x, position.y, position.z }, CastingShadows());

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &parameterBuffer)))
	{
		std::cerr << "Failed to create light parameter Buffer" << std::endl;
	}
}

SharedResources::cShader PointLight::LightShader()
{
	return CastingShadows() ? SharedResources::cShader::PointLightShadow : SharedResources::cShader::PointLight;
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
	if (!CastingShadows()) return nullptr;

	CameraPerspective* outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, 90.0f, nearPlane, farPlane);
	outputCamera->Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void PointLight::SetFalloff(float falloff)
{
	this->falloff = falloff;
	flagParameterChange();
}

void PointLight::CreateShadowMap(UINT resolution, float nearPlane, float farPlane, Renderer* renderer)
{
	DeleteShadowMap();
	shadowmap = new ShadowMapCube(resolution, this, renderer);
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

void PointLight::Render()
{
}

void PointLight::DepthRender()
{
}

DirectX::XMFLOAT4X4 PointLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output = DirectX::XMFLOAT4X4();

	if (CastingShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixPerspectiveFovLH(90.0f, 1.0f, nearPlane, farPlane));
	}
	
	return output;
}

void PointLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	PointLightBuffer bufferStruct = PointLightBuffer(falloff, Diffuse(), Specular(), { position.x, position.y, position.z }, CastingShadows());

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}

DirectionalLight::DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) : LightBase(diffuse, specular), nearPlane(0), farPlane(0), viewWidth(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	DirectionalLightBuffer bufferStruct = DirectionalLightBuffer(Diffuse(), Specular(), { output.x, output.y, output.z }, CastingShadows());

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &parameterBuffer)))
	{
		std::cerr << "Failed to create light parameter Buffer" << std::endl;
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

Camera* DirectionalLight::ShadowMapCamera()
{
	if (!CastingShadows()) return nullptr;

	CameraOrthographic* outputCamera = new CameraOrthographic(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, viewWidth, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMFLOAT4X4 inputRotation;
	DirectX::XMStoreFloat4x4(&inputRotation, rotation);
	outputCamera->Rotate(inputRotation, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void DirectionalLight::CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane, Renderer* renderer)
{
	DeleteShadowMap();
	shadowmap = new ShadowMapSingle(resolution, this, renderer);
	this->viewWidth = viewWidth;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

SharedResources::cShader DirectionalLight::LightShader()
{
	return CastingShadows() ? SharedResources::cShader::DirectionalLightShadow : SharedResources::cShader::DirectionalLight;
}

DirectX::XMFLOAT4X4 DirectionalLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output = DirectX::XMFLOAT4X4();

	if (CastingShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixOrthographicLH(viewWidth, viewWidth, nearPlane, farPlane));
	}

	return output;
}

void DirectionalLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	DirectionalLightBuffer bufferStruct = DirectionalLightBuffer(Diffuse(), Specular(), { output.x, output.y, output.z }, CastingShadows());

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}



SpotLight::SpotLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof) : lightFOV(lightFOV), fallof(fallof), LightBase(diffuse, specular), nearPlane(0), farPlane(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotLightBuffer bufferStruct = SpotLightBuffer(lightFOV, fallof, Diffuse(), Specular(), { position.x, position.y, position.z }, { output.x, output.y, output.z }, CastingShadows());

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &parameterBuffer)))
	{
		std::cerr << "Failed to create light parameter Buffer" << std::endl;
	}
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

Camera* SpotLight::ShadowMapCamera()
{
	if (!CastingShadows()) return nullptr;

	CameraPerspective* outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, lightFOV, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;
	DirectX::XMFLOAT4X4 inputRotation;
	DirectX::XMStoreFloat4x4(&inputRotation, rotation);
	outputCamera->Rotate(inputRotation, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

SharedResources::cShader SpotLight::LightShader()
{
	return CastingShadows() ? SharedResources::cShader::SpotLightShadow : SharedResources::cShader::SpotLight;
}

void SpotLight::Render()
{
}

void SpotLight::DepthRender()
{
}

DirectX::XMFLOAT4X4 SpotLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output = DirectX::XMFLOAT4X4();

	if (CastingShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixPerspectiveFovLH(lightFOV, 1.0f, nearPlane, farPlane));
	}

	return output;
}

void SpotLight::SetFalloff(float falloff)
{
	this->fallof = falloff;
	flagParameterChange();
}

void SpotLight::CreateShadowMap(UINT resolution, float nearPlane, float farPlane, Renderer* renderer)
{
	DeleteShadowMap();
	shadowmap = new ShadowMapSingle(resolution, this, renderer);
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

void SpotLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);
	DirectX::XMVECTOR direction = { 0.0f, -1.0f, 0.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotLightBuffer bufferStruct = SpotLightBuffer(lightFOV, fallof, Diffuse(), Specular(), { position.x, position.y, position.z }, { output.x, output.y, output.z }, CastingShadows());

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}


Shadowmap::Shadowmap(UINT resolution, LightBase* linkedLight, Renderer* renderer) : resolution(resolution), linkedLight(linkedLight), renderer(renderer), mapSRV(nullptr), mapTexture(nullptr), shadowMappingBuffer(nullptr)
{
	latestRender = 0;
	mappingTransformed = true;
}

Shadowmap::~Shadowmap()
{
	mapTexture->Release();
	mapSRV->Release();
	shadowMappingBuffer->Release();
}

void Shadowmap::Bind()
{
	if (mappingTransformed)
	{
		ShadowMappingBuffer bufferData(linkedLight->InverseTransformMatrix(), linkedLight->ProjMatrix());

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		Pipeline::ResourceManipulation::MapBuffer(shadowMappingBuffer, &mappedResource);
		memcpy(mappedResource.pData, &bufferData, sizeof(ShadowMappingBuffer));
		Pipeline::ResourceManipulation::UnmapBuffer(shadowMappingBuffer);

		MapRender();

		latestRender = Pipeline::FrameCounter();
		mappingTransformed = false;
	}
	else if (latestRender != Pipeline::FrameCounter())
	{
		MapRender();
		latestRender = Pipeline::FrameCounter();
	}

	Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowmappingBuffer(shadowMappingBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowMapResource(mapSRV);
}

void Shadowmap::flagShadowChange()
{
	mappingTransformed = true;
}

int Shadowmap::Resolution()
{
	return resolution;
}

ShadowMapSingle::ShadowMapSingle(UINT resolution, LightBase* linkedLight, Renderer* renderer) : Shadowmap(resolution, linkedLight, renderer)
{
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &shadowMappingBuffer)))
	{
		std::cerr << "Failed to set up own shadowMapping buffer" << std::endl;
	}


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

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &mapTexture)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	D3D11_TEX2D_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2D = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(mapTexture, &srvDesc, &mapSRV)))
	{
		std::cerr << "Failed to set up own shadow map SRV" << std::endl;
	}

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

void ShadowMapSingle::MapRender()
{
}



ShadowMapCube::ShadowMapCube(UINT resolution, LightBase* linkedLight, Renderer* renderer) : Shadowmap(resolution, linkedLight, renderer)
{
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &shadowMappingBuffer)))
	{
		std::cerr << "Failed to set up own shadowMapping buffer" << std::endl;
	}


	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 6;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &mapTexture)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_SRV texSRV;
	texSRV.ArraySize = 6;
	texSRV.FirstArraySlice = 0;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.Texture2DArray = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(mapTexture, &srvDesc, &mapSRV)))
	{
		std::cerr << "Failed to set up own shadow map SRV" << std::endl;
	}

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

void ShadowMapCube::MapRender()
{
}
