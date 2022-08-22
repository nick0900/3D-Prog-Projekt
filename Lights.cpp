#include "Lights.h"
#include <iostream>
#include "Renderer.h"

#include "Pipeline.h"


SingularLight::SingularLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) : lightDiffuse(diffuse), lightSpecular(specular), castShadows(false), shadowmap(nullptr), parametersModifyed(false), parameterBuffer(nullptr)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create light transform Buffer" << std::endl;
	}
}

SingularLight::~SingularLight()
{
	DeleteShadowMap();
	parameterBuffer->Release();
	worldTransformBuffer->Release();
}

void SingularLight::SetDiffuse(std::array<float, 3> lightDiffuse)
{
	for (int i = 0; i < 3; i++)
	{
		this->lightDiffuse[i] = lightDiffuse[i];
	}
	parametersModifyed = true;
}

std::array<float, 3> SingularLight::Diffuse()
{
	return lightDiffuse;
}

void SingularLight::SetSpecular(std::array<float, 3> lightSpecular)
{
	for (int i = 0; i < 3; i++)
	{
		this->lightSpecular[i] = lightSpecular[i];
	}
	parametersModifyed = true;
}

std::array<float, 3> SingularLight::Specular()
{
	return lightSpecular;
}

void SingularLight::Bind()
{
	if (parametersModifyed)
	{
		UpdateParameterBuffer();

		parametersModifyed = false;
	}

	BindBuffer();

	if (castShadows)
	{
		shadowmap->Bind();
	}

	Pipeline::Deferred::LightPass::ComputeShader::Settings::Shadowcaster(castShadows);
}

void SingularLight::DeleteShadowMap()
{
	if (shadowmap != nullptr)
	{
		delete shadowmap;
		shadowmap = nullptr;
	}
}

bool SingularLight::CastingShadows()
{
	return castShadows;
}

void SingularLight::SetCastShadows(bool castShadows)
{
	this->castShadows = (shadowmap != nullptr) && castShadows;
}

void SingularLight::flagParameterChange()
{
	parametersModifyed = true;
}

void SingularLight::OnModyfied()
{
	if (castShadows)
	{
		shadowmap->flagShadowChange();
	}

	flagParameterChange();
}



PointLight::PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float fallof) : falloff(fallof), SingularLight(diffuse, specular), nearPlane(0), farPlane(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	PointLightBuffer bufferStruct = PointLightBuffer(falloff, Diffuse(), Specular(), { position.x, position.y, position.z });

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
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invTranslation * invRotation));

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

void PointLight::CreateShadowMap(UINT resolution, float nearPlane, float farPlane, OmniDistanceRenderer* renderer)
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
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(90.0f * OBJECT_ROTATION_UNIT_DEGREES, 1.0f, nearPlane, farPlane)));
	}
	
	return output;
}

void PointLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	PointLightBuffer bufferStruct = PointLightBuffer(falloff, Diffuse(), Specular(), { position.x, position.y, position.z });

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}

void PointLight::BindBuffer()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(LightShaderMode::LightTypePoint);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::PointLightParameterBuffer(parameterBuffer);
}

DirectionalLight::DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) : SingularLight(diffuse, specular), nearPlane(0), farPlane(0), viewWidth(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	DirectionalLightBuffer bufferStruct = DirectionalLightBuffer(Diffuse(), Specular(), { output.x, output.y, output.z });

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

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
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
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion)));

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invTranslation * invRotation));

	return output;
}

Camera* DirectionalLight::ShadowMapCamera()
{
	if (!CastingShadows()) return nullptr;

	CameraOrthographic* outputCamera = new CameraOrthographic(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, viewWidth, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	outputCamera->Rotate(rotationQuaternion, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void DirectionalLight::CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane, DepthRenderer* renderer)
{
	DeleteShadowMap();
	shadowmap = new ShadowMapSingle(resolution, this, renderer);
	this->viewWidth = viewWidth;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

DirectX::XMFLOAT4X4 DirectionalLight::ProjMatrix()
{
	DirectX::XMFLOAT4X4 output = DirectX::XMFLOAT4X4();

	if (CastingShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(viewWidth, viewWidth, nearPlane, farPlane)));
	}

	return output;
}

void DirectionalLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	DirectionalLightBuffer bufferStruct = DirectionalLightBuffer(Diffuse(), Specular(), { output.x, output.y, output.z });

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}

