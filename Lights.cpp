#include "Lights.h"
#include <iostream>
#include "Renderer.h"

#include "Pipeline.h"

LightBase::LightBase() :
	paramsModified(true),
	transformed(true),
	parameters(LightBufferStruct()),
	arrayPosition(-1)
{
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(LightBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &parameters;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &paramsBuffer)))
	{
		std::cerr << "failed to create staged parameter buffer of individual light base" << std::endl;
	}
}

LightBase::~LightBase()
{
	paramsBuffer->Release();

	if (shadowMap != nullptr)
	{
		ShadowMapDestroy();
	}
}

bool LightBase::SetupBase()
{
	paramCount = 0;
	paramCapacity = SMALLEST_PARAM_CAPACITY;

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = paramCapacity * sizeof(LightBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(LightBufferStruct);

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &paramsStructuredBuffer)))
	{
		return false;
	}

	slotTracker = new LightBase * [paramCapacity];

	return true;
}

void LightBase::ReleaseBase()
{
	paramsStructuredBuffer->Release();

	delete[] slotTracker;
}

void LightBase::SetParams(LightBufferStruct& parameters)
{
	this->parameters = parameters;
	paramsModified = true;
}

void LightBase::BindParams()
{
	bool newParams = UpdateParams();

	if (arrayPosition < 0)
	{
		arrayPosition = FreePosition();

		while (arrayPosition < 0)
		{
			Reallocate(paramCapacity * 2);
			arrayPosition = FreePosition();
		}

		slotTracker[arrayPosition] = this;

		paramCount++;
	}
	else if (!newParams)
	{
		return;
	}
	
	Pipeline::ResourceManipulation::StructuredBufferCpy(paramsStructuredBuffer, arrayPosition, paramsBuffer, 0, 1);
}

void LightBase::UnbindParams()
{
	if (arrayPosition < 0) return;

	slotTracker[arrayPosition] = nullptr;

	paramCount--;

	if (slotTracker[paramCount] != nullptr)
	{
		slotTracker[paramCount]->ParamMove(arrayPosition);
	}

	arrayPosition = -1;

	if ((paramCapacity > SMALLEST_PARAM_CAPACITY) && (paramCount < (paramCapacity / 2 - 2)))
	{
		Reallocate(paramCapacity / 2);
	}
}

void LightBase::LightFOV(float angle)
{
	parameters.lightFOV = angle;
	paramsModified = true;
}

void LightBase::Falloff(float falloff)
{
	parameters.falloff = falloff;
	paramsModified = true;
}

void LightBase::OnModyfied()
{
	transformed = true;
}

void LightBase::BindBase()
{
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightParameterStructuredBuffer(paramsStructuredBuffer);
}

bool LightBase::ShadowMapCreate(int mapType)
{
	if (shadowMap != nullptr)
	{
		return false;
	}

	shadowMap = SpecificShadowOptions(mapType);
	
	if (shadowMap == nullptr)
	{
		return false;
	}

	if (shadowMap->SetupComplete())
	{
		return true;
	}

	delete shadowMap;
	return false;
}

void LightBase::ShadowMapDestroy()
{
	if (shadowMap == nullptr)
	{
		return;
	}
	delete shadowMap;
	shadowMap = nullptr;
}

bool LightBase::HasShadowMap()
{
	return shadowMap != nullptr;
}

void LightBase::SetCastShadows(bool castShadows)
{
	if (castShadows)
	{
		if ((shadowMap != nullptr) && (parameters.shadowMapIndex < 0))
		{
			shadowMap->BindShadowMap();

			parameters.shadowMapIndex = shadowMap->ShadowResourceIndex();
			parameters.shadowMapTypeIndex = shadowMap->ShadowTypeIndex();
			return;
		}

		parameters.shadowMapIndex = -1;
		parameters.shadowMapTypeIndex = -1;
		return;
	}
	
	if ((shadowMap != nullptr) && (parameters.shadowMapIndex >= 0))
	{
		shadowMap->UbindShadowMap();
	}

	parameters.shadowMapIndex = -1;
	parameters.shadowMapTypeIndex = -1;
}

bool LightBase::CastShadowsStatus()
{
	return parameters.shadowMapIndex >= 0;
}

void LightBase::RenderShadowMap()
{
	if (HasShadowMap() && CastShadowsStatus())
	{
		shadowMap->RenderShadowMap();
	}
}

