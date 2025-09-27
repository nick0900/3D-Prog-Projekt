#pragma once
#include <DirectXMath.h>
#include <string>
#include <random>

#include "BaseObject.h"

struct ParticleCountBufferStruct
{
	UINT particleCount;
	float padding[3];

	ParticleCountBufferStruct(UINT count) : particleCount(count)
	{
		for (int i = 0; i < 3; i++)
		{
			padding[i] = 0.0f;
		}
	}
};

class ParticleSystem : public Object
{
public:
	virtual ~ParticleSystem();

protected:
	void BuffersSetup(D3D11_SUBRESOURCE_DATA& data, UINT structSize, UINT initialCount);

	bool IsInitilized();

	void SwitchBuffers();

	void BindForParticleUpdate();
	void BindForParticleRender();

protected:
	ID3D11Buffer* indirectParticleCount = nullptr;

private:
	bool inFirst = true;

	ID3D11Buffer* particleBufferFirst = nullptr;
	ID3D11UnorderedAccessView* appendConsumeViewFirst = nullptr;
	ID3D11ShaderResourceView* particleDataViewFirst = nullptr;

	ID3D11Buffer* particleBufferSecond = nullptr;
	ID3D11UnorderedAccessView* appendConsumeViewSecond = nullptr;
	ID3D11ShaderResourceView* particleDataViewSecond = nullptr;

	ID3D11Buffer* constantBufferParticleCount = nullptr;
};

struct GalaxyParticleStruct
{
	DirectX::XMFLOAT3 position;
	float padding1;

	DirectX::XMFLOAT4X4 rotation;

	float color[3];
	float padding2;
};

class Galaxy : public ParticleSystem
{
public:
	Galaxy(const std::string particleTexturePath);
	virtual ~Galaxy();

	void InitializeParticles(UINT count);
	void UpdateParticles();
	void AddParticles();
	void RemoveParticles();

	virtual void Render() override;
	virtual void DepthRender() override;

private:
	UINT particleCount;
	UINT maxParticles;

	int textureID;

	ID3D11Buffer* addBuffer;

	ID3D11BlendState* blendState;

	ID3D11DepthStencilState* depthState;

	std::default_random_engine re;
};