void DirectionalLight::BindBuffer()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(LightShaderMode::LightTypeDirectional);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::DirectionalLightParameterBuffer(parameterBuffer);
	Pipeline::ShadowMapping::BorderSampleWhite();
}



SpotLight::SpotLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof) : lightFOV(lightFOV), fallof(fallof), SingularLight(diffuse, specular), nearPlane(0), farPlane(0)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotLightBuffer bufferStruct = SpotLightBuffer(lightFOV, fallof, Diffuse(), Specular(), { position.x, position.y, position.z }, { output.x, output.y, output.z });

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

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

DirectX::XMFLOAT4X4 SpotLight::InverseTransformMatrix()
{
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion)));

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invTranslation * invRotation));

	return output;
}

Camera* SpotLight::ShadowMapCamera()
{
	if (!CastingShadows()) return nullptr;

	CameraPerspective* outputCamera = new CameraPerspective(shadowmap->Resolution(), shadowmap->Resolution(), 0, 0, lightFOV, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);
	
	outputCamera->Rotate(rotationQuaternion, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

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
	DirectX::XMFLOAT4X4 output = DirectX::XMFLOAT4X4();

	if (CastingShadows())
	{
		DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(lightFOV * OBJECT_ROTATION_UNIT_DEGREES, 1.0f, nearPlane, farPlane)));
	}

	return output;
}

void SpotLight::SetFOV(float lightFOV)
{
	this->lightFOV = lightFOV;
	flagParameterChange();
}

void SpotLight::SetFalloff(float falloff)
{
	this->fallof = falloff;
	flagParameterChange();
}

void SpotLight::CreateShadowMap(UINT resolution, float nearPlane, float farPlane, DepthRenderer* renderer)
{
	DeleteShadowMap();
	shadowmap = new ShadowMapSingle(resolution, this, renderer);
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}

void SpotLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f};
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotLightBuffer bufferStruct = SpotLightBuffer(lightFOV, fallof, Diffuse(), Specular(), { position.x, position.y, position.z }, { output.x, output.y, output.z });

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}

void SpotLight::BindBuffer()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(LightShaderMode::LightTypeSpot);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::SpotLightParameterBuffer(parameterBuffer);
	Pipeline::ShadowMapping::BorderSampleBlack();
}


Shadowmap::Shadowmap(UINT resolution, SingularLight* linkedLight) : resolution(resolution), linkedLight(linkedLight), mapSRV(nullptr), mapTexture(nullptr), shadowMappingBuffer(nullptr)
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
	BindMap();
}

void Shadowmap::flagShadowChange()
{
	mappingTransformed = true;
}

int Shadowmap::Resolution()
{
	return resolution;
}

ShadowMapSingle::ShadowMapSingle(UINT resolution, SingularLight* linkedLight, DepthRenderer* renderer) : Shadowmap(resolution, linkedLight), renderer(renderer)
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
	Camera* view = linkedLight->ShadowMapCamera();
	renderer->CameraDepthRender(mapDSV, view);
	delete view;
}

void ShadowMapSingle::BindMap()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::ShadowMapType(0);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowMap(mapSRV);
}



ShadowMapCube::ShadowMapCube(UINT resolution, SingularLight* linkedLight, OmniDistanceRenderer* renderer) : Shadowmap(resolution, linkedLight), renderer(renderer)
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
	textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &mapTexture)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	D3D11_TEXCUBE_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.TextureCube = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(mapTexture, &srvDesc, &mapSRV)))
	{
		std::cerr << "Failed to set up own shadow map SRV" << std::endl;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_RTV texRTV;
	texRTV.ArraySize = 1;
	texRTV.MipSlice = 0;
	rtvDesc.Texture2DArray = texRTV;
	
	for (int i = 0; i < 6; i++)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		if (FAILED(Pipeline::Device()->CreateRenderTargetView(mapTexture, &rtvDesc, &mapRTV[i])))
		{
			std::cerr << "Error: Failed to set up shadowmap RTVs" << std::endl;
		}
	}
}

ShadowMapCube::~ShadowMapCube()
{
	for (int i = 0; i < 6; i++)
	{
		mapRTV[i]->Release();
	}
}

