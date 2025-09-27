#include <iostream>
#include <random>
#include <math.h>

#include "ParticleSystems.h"

#include "SharedResources.h"
#include "Pipeline.h"

ParticleSystem::~ParticleSystem()
{
	if (particleBufferFirst != nullptr) particleBufferFirst->Release();
	if (particleBufferSecond != nullptr) particleBufferSecond->Release();
	if (particleDataViewFirst != nullptr) particleDataViewFirst->Release();
	if (particleDataViewSecond != nullptr) particleDataViewSecond->Release();
	if (appendConsumeViewFirst != nullptr) appendConsumeViewFirst->Release();
	if (appendConsumeViewSecond != nullptr) appendConsumeViewSecond->Release();
	if (indirectParticleCount != nullptr) indirectParticleCount->Release();
	if (constantBufferParticleCount != nullptr) constantBufferParticleCount->Release();
}

void ParticleSystem::BuffersSetup(D3D11_SUBRESOURCE_DATA& data, UINT structSize, UINT initialCount)
{
	inFirst = true;

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = structSize * initialCount;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = structSize;
	
	if (particleBufferFirst != nullptr) particleBufferFirst->Release();

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &particleBufferFirst)))
	{
		std::cerr << "Failed to set up append/consume buffer!" << std::endl;
	}

	if (particleBufferSecond != nullptr) particleBufferSecond->Release();

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &particleBufferSecond)))
	{
		std::cerr << "Failed to set up append/consume buffer!" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	D3D11_BUFFER_SRV bufSRV = {};
	bufSRV.NumElements = initialCount;
	srvDesc.Buffer = bufSRV;

	if (particleDataViewFirst != nullptr) particleDataViewFirst->Release();

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(particleBufferFirst, &srvDesc, &particleDataViewFirst)))
	{
		std::cerr << "Failed to set up buffer SRV" << std::endl;
	}

	if (particleDataViewSecond != nullptr) particleDataViewSecond->Release();

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(particleBufferSecond, &srvDesc, &particleDataViewSecond)))
	{
		std::cerr << "Failed to set up buffer SRV" << std::endl;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	D3D11_BUFFER_UAV bufUAV;
	bufUAV.FirstElement = 0;
	bufUAV.NumElements = initialCount;
	bufUAV.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	uavDesc.Buffer = bufUAV;

	if (appendConsumeViewFirst != nullptr) appendConsumeViewFirst->Release();

	if (FAILED(Pipeline::Device()->CreateUnorderedAccessView(particleBufferFirst, &uavDesc, &appendConsumeViewFirst)))
	{
		std::cerr << "Failed to set up buffer UAV" << std::endl;
	}

	if (appendConsumeViewSecond != nullptr) appendConsumeViewSecond->Release();

	if (FAILED(Pipeline::Device()->CreateUnorderedAccessView(particleBufferSecond, &uavDesc, &appendConsumeViewSecond)))
	{
		std::cerr << "Failed to set up buffer UAV" << std::endl;
	}

	UINT args[4] = { initialCount, 1, 0, 0 };

	bufferDesc.ByteWidth = sizeof(UINT) * 4;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bufferDesc.StructureByteStride = 0;

	data.pSysMem = args;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (indirectParticleCount != nullptr) indirectParticleCount->Release();

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &indirectParticleCount)))
	{
		std::cerr << "Failed to set up indirect args buffer!" << std::endl;
	}

	ParticleCountBufferStruct countStruct = ParticleCountBufferStruct(initialCount);

	bufferDesc.ByteWidth = sizeof(ParticleCountBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	data.pSysMem = &countStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (constantBufferParticleCount != nullptr) constantBufferParticleCount->Release();

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &constantBufferParticleCount)))
	{
		std::cerr << "Failed to set up particle count constant buffer!" << std::endl;
	}

	ID3D11UnorderedAccessView* uavs[2] = { appendConsumeViewFirst, appendConsumeViewSecond };
	UINT init[2] = { initialCount, 0 };
	Pipeline::Particles::Update::Bind::AppendConsumeBuffers(uavs, init);
	Pipeline::Particles::Update::Clear::UAVs();
}