void LightBase::SetLightDiffuse(const std::array<float, 3>& diffuse)
{
	parameters.diffuse[0] = diffuse[0];
	parameters.diffuse[1] = diffuse[1];
	parameters.diffuse[2] = diffuse[2];

	paramsModified = true;
}

void LightBase::SetLightSpecular(const std::array<float, 3>& specular)
{
	parameters.specular[0] = specular[0];
	parameters.specular[1] = specular[1];
	parameters.specular[2] = specular[2];

	paramsModified = true;
}

void LightBase::Bind()
{
	BindParams();
	if (shadowMap != nullptr)
	{
		shadowMap->BindShadowMap();
	}
}

void LightBase::Unbind()
{
	UnbindParams();
	if (shadowMap != nullptr)
	{
		shadowMap->UbindShadowMap();
	}
}

void LightBase::ParamMove(int newPosition)
{
	slotTracker[newPosition] = this;

	Pipeline::ResourceManipulation::StructuredBufferCpy(paramsStructuredBuffer, newPosition, paramsBuffer, 0, 1);

	arrayPosition = newPosition;
}

bool LightBase::UpdateParams()
{
	if (transformed)
	{
		parameters.invTransform = InverseTransformMatrix();

		DirectX::XMFLOAT3 direction;
		LightDirection(direction);

		parameters.direction[0] = direction.x;
		parameters.direction[1] = direction.y;
		parameters.direction[2] = direction.z;

		parameters.position[0] = position.x;
		parameters.position[1] = position.y;
		parameters.position[2] = position.z;

		transformed = false;
		paramsModified = true;
	}

	if (paramsModified)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		Pipeline::ResourceManipulation::MapBuffer(paramsBuffer, &mappedResource);
		memcpy(mappedResource.pData, &parameters, sizeof(LightBufferStruct));
		Pipeline::ResourceManipulation::UnmapBuffer(paramsBuffer);

		paramsModified = false;

		return true;
	}
	return false;
}

int LightBase::FreePosition()
{
	for (int i = 0; i < paramCapacity; i++)
	{
		if (*(slotTracker + i) == nullptr)
		{
			return i;
		}
	}
	return -1;
}

bool LightBase::Reallocate(UINT newCapacity)
{
	ID3D11Buffer* buffersTemp;

	LightBase** trackerTemp;

	trackerTemp = new LightBase*[newCapacity];

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = newCapacity * sizeof(LightBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(LightBufferStruct);

	D3D11_SUBRESOURCE_DATA data;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &buffersTemp)))
	{
		return false;
	}

	UINT cpySize = newCapacity > paramCapacity ? paramCapacity : newCapacity;

	Pipeline::ResourceManipulation::StructuredBufferCpy(buffersTemp, 0, paramsStructuredBuffer, 0, cpySize);

	memcpy(trackerTemp, slotTracker, cpySize);

	delete[] slotTracker;

	slotTracker = trackerTemp;

	paramsStructuredBuffer->Release();
	paramsStructuredBuffer = buffersTemp;

	return true;
}

ShadowHandlerBase::ShadowHandlerBase() :
	shadowMapUpdated(true),
	arrayPosition(-1),
	setupComplete(true)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	ShadowMapDesc(textureDesc);

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &shadowMap)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
		setupComplete = false;
	}
}

ShadowHandlerBase::~ShadowHandlerBase()
{
	shadowMap->Release();
}

bool ShadowHandlerBase::SetupShadows()
{
	StaticCount() = 0;
	StaticCapacity() = SMALLES_SHADOW_CAPACITY;

	D3D11_TEXTURE2D_DESC textureDesc;
	ShadowMapDesc(textureDesc);
	textureDesc.ArraySize = StaticCapacity();

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &StaticShadowMapArray())))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ShadowMapArraySRVDesc(srvDesc);
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = StaticCapacity();
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(StaticShadowMapArray(), &srvDesc, &StaticShadowMapArraySRV())))
	{
		return false;
	}
	
	StaticSlotTracker() = new ShadowHandlerBase * [StaticCapacity()];

	return true;
}

void ShadowHandlerBase::ReleaseShadows()
{
	StaticShadowMapArray()->Release();
	StaticShadowMapArraySRV()->Release();
	delete[] StaticSlotTracker();
}

void ShadowHandlerBase::ShadowMove(int newPosition)
{
	StaticSlotTracker()[newPosition] = this;

	Pipeline::ResourceManipulation::TextureArrayCpy(StaticShadowMapArray(), newPosition, shadowMap, 0, 1);

	arrayPosition = newPosition;
}