void ShadowMapCube::MapRender()
{
	Camera* view = linkedLight->ShadowMapCamera();

	renderer->OmniDistanceRender(mapRTV, view);

	delete view;
}

void ShadowMapCube::BindMap()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::ShadowMapType(1);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowCubeMap(mapSRV);
}

AmbientLight::AmbientLight(const std::array<float, 3>& ambient)
{
	parametersModifyed = false;
	for (int i = 0; i < 3; i++)
	{
		this->lightAmbient[i] = ambient[i];
	}

	D3D11_BUFFER_DESC bufferDesc;

	AmbientLightBuffer bufferStruct = AmbientLightBuffer(lightAmbient);

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

AmbientLight::~AmbientLight()
{
	parameterBuffer->Release();
}

void AmbientLight::SetAmbient(std::array<float, 3> lightAmbient)
{
	for (int i = 0; i < 3; i++)
	{
		this->lightAmbient[i] = lightAmbient[i];
	}
	parametersModifyed = true;
}

std::array<float, 3> AmbientLight::Ambient()
{
	return lightAmbient;
}

DirectX::XMFLOAT4X4 AmbientLight::TransformMatrix()
{
	return DirectX::XMFLOAT4X4();
}

DirectX::XMFLOAT4X4 AmbientLight::InverseTransformMatrix()
{
	return DirectX::XMFLOAT4X4();
}

void AmbientLight::Render()
{
}

void AmbientLight::DepthRender()
{
}

void AmbientLight::Bind()
{
	if (parametersModifyed)
	{
		UpdateParameterBuffer();

		parametersModifyed = false;
	}

	BindBuffer();
}

void AmbientLight::UpdateParameterBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	AmbientLightBuffer bufferStruct = AmbientLightBuffer(lightAmbient);

	Pipeline::ResourceManipulation::MapBuffer(parameterBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterBuffer);
}

void AmbientLight::BindBuffer()
{
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(LightShaderMode::LightTypeAmbientBasic);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::AmbientLightParameterBuffer(parameterBuffer);
}

SpotDirLightArray::SpotDirLightArray(UINT lightCapacity, UINT shadowmapResolution, DepthRenderer& renderer, std::vector<LightBaseStaging*>* StagingLights) :
	lightCapacity(lightCapacity),
	mapResolution(shadowmapResolution),
	shadowmapRenderer(&renderer),
	StagingLights(StagingLights)
{
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(SpotDirStruct) * lightCapacity;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(SpotDirStruct);

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &parametersStructuredBuffer)))
	{
		std::cerr << "Failed to set up parameters structured buffer!" << std::endl;
	}

	bufferDesc.ByteWidth = sizeof(ShadowMappingBuffer) * lightCapacity;
	bufferDesc.StructureByteStride = sizeof(ShadowMappingBuffer);

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &shadowProjectionsStructuredBuffer)))
	{
		std::cerr << "Failed to set up parameters structured buffer!" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	D3D11_BUFFER_SRV bufSRV = {};
	bufSRV.NumElements = lightCapacity;
	srvDesc.Buffer = bufSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(parametersStructuredBuffer, &srvDesc, &parametersSRV)))
	{
		std::cerr << "Failed to set up parameter buffer SRV" << std::endl;
	}

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(shadowProjectionsStructuredBuffer, &srvDesc, &shadowProjectionsSRV)))
	{
		std::cerr << "Failed to set up shadowmapping buffer SRV" << std::endl;
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = lightCapacity;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Width = mapResolution;
	textureDesc.Height = mapResolution;
	
	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &shadowmapArray)))
	{
		std::cerr << "Failed to set up shadowmap texture array resource" << std::endl;
	}

	srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	texSRV.ArraySize = lightCapacity;
	texSRV.FirstArraySlice = 0;
	srvDesc.Texture2DArray = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(shadowmapArray, &srvDesc, &shadowmapsSRV)))
	{
		std::cerr << "Failed to set up own shadow map SRV" << std::endl;
	}
}

SpotDirLightArray::~SpotDirLightArray()
{
	parametersStructuredBuffer->Release();
	parametersSRV->Release();

	shadowProjectionsStructuredBuffer->Release();
	shadowProjectionsSRV->Release();

	shadowmapArray->Release();
	shadowmapsSRV->Release();
}

void SpotDirLightArray::Render()
{
}