bool ParticleSystem::IsInitilized()
{
	return (particleBufferFirst != nullptr) &&
		(particleBufferSecond != nullptr) &&
		(particleDataViewFirst != nullptr) &&
		(particleDataViewSecond != nullptr) &&
		(appendConsumeViewFirst != nullptr) &&
		(appendConsumeViewSecond != nullptr) &&
		(indirectParticleCount != nullptr) &&
		(constantBufferParticleCount != nullptr);
}

void ParticleSystem::SwitchBuffers()
{
	inFirst = !inFirst;
}

void ParticleSystem::BindForParticleUpdate()
{
	if (inFirst)
	{
		ID3D11UnorderedAccessView* uavs[2] = { appendConsumeViewFirst, appendConsumeViewSecond };
		UINT init[2] = { -1, -1 };
		Pipeline::Particles::Update::Bind::AppendConsumeBuffers(uavs, init);

		Pipeline::Particles::CopyCount(constantBufferParticleCount, appendConsumeViewFirst);
	}
	else
	{
		ID3D11UnorderedAccessView* uavs[2] = { appendConsumeViewSecond, appendConsumeViewFirst };
		UINT init[2] = { -1, -1 };
		Pipeline::Particles::Update::Bind::AppendConsumeBuffers(uavs, init);

		Pipeline::Particles::CopyCount(constantBufferParticleCount, appendConsumeViewSecond);
	}

	Pipeline::Particles::Update::Bind::ConstantBuffer(constantBufferParticleCount);
}

void ParticleSystem::BindForParticleRender()
{
	if (inFirst)
	{
		Pipeline::Particles::Render::Bind::ParticleBuffer(particleDataViewFirst);

		Pipeline::Particles::CopyCount(indirectParticleCount, appendConsumeViewFirst);
	}
	else
	{
		Pipeline::Particles::Render::Bind::ParticleBuffer(particleDataViewSecond);

		Pipeline::Particles::CopyCount(indirectParticleCount, appendConsumeViewSecond);
	}
}

Galaxy::Galaxy(const std::string particleTexturePath) : particleCount(0), re(42)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create transform buffer!" << std::endl;
	}

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(GalaxyParticleStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, nullptr, &addBuffer)))
	{
		std::cerr << "Failed to set up add buffer!" << std::endl;
	}

	textureID = SharedResources::GetTexture(particleTexturePath);

	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;


	if (FAILED(Pipeline::Device()->CreateBlendState(&blendDesc, &blendState)))
	{
		std::cerr << "Failed to set up BlendState!" << std::endl;
	}

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

	if (FAILED(Pipeline::Device()->CreateDepthStencilState(&depthDesc, &depthState)))
	{
		std::cerr << "Failed to set up depthstencilState!" << std::endl;
	}
}

Galaxy::~Galaxy()
{
	worldTransformBuffer->Release();
	addBuffer->Release();
	blendState->Release();
	depthState->Release();
}

void Galaxy::InitializeParticles(UINT count)
{
	maxParticles = count;

	std::normal_distribution<float> nd(0, 1);
	std::exponential_distribution<float> ed(0.5);
	std::uniform_real_distribution<float> ud(0.0f, 360.0f);

	GalaxyParticleStruct* particles = new GalaxyParticleStruct[count];

	for (int i = 0; i < count; i++)
	{
		float angle = ud(re);
		float distance = ed(re);
		float planeOffset = nd(re);
		particles[i].position = DirectX::XMFLOAT3(cos(angle) * distance, planeOffset, sin(angle) * distance);

		DirectX::XMVECTOR particlePos = DirectX::XMLoadFloat3(&(particles[i].position));

		DirectX::XMVECTOR helpVector = { 0.0f, 1.0f, 0.0f };

		helpVector = DirectX::XMVector3Cross(particlePos, helpVector);

		helpVector = DirectX::XMVector3Cross(particlePos, helpVector);

		helpVector = DirectX::XMVector3Normalize(helpVector);

		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationAxis(helpVector, 0.5f * OBJECT_ROTATION_UNIT_DEGREES / (distance + 1.0f));

		DirectX::XMStoreFloat4x4(&(particles[i].rotation), rotationMatrix);

		std::uniform_real_distribution<float> udcolor(0.0f, 0.1f);
		float colorDeviance = udcolor(re);

		particles[i].color[0] = 1.0f - colorDeviance;
		colorDeviance = udcolor(re);
		particles[i].color[1] = 1.0f - colorDeviance;
		particles[i].color[2] = 1.0f;
	}

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = particles;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	BuffersSetup(data, sizeof(GalaxyParticleStruct), count);

	delete[] particles;

	particleCount = count;
}