bool ShadowHandlerBase::SetupComplete()
{
	return setupComplete;
}

void ShadowHandlerBase::UpdateShadowMap()
{
	shadowMapUpdated = true;
}

int ShadowHandlerBase::ShadowResourceIndex()
{
	return arrayPosition;
}

void ShadowHandlerBase::BindShadowMap()
{
	if (arrayPosition < 0)
	{
		arrayPosition = FreePosition();

		while (arrayPosition < 0)
		{
			Reallocate(StaticCapacity() * 2);
			arrayPosition = FreePosition();
		}

		StaticSlotTracker()[arrayPosition] = this;

		StaticCount()++;
	}
	else if (!shadowMapUpdated)
	{
		return;
	}

	Pipeline::ResourceManipulation::TextureArrayCpy(StaticShadowMapArray(), arrayPosition, shadowMap, 0, 1);
}

void ShadowHandlerBase::UbindShadowMap()
{
	if (arrayPosition < 0) return;

	StaticSlotTracker()[arrayPosition] = nullptr;

	StaticCount()--;

	if (StaticSlotTracker()[StaticCount()] != nullptr)
	{
		StaticSlotTracker()[StaticCount()]->ShadowMove(arrayPosition);
	}

	arrayPosition = -1;

	if ((StaticCapacity() > SMALLES_SHADOW_CAPACITY) && (StaticCount() < (StaticCapacity() / 2 - 2)))
	{
		Reallocate(StaticCapacity() / 2);
	}
}

int ShadowHandlerBase::FreePosition()
{
	for (int i = 0; i < StaticCapacity(); i++)
	{
		if (*(StaticSlotTracker() + i) == nullptr)
		{
			return i;
		}
	}
	return -1;
}

bool ShadowHandlerBase::Reallocate(UINT newCapacity)
{
	ID3D11Texture2D* texturesTemp;

	ShadowHandlerBase** trackerTemp;

	trackerTemp = new ShadowHandlerBase*[newCapacity];

	D3D11_TEXTURE2D_DESC textureDesc;
	ShadowMapDesc(textureDesc);
	textureDesc.ArraySize = newCapacity;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &texturesTemp)))
	{
		return false;
	}

	UINT cpySize = newCapacity > StaticCapacity() ? StaticCapacity() : newCapacity;

	Pipeline::ResourceManipulation::TextureArrayCpy(texturesTemp, 0, StaticShadowMapArray(), 0, cpySize);

	memcpy(trackerTemp, StaticSlotTracker(), cpySize);

	delete[] StaticSlotTracker();

	StaticSlotTracker() = trackerTemp;

	StaticShadowMapArray()->Release();
	StaticShadowMapArray() = texturesTemp;

	return true;
}

ShadowMapSingle320X320::ShadowMapSingle320X320()
{
	if (FAILED(Pipeline::Device()->CreateDepthStencilView(shadowMap, nullptr, &mapDSV)))
	{
		std::cerr << "Failed to set up own shadow map DSV" << std::endl;
		setupComplete = false;
	}
}

ShadowMapSingle320X320::~ShadowMapSingle320X320()
{
	mapDSV->Release();
}

int ShadowMapSingle320X320::ShadowTypeIndex()
{
	return Single320X320;
}

void ShadowMapSingle320X320::RenderShadowMap()
{
	
}

void ShadowMapSingle320X320::ShadowMapDesc(D3D11_TEXTURE2D_DESC& textureDesc)
{
	textureDesc.Width = 320;
	textureDesc.Height = 320;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_STAGING;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
}

void ShadowMapSingle320X320::ShadowMapArraySRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
}

UINT& ShadowMapSingle320X320::StaticCount()
{
	return count;
}

UINT& ShadowMapSingle320X320::StaticCapacity()
{
	return capacity;
}

ShadowHandlerBase**& ShadowMapSingle320X320::StaticSlotTracker()
{
	return slotTracker;
}

ID3D11Texture2D*& ShadowMapSingle320X320::StaticShadowMapArray()
{
	return shadowMapArray;
}

ID3D11ShaderResourceView*& ShadowMapSingle320X320::StaticShadowMapArraySRV()
{
	return shadowMapArraySRV;
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
	return DirectX::XMFLOAT4X4();
}

void PointLight::LightDirection(DirectX::XMFLOAT3& direction)
{
}