void SpotDirLightArray::DepthRender()
{
}

void SpotDirLightArray::Bind()
{
	int count = StagingLights->size() > lightCapacity ? lightCapacity : StagingLights->size();
	int i = 0;
	for (LightBaseStaging* light : *StagingLights)
	{
		if (i >= count) break;
		
		if (light->CastShadow())
		{
			if (light->ShadowmapResolution() == mapResolution)
			{
				light->StageLightParameters(i, parametersStructuredBuffer);

				if (!light->ShadowIsUpdated())
				{
					Camera* view = light->ShadowMapCamera();
					shadowmapRenderer->CameraDepthRender(light->StagingShadowmapDSV(), view);
					delete view;
					light->MarkShadowUpdate();
				}
				light->StageLightShadowmap(i, shadowProjectionsStructuredBuffer, shadowmapArray);
				i++;
			}
		}
		else
		{
			light->StageLightParameters(i, parametersStructuredBuffer);
			i++;
		}
	}

	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightArrayParameters(parametersSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightArrayShadowMapping(shadowProjectionsSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightArrayShadowmaps(shadowmapsSRV);
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightCount(count);
	Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(LightShaderMode::SpotDirArray);
	Pipeline::ShadowMapping::BorderSampleWhite();
}

UINT SpotDirLightArray::ShadowmapsResolution()
{
	return mapResolution;
}

DirectX::XMFLOAT4X4 SpotDirLightArray::TransformMatrix()
{
	return DirectX::XMFLOAT4X4();
}

DirectX::XMFLOAT4X4 SpotDirLightArray::InverseTransformMatrix()
{
	return DirectX::XMFLOAT4X4();
}

LightBaseStaging::LightBaseStaging()
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create light transform Buffer" << std::endl;
	}
}

LightBaseStaging::~LightBaseStaging()
{
	parameterStagingBuffer->Release();

	RemoveShadowMapping();
	
	worldTransformBuffer->Release();
}

void LightBaseStaging::RemoveShadowMapping()
{
	if (shadowProjectionStagingBuffer != nullptr)
	{
		shadowProjectionStagingBuffer->Release();
		shadowProjectionStagingBuffer = nullptr;
	}
	if (shadowmapTexture != nullptr)
	{
		shadowmapTexture->Release();
		shadowmapTexture = nullptr;
	}
	if (shadowmapDSV != nullptr)
	{
		shadowmapDSV->Release();
		shadowmapDSV = nullptr;
	}
	SetCastShadow(false);
}

void LightBaseStaging::SetCastShadow(bool castShadow)
{
	bool newValue;
	if (shadowProjectionStagingBuffer != nullptr)
	{
		newValue = castShadow;
	}
	else
	{
		newValue = false;
	}

	if (newValue != this->castShadow)
	{
		FlagModification();
	}

	this->castShadow = newValue;
}

bool LightBaseStaging::CastShadow()
{
	return castShadow;
}

UINT LightBaseStaging::ShadowmapResolution()
{
	return mapResolution;
}

void LightBaseStaging::StageLightParameters(UINT index, ID3D11Buffer* parametersStructuredBuffer)
{
	if (modyfied)
	{
		StagingBuffersUpdate();
		modyfied = false;
	}
	Pipeline::ResourceManipulation::StageResource(parametersStructuredBuffer, index, sizeof(SpotDirStruct), parameterStagingBuffer);
}

void LightBaseStaging::StageLightShadowmap(UINT index, ID3D11Buffer* shadowProjectionsStructuredBuffer, ID3D11Texture2D* shadowmapArray)
{
	if (modyfied)
	{
		StagingBuffersUpdate();
		modyfied = false;
	}
	Pipeline::ResourceManipulation::StageResource(shadowProjectionsStructuredBuffer, index, sizeof(ShadowMappingBuffer), shadowProjectionStagingBuffer);
	Pipeline::ResourceManipulation::StageResource(shadowmapArray, index, shadowmapTexture);
}

ID3D11DepthStencilView* LightBaseStaging::StagingShadowmapDSV()
{
	return shadowmapDSV;
}

void LightBaseStaging::MarkShadowUpdate()
{
	latestUpdate = Pipeline::FrameCounter();
}

bool LightBaseStaging::ShadowIsUpdated()
{
	return latestUpdate == Pipeline::FrameCounter();
}