void Galaxy::UpdateParticles()
{
	if (!IsInitilized()) return;

	BindForParticleUpdate();

	SharedResources::BindComputeShader(SharedResources::cShader::GalaxyUpdate32);

	Pipeline::Particles::Update::Dispatch32(particleCount);

	Pipeline::Particles::Update::Clear::UAVs();

	SwitchBuffers();
}

void Galaxy::AddParticles()
{
	if (maxParticles <= particleCount) return;

	GalaxyParticleStruct particle;

	std::normal_distribution<float> nd(0, 1);
	std::exponential_distribution<float> ed(0.5);
	std::uniform_real_distribution<float> ud(0.0f, 360.0f);

	float angle = ud(re);
	float distance = ed(re);
	float planeOffset = nd(re);
	particle.position = DirectX::XMFLOAT3(cos(angle) * distance, planeOffset, sin(angle) * distance);

	DirectX::XMVECTOR particlePos = DirectX::XMLoadFloat3(&(particle.position));

	DirectX::XMVECTOR helpVector = { 0.0f, 1.0f, 0.0f };

	helpVector = DirectX::XMVector3Cross(particlePos, helpVector);

	helpVector = DirectX::XMVector3Cross(particlePos, helpVector);

	helpVector = DirectX::XMVector3Normalize(helpVector);

	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationAxis(helpVector, 0.5f * OBJECT_ROTATION_UNIT_DEGREES / (distance + 1.0f));

	DirectX::XMStoreFloat4x4(&(particle.rotation), rotationMatrix);

	std::uniform_real_distribution<float> udcolor(0.0f, 0.5f);
	float colorDeviance = udcolor(re);

	particle.color[0] = 1.0f - colorDeviance / 5.0f;
	colorDeviance = udcolor(re);
	particle.color[1] = 1.0f - colorDeviance;
	particle.color[2] = 0.6f;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	Pipeline::ResourceManipulation::MapBuffer(addBuffer, &mappedResource);
	memcpy(mappedResource.pData, &particle, sizeof(GalaxyParticleStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(addBuffer);

	BindForParticleUpdate();

	Pipeline::Particles::Update::Bind::ConstantBuffer(addBuffer);

	SharedResources::BindComputeShader(SharedResources::cShader::GalaxyAdd1);

	Pipeline::Particles::Update::Dispatch1();

	Pipeline::Particles::Update::Clear::UAVs();

	particleCount++;
}

void Galaxy::RemoveParticles()
{
	if (particleCount == 1) return;
	BindForParticleUpdate();

	SharedResources::BindComputeShader(SharedResources::cShader::GalaxyRemove1);

	Pipeline::Particles::Update::Dispatch1();

	Pipeline::Particles::Update::Clear::UAVs();

	particleCount--;
}

void Galaxy::Render()
{
	if (!IsInitilized()) return;
	BindForParticleRender();

	SharedResources::BindVertexShader(SharedResources::vShader::VSParticlePoints);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UpdateTransformBuffer();

	SharedResources::BindGeometryShader(SharedResources::gShader::BillboardedParticles);
	Pipeline::Particles::Render::Bind::GSTransformBuffer(worldTransformBuffer);
	
	SharedResources::BindPixelShader(SharedResources::pShader::PSGalaxyParticles);
	Pipeline::Particles::Render::Bind::PSParticleTexture(SharedResources::GetTextureSRV(textureID));

	Pipeline::Particles::Render::Bind::BlendState(blendState);
	Pipeline::Particles::Render::Bind::DepthState(depthState);


	Pipeline::Particles::Render::IndirectInstancedDraw(indirectParticleCount);


	Pipeline::Particles::Render::Clear::GeometryShader();
	Pipeline::Particles::Render::Clear::ParticleBuffer();
	Pipeline::Particles::Render::Clear::BlendState();
	Pipeline::Particles::Render::Clear::DepthState();
}

void Galaxy::DepthRender()
{
	//will not cast shadows
}
