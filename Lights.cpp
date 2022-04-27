#include "Lights.h"
#include <iostream>

#include "Pipeline.h"

LightBase::LightBase() :
	paramsModified(true),
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
}

bool LightBase::SetupBase()
{
	paramCount = 0;
	paramCapacity = 8;

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

	//implement deallocation
}

void LightBase::BindBase()
{
	Pipeline::Deferred::LightPass::ComputeShader::Bind::LightParameterStructuredBuffer(paramsStructuredBuffer);
}

void LightBase::ParamMove(int newPosition)
{
	slotTracker[newPosition] = this;

	Pipeline::ResourceManipulation::StructuredBufferCpy(paramsStructuredBuffer, newPosition, paramsBuffer, 0, 1);

	arrayPosition = newPosition;
}

bool LightBase::UpdateParams()
{
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
	arrayPosition(-1)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	ShadowMapDesc(textureDesc);

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &shadowMap)))
	{
		std::cerr << "Failed to set up own shadow map texture resource" << std::endl;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ShadowMapRTVDesc(rtvDesc);

	if (FAILED(Pipeline::Device()->CreateRenderTargetView(shadowMap, &rtvDesc, &ShadowMapRTV)))
	{
		std::cerr << "Failed to set up own shadow map renderTarget" << std::endl;
	}
}

ShadowHandlerBase::~ShadowHandlerBase()
{
	shadowMap->Release();
	ShadowMapRTV->Release();
}

bool ShadowHandlerBase::SetupShadows()
{
	StaticCount() = 0;
	StaticCapacity() = 8;

	D3D11_TEXTURE2D_DESC textureDesc;
	ShadowMapDesc(textureDesc);
	textureDesc.ArraySize = StaticCapacity();

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &StaticShadowMapArray())))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ShadowMapArraySRVDesc(srvDesc, StaticCapacity());

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