void LightBaseStaging::Render()
{
}

void LightBaseStaging::DepthRender()
{
}

DirectX::XMFLOAT4X4 LightBaseStaging::TransformMatrix()
{
	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	rotation = DirectX::XMMatrixRotationRollPitchYaw(90.0f, 0.0f, 0.0f) * rotation;

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

DirectX::XMFLOAT4X4 LightBaseStaging::InverseTransformMatrix()
{
	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion)));

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invTranslation * invRotation));

	return output;
}

void LightBaseStaging::FlagModification()
{
	modyfied = true;
}

void LightBaseStaging::OnModyfied()
{
	modyfied = true;
}

SpotLightStaging::SpotLightStaging(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof) :
	LightBaseStaging(), lightFOV(lightFOV), fallof(fallof), lightDiffuse(diffuse), lightSpecular(specular)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotDirStruct bufferStruct = SpotDirStruct(lightFOV, fallof, diffuse, specular, { position.x, position.y, position.z }, { output.x, output.y, output.z }, 0);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &parameterStagingBuffer)))
	{
		std::cerr << "Failed to create light parameter Buffer" << std::endl;
	}
}

void SpotLightStaging::SetupShadowmapping(SpotDirLightArray& lightArray, float nearPlane, float farPlane)
{
	RemoveShadowMapping();

	mapResolution = lightArray.ShadowmapsResolution();
	this->farPlane = farPlane;
	this->nearPlane = nearPlane;

	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMFLOAT4X4 projection = DirectX::XMFLOAT4X4();

	DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(lightFOV * OBJECT_ROTATION_UNIT_DEGREES, 1.0f, nearPlane, farPlane)));

	ShadowMappingBuffer bufferStruct = ShadowMappingBuffer(InverseTransformMatrix(), projection);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &shadowProjectionStagingBuffer)))
	{
		std::cerr << "Failed to create light shadowmapping Buffer" << std::endl;
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Width = mapResolution;
	textureDesc.Height = mapResolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &shadowmapTexture)))
	{
		std::cerr << "Failed to set up shadow map texture resource" << std::endl;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_DSV texDSV;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2D = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(shadowmapTexture, &dsvDesc, &shadowmapDSV)))
	{
		std::cerr << "Error: Failed to set up shadowmap DSV" << std::endl;
	}
}

Camera* SpotLightStaging::ShadowMapCamera()
{
	if (!CastShadow()) return nullptr;

	CameraPerspective* outputCamera = new CameraPerspective(mapResolution, mapResolution, 0, 0, lightFOV, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	outputCamera->Rotate(rotationQuaternion, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void SpotLightStaging::SetDiffuse(std::array<float, 3> lightDiffuse)
{
	this->lightDiffuse = lightDiffuse;
	FlagModification();
}

std::array<float, 3> SpotLightStaging::Diffuse()
{
	return lightDiffuse;
}

void SpotLightStaging::SetSpecular(std::array<float, 3> lightSpecular)
{
	this->lightSpecular = lightSpecular;
	FlagModification();
}

std::array<float, 3> SpotLightStaging::Specular()
{
	return lightSpecular;
}

void SpotLightStaging::StagingBuffersUpdate()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotDirStruct bufferStruct = SpotDirStruct(lightFOV, fallof, lightDiffuse, lightSpecular, { position.x, position.y, position.z }, { output.x, output.y, output.z }, CastShadow() ? 1 : 0);

	Pipeline::ResourceManipulation::MapStagingBuffer(parameterStagingBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterStagingBuffer);

	if (CastShadow())
	{
		DirectX::XMFLOAT4X4 projection = DirectX::XMFLOAT4X4();

		DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(lightFOV * OBJECT_ROTATION_UNIT_DEGREES, 1.0f, nearPlane, farPlane)));

		ShadowMappingBuffer shadowBufferStruct = ShadowMappingBuffer(InverseTransformMatrix(), projection);

		Pipeline::ResourceManipulation::MapStagingBuffer(shadowProjectionStagingBuffer, &mappedResource);
		memcpy(mappedResource.pData, &shadowBufferStruct, sizeof(shadowBufferStruct));
		Pipeline::ResourceManipulation::UnmapBuffer(shadowProjectionStagingBuffer);
	}
}

DirectionalLightStaging::DirectionalLightStaging(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) :
	LightBaseStaging(), lightDiffuse(diffuse), lightSpecular(specular)
{
	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotDirStruct bufferStruct = SpotDirStruct(diffuse, specular, { output.x, output.y, output.z }, 0);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &parameterStagingBuffer)))
	{
		std::cerr << "Failed to create light parameter Buffer" << std::endl;
	}
}

void DirectionalLightStaging::SetupShadowmapping(SpotDirLightArray& lightArray, float viewWidth, float nearPlane, float farPlane)
{
	RemoveShadowMapping();

	mapResolution = lightArray.ShadowmapsResolution();

	this->viewWidth = viewWidth;
	this->farPlane = farPlane;
	this->nearPlane = nearPlane;

	D3D11_BUFFER_DESC bufferDesc;

	DirectX::XMFLOAT4X4 projection = DirectX::XMFLOAT4X4();

	DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(viewWidth, viewWidth, nearPlane, farPlane)));

	ShadowMappingBuffer bufferStruct = ShadowMappingBuffer(InverseTransformMatrix(), projection);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &shadowProjectionStagingBuffer)))
	{
		std::cerr << "Failed to create light shadowmapping Buffer" << std::endl;
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Width = mapResolution;
	textureDesc.Height = mapResolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &shadowmapTexture)))
	{
		std::cerr << "Failed to set up shadow map texture resource" << std::endl;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_DSV texDSV;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2D = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(shadowmapTexture, &dsvDesc, &shadowmapDSV)))
	{
		std::cerr << "Error: Failed to set up shadowmap DSV" << std::endl;
	}
}

Camera* DirectionalLightStaging::ShadowMapCamera()
{
	if (!CastShadow()) return nullptr;

	CameraOrthographic* outputCamera = new CameraOrthographic(mapResolution, mapResolution, 0, 0, viewWidth, nearPlane, farPlane);

	outputCamera->Translate({ position.x, position.y, position.z }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	outputCamera->Rotate(rotationQuaternion, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE);

	return outputCamera;
}

void DirectionalLightStaging::SetDiffuse(std::array<float, 3> lightDiffuse)
{
	this->lightDiffuse = lightDiffuse;
	FlagModification();
}

std::array<float, 3> DirectionalLightStaging::Diffuse()
{
	return lightDiffuse;
}

void DirectionalLightStaging::SetSpecular(std::array<float, 3> lightSpecular)
{
	this->lightSpecular = lightSpecular;
	FlagModification();
}

std::array<float, 3> DirectionalLightStaging::Specular()
{
	return lightSpecular;
}

void DirectionalLightStaging::StagingBuffersUpdate()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));
	DirectX::XMVECTOR direction = { 0.0f, 0.0f, -1.0f };
	direction = DirectX::XMVector3Transform(direction, rotation);
	DirectX::XMFLOAT3 output;
	DirectX::XMStoreFloat3(&output, direction);

	SpotDirStruct bufferStruct = SpotDirStruct(lightDiffuse, lightSpecular, { output.x, output.y, output.z }, CastShadow() ? 1 : 0);

	Pipeline::ResourceManipulation::MapStagingBuffer(parameterStagingBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(parameterStagingBuffer);

	if (CastShadow())
	{
		DirectX::XMFLOAT4X4 projection = DirectX::XMFLOAT4X4();

		DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(viewWidth, viewWidth, nearPlane, farPlane)));

		ShadowMappingBuffer shadowBufferStruct = ShadowMappingBuffer(InverseTransformMatrix(), projection);

		Pipeline::ResourceManipulation::MapStagingBuffer(shadowProjectionStagingBuffer, &mappedResource);
		memcpy(mappedResource.pData, &shadowBufferStruct, sizeof(shadowBufferStruct));
		Pipeline::ResourceManipulation::UnmapBuffer(shadowProjectionStagingBuffer);
	}
}

void SpotLightStaging::SetFOV(float lightFOV)
{
	this->lightFOV = lightFOV;
	FlagModification();
}

float SpotLightStaging::FOV()
{
	return lightFOV;
}

void SpotLightStaging::SetFalloff(float falloff)
{
	this->fallof = falloff;
	FlagModification();
}

float SpotLightStaging::Falloff()
{
	return fallof;
}
