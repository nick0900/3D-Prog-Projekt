#include <Windows.h>
#include <iostream>

#include "Pipeline.h"
#include "SharedResources.h"

namespace Base
{
	static ID3D11Device* device;
	static ID3D11DeviceContext* immediateContext;
	static IDXGISwapChain* swapChain;

	static UINT backBufferWidth;
	static UINT backBufferHeight;

	static UINT frameCount;
}

namespace Samplers
{
	static ID3D11SamplerState* samplerwrap;
	static ID3D11SamplerState* samplerBorderBlack;
	static ID3D11SamplerState* samplerBorderWhite;
}

namespace CSConfig
{
	struct CSSettings
	{
		int lightType = -1;
		int shadowMapType = -1;
		float shadowBias = 0.09f;
		bool shadowcaster = false;
		bool padding[3];
	} settings;

	ID3D11Buffer* CSConfigBuffer;
}

bool CreateInterfaces(UINT width, UINT height, HWND window, ID3D11Device*& device, ID3D11DeviceContext*& immediateContext, IDXGISwapChain*& swapChain)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// default
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = window;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;


	UINT flags = 0;
	if (_DEBUG)
	{
		flags = D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &immediateContext);

	return !FAILED(hr);
}

bool Pipeline::SetupRender(UINT width, UINT height, HWND window)
{
	Base::backBufferWidth = width;
	Base::backBufferHeight = height;

	Base::frameCount = 0;

	if (!CreateInterfaces(width, height, window, Base::device, Base::immediateContext, Base::swapChain))
	{
		std::cerr << "Failed to create interfaces!" << std::endl;
		return false;
	}
	
	D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;

	if (FAILED(Base::device->CreateSamplerState(&samplerDesc, &Samplers::samplerwrap)))
	{
		std::cerr << "Failed to create sampler!" << std::endl;
		return false;
	}

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	if (FAILED(Base::device->CreateSamplerState(&samplerDesc, &Samplers::samplerBorderBlack)))
	{
		std::cerr << "Failed to create sampler!" << std::endl;
		return false;
	}

	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;

	if (FAILED(Base::device->CreateSamplerState(&samplerDesc, &Samplers::samplerBorderWhite)))
	{
		std::cerr << "Failed to create sampler!" << std::endl;
		return false;
	}

	Base::immediateContext->PSSetSamplers(0, 1, &Samplers::samplerwrap);
	Base::immediateContext->CSSetSamplers(0, 1, &Samplers::samplerwrap);

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(CSConfig::CSSettings);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &CSConfig::settings;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &CSConfig::CSConfigBuffer)))
	{
		std::cerr << "Failed to create compute shader config buffer" << std::endl;
		return false;
	}
	return true;
}

void Pipeline::Release()
{
	Base::swapChain->Release();
	Base::immediateContext->Release();
	Base::device->Release();

	Samplers::samplerwrap->Release();
	Samplers::samplerBorderBlack->Release();
	Samplers::samplerBorderWhite->Release();
	CSConfig::CSConfigBuffer->Release();
}

ID3D11Device* Pipeline::Device()
{
	return Base::device;
}

bool Pipeline::GetBackbufferRTV(ID3D11RenderTargetView*& backBufferRTV)
{
	ID3D11Texture2D* backBuffer = nullptr;
	if (FAILED(Base::swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
	{
		std::cerr << "Failed to get back buffer!" << std::endl;
		return false;
	}

	HRESULT hr = Pipeline::Device()->CreateRenderTargetView(backBuffer, nullptr, &backBufferRTV);

	backBuffer->Release();

	return !FAILED(hr);
}

bool Pipeline::GetBackbufferUAV(ID3D11UnorderedAccessView*& backBufferUAV)
{
	ID3D11Texture2D* backBuffer = nullptr;
	if (FAILED(Base::swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
	{
		std::cerr << "Failed to get back buffer!" << std::endl;
		return false;
	}

	HRESULT hr = Pipeline::Device()->CreateUnorderedAccessView(backBuffer, nullptr, &backBufferUAV);

	backBuffer->Release();

	return !FAILED(hr);
}

UINT Pipeline::BackBufferWidth()
{
	return Base::backBufferWidth;
}

UINT Pipeline::BackBufferHeight()
{
	return Base::backBufferHeight;
}

void Pipeline::DrawIndexed(UINT size, UINT start)
{
	Base::immediateContext->DrawIndexed(size, start, 0);
}

void Pipeline::Switch()
{
	Base::swapChain->Present(0, 0);
}

void Pipeline::IncrementCounter()
{
	Base::frameCount++;
}

UINT Pipeline::FrameCounter()
{
	return Base::frameCount;
}

void Pipeline::Deferred::GeometryPass::Set::Viewport(D3D11_VIEWPORT& viewport)
{
	Base::immediateContext->RSSetViewports(1, &viewport);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexShader(ID3D11VertexShader* vShader)
{
	Base::immediateContext->VSSetShader(vShader, nullptr, 0);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::InputLayout(ID3D11InputLayout* inputLayout)
{
	Base::immediateContext->IASetInputLayout(inputLayout);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	Base::immediateContext->IASetPrimitiveTopology(topology);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexBuffer(UINT stride, UINT offset, ID3D11Buffer* vBuffer)
{
	Base::immediateContext->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::IndexBuffer(ID3D11Buffer* iBuffer)
{
	Base::immediateContext->IASetIndexBuffer(iBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraViewBuffer(ID3D11Buffer* cameraViewBuffer)
{
	Base::immediateContext->VSSetConstantBuffers(0, 1, &cameraViewBuffer);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraProjectionBuffer(ID3D11Buffer* cameraProjectionBuffer)
{
	Base::immediateContext->VSSetConstantBuffers(1, 1, &cameraProjectionBuffer);
}

void Pipeline::Deferred::GeometryPass::VertexShader::Bind::ObjectTransform(ID3D11Buffer* transformBuffer)
{
	Base::immediateContext->VSSetConstantBuffers(2, 1, &transformBuffer);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::PixelShader(ID3D11PixelShader* pShader)
{
	Base::immediateContext->PSSetShader(pShader, nullptr, 0);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::MaterialParameters(ID3D11Buffer* paramBuffer)
{
	Base::immediateContext->PSSetConstantBuffers(0, 1, &paramBuffer);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::AmbientMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSSetShaderResources(0, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::DiffuseMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSSetShaderResources(1, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::SpecularMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSSetShaderResources(2, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::Reflectionmap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSSetShaderResources(0, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::GBuffers(ID3D11RenderTargetView* normal, ID3D11RenderTargetView* ambient, ID3D11RenderTargetView* diffuse, ID3D11RenderTargetView* specular, ID3D11DepthStencilView* dsView)
{
	ID3D11RenderTargetView* RTVs[4] = { normal, ambient, diffuse, specular };

	Base::immediateContext->OMSetRenderTargets(4, RTVs, dsView);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Clear::SRVs()
{
	ID3D11ShaderResourceView* clear[3] = {nullptr, nullptr, nullptr};
	Base::immediateContext->PSSetShaderResources(0, 3, clear);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Clear::GBuffers()
{
	ID3D11RenderTargetView* clear[4] = { nullptr, nullptr, nullptr, nullptr };
	Base::immediateContext->OMSetRenderTargets(4, clear, nullptr);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::ComputeShader(ID3D11ComputeShader* cShader)
{
	Base::immediateContext->CSSetShader(cShader, nullptr, 0);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewBuffer(ID3D11Buffer* cameraViewBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(1, 1, &cameraViewBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraProjectionBuffer(ID3D11Buffer* cameraProjectionBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(2, 1, &cameraProjectionBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewportBuffer(ID3D11Buffer* cameraViewportBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(3, 1, &cameraViewportBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::AmbientLightParameterBuffer(ID3D11Buffer* parameterBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(4, 1, &parameterBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::PointLightParameterBuffer(ID3D11Buffer* parameterBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(5, 1, &parameterBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::DirectionalLightParameterBuffer(ID3D11Buffer* parameterBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(6, 1, &parameterBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::SpotLightParameterBuffer(ID3D11Buffer* parameterBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(7, 1, &parameterBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowmappingBuffer(ID3D11Buffer* shadowmappingBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(8, 1, &shadowmappingBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::DepthBuffer(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(0, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::NormalBuffer(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(1, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::AmbientBuffer(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(2, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::DiffuesBuffer(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(3, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::SpecularBuffer(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(4, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(5, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::ShadowCubeMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->CSSetShaderResources(6, 1, &SRV);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::BackBufferUAV(ID3D11UnorderedAccessView* UAV)
{
	Base::immediateContext->CSSetUnorderedAccessViews(0, 1, &UAV, nullptr);
}

bool Pipeline::Deferred::LightPass::ComputeShader::Dispatch32X32(UINT width, UINT height, UINT topLeftX, UINT topLeftY)
{
	SharedResources::BindComputeShader(SharedResources::cShader::Standard32x32);

	UINT computeWidth = 32;
	UINT computeHeight = 32;

	UINT dispatchWidth = width / computeWidth;

	UINT dispatchHeight = height / computeHeight;

	if ((computeWidth * dispatchWidth != width) || (computeHeight * dispatchHeight != height))
	{
		std::cerr << "width and height must be a multiple of " << computeWidth << " and " << computeHeight << " respectively!" << std::endl;
		return false;
	}

	Base::immediateContext->Dispatch(dispatchWidth, dispatchHeight, 1);
	return true;
}

bool Pipeline::Deferred::LightPass::ComputeShader::ColorDispatch32X32(UINT width, UINT height, UINT topLeftX, UINT topLeftY)
{
	SharedResources::BindComputeShader(SharedResources::cShader::ColorPass32x32);

	UINT computeWidth = 32;
	UINT computeHeight = 32;

	UINT dispatchWidth = width / computeWidth;

	UINT dispatchHeight = height / computeHeight;

	if ((computeWidth * dispatchWidth != width) || (computeHeight * dispatchHeight != height))
	{
		std::cerr << "width and height must be a multiple of " << computeWidth << " and " << computeHeight << " respectively!" << std::endl;
		return false;
	}

	Base::immediateContext->Dispatch(dispatchWidth, dispatchHeight, 1);
	return true;
}

void Pipeline::Deferred::LightPass::ComputeShader::Clear::ComputeSRVs()
{
	ID3D11ShaderResourceView* clear[7] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	Base::immediateContext->CSSetShaderResources(0, 7, clear);
}

void Pipeline::Deferred::LightPass::ComputeShader::Clear::TargetUAV()
{
	ID3D11UnorderedAccessView* clear[1] = { nullptr };
	Base::immediateContext->CSSetUnorderedAccessViews(0, 1, clear, nullptr);
}

void Pipeline::ResourceManipulation::MapBuffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mappedResource)
{
	Base::immediateContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, mappedResource);
}

void Pipeline::ResourceManipulation::UnmapBuffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->Unmap(buffer, 0);
}

void Pipeline::ShadowMapping::ClearPixelShader()
{
	Base::immediateContext->PSSetShader(nullptr, nullptr, 0);
}

void Pipeline::ShadowMapping::BindDepthStencil(ID3D11DepthStencilView* dsv)
{
	Base::immediateContext->OMSetRenderTargets(0, nullptr, dsv);
}

void Pipeline::ShadowMapping::BindDistanceBuffer(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
{
	SharedResources::BindPixelShader(SharedResources::pShader::DistanceWrite);
	Base::immediateContext->OMSetRenderTargets(1, &rtv, dsv);
}

void Pipeline::ShadowMapping::UnbindDepthStencil()
{
	Base::immediateContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void Pipeline::ShadowMapping::UnbindDistanceBuffer()
{
	Base::immediateContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void Pipeline::ShadowMapping::BorderSampleBlack()
{
	Base::immediateContext->PSSetSamplers(1, 1, &Samplers::samplerBorderBlack);
	Base::immediateContext->CSSetSamplers(1, 1, &Samplers::samplerBorderBlack);
}

void Pipeline::ShadowMapping::BorderSampleWhite()
{
	Base::immediateContext->PSSetSamplers(1, 1, &Samplers::samplerBorderWhite);
	Base::immediateContext->CSSetSamplers(1, 1, &Samplers::samplerBorderWhite);
}

void Pipeline::Clean::RenderTargetView(ID3D11RenderTargetView* rtv)
{
	float clearColour[4] = { 0, 0, 0, 0 };
	Base::immediateContext->ClearRenderTargetView(rtv, clearColour);
}

void Pipeline::Clean::DepthStencilView(ID3D11DepthStencilView* dsv)
{
	Base::immediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void Pipeline::Clean::UnorderedAccessView(ID3D11UnorderedAccessView* uav)
{
	float clearColour[4] = { 0, 0, 0, 0 };
	Base::immediateContext->ClearUnorderedAccessViewFloat(uav, clearColour);
}

void Pipeline::Deferred::LightPass::ComputeShader::Settings::LightType(int type)
{
	CSConfig::settings.lightType = type;
}

void Pipeline::Deferred::LightPass::ComputeShader::Settings::ShadowMapType(int type)
{
	CSConfig::settings.shadowMapType = type;
}

void Pipeline::Deferred::LightPass::ComputeShader::Settings::Shadowcaster(bool castsShadows)
{
	CSConfig::settings.shadowcaster = castsShadows;
}

void Pipeline::Deferred::LightPass::ComputeShader::Settings::BindBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	Pipeline::ResourceManipulation::MapBuffer(CSConfig::CSConfigBuffer, &mappedResource);
	memcpy(mappedResource.pData, &CSConfig::settings, sizeof(CSConfig::CSSettings));
	Pipeline::ResourceManipulation::UnmapBuffer(CSConfig::CSConfigBuffer);

	Base::immediateContext->CSSetConstantBuffers(0, 1, &CSConfig::CSConfigBuffer);
}

void Pipeline::Deferred::GeometryPass::HullShader::Bind::HullShader(ID3D11HullShader* hShader)
{
	Base::immediateContext->HSSetShader(hShader, nullptr, 0);
}

void Pipeline::Deferred::GeometryPass::HullShader::Bind::HSConfigBuffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->HSSetConstantBuffers(0, 1, &buffer);
}

void Pipeline::Deferred::GeometryPass::HullShader::UnBind::HullShader()
{
	Base::immediateContext->HSSetShader(nullptr, nullptr, 0);
}

void Pipeline::Deferred::GeometryPass::DomainShader::Bind::DomainShader(ID3D11DomainShader* dShader)
{
	Base::immediateContext->DSSetShader(dShader, nullptr, 0);
}

void Pipeline::Deferred::GeometryPass::DomainShader::Bind::DSConfigBuffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->DSSetConstantBuffers(0, 1, &buffer);
}

void Pipeline::Deferred::GeometryPass::DomainShader::Bind::viewBuffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->DSSetConstantBuffers(1, 1, &buffer);
}

void Pipeline::Deferred::GeometryPass::DomainShader::Bind::ProjectionBuffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->DSSetConstantBuffers(2, 1, &buffer);
}

void Pipeline::Deferred::GeometryPass::DomainShader::UnBind::DomainShader()
{
	Base::immediateContext->DSSetShader(nullptr, nullptr, 0);
}

void Pipeline::Particles::Update::Bind::AppendBuffer(ID3D11UnorderedAccessView* uav)
{
	UINT init[1] = { 1000 };
	Base::immediateContext->CSSetUnorderedAccessViews(0, 1, &uav, init);
}

void Pipeline::Particles::Update::Bind::ConsumeBuffer(ID3D11UnorderedAccessView* uav)
{
	UINT init[1] = { 0 };
	Base::immediateContext->CSSetUnorderedAccessViews(1, 1, &uav, init);
}

void Pipeline::Particles::Update::Bind::AppendConsumeBuffers(ID3D11UnorderedAccessView* uav[2], UINT count[2])
{
	Base::immediateContext->CSSetUnorderedAccessViews(0, 2, uav, count);
}

void Pipeline::Particles::Update::Bind::ConstantBuffer(ID3D11Buffer* countBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(0, 1, &countBuffer);
}

void Pipeline::Particles::Update::Clear::UAVs()
{
	ID3D11UnorderedAccessView* clear[2] = { nullptr, nullptr };
	Base::immediateContext->CSSetUnorderedAccessViews(0, 2, clear, nullptr);
}

void Pipeline::Particles::CopyCount(ID3D11Buffer* dstBuffer, ID3D11UnorderedAccessView* srcView)
{
	Base::immediateContext->CopyStructureCount(dstBuffer, 0, srcView);
}

void Pipeline::Particles::Update::Dispatch32(UINT particleCount)
{
	UINT computeWidth = 32;

	UINT dispatchWidth = particleCount / computeWidth + (particleCount % computeWidth != 0);

	Base::immediateContext->Dispatch(dispatchWidth, 1, 1);
}

void Pipeline::Particles::Update::Dispatch1()
{
	Base::immediateContext->Dispatch(1, 1, 1);
}

void Pipeline::Particles::Render::Clear::GeometryShader()
{
	Base::immediateContext->GSSetShader(nullptr, nullptr, 0);
}

void Pipeline::Particles::Render::Clear::InputAssembler()
{
	ID3D11Buffer* clear[1] = { nullptr };
	UINT uint = 0;
	Base::immediateContext->IASetVertexBuffers(0, 1, clear, &uint, &uint);
	Base::immediateContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	Base::immediateContext->IASetInputLayout(nullptr);
}

void Pipeline::Particles::Render::Clear::ParticleBuffer()
{
	ID3D11ShaderResourceView* clear[1] = { nullptr };
	Base::immediateContext->VSSetShaderResources(0, 1, clear);
}

void Pipeline::Particles::Render::Clear::BlendState()
{
	Base::immediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Pipeline::Particles::Render::Clear::DepthState()
{
	Base::immediateContext->OMSetDepthStencilState(nullptr, 0);
}

void Pipeline::Particles::Render::Bind::GeometryShader(ID3D11GeometryShader* gShader)
{
	Base::immediateContext->GSSetShader(gShader, nullptr, 0);
}

void Pipeline::Particles::Render::Bind::ParticleBuffer(ID3D11ShaderResourceView* bufferSRV)
{
	Base::immediateContext->VSSetShaderResources(0, 1, &bufferSRV);
}

void Pipeline::Particles::Render::Bind::GSViewBuffer(ID3D11Buffer* viewBuffer)
{
	Base::immediateContext->GSSetConstantBuffers(0, 1, &viewBuffer);
}

void Pipeline::Particles::Render::Bind::GSProjectionBuffer(ID3D11Buffer* projBuffer)
{
	Base::immediateContext->GSSetConstantBuffers(1, 1, &projBuffer);
}

void Pipeline::Particles::Render::Bind::GSTransformBuffer(ID3D11Buffer* transformBuffer)
{
	Base::immediateContext->GSSetConstantBuffers(2, 1, &transformBuffer);
}

void Pipeline::Particles::Render::Bind::PSParticleTexture(ID3D11ShaderResourceView* srv)
{
	Base::immediateContext->PSSetShaderResources(0, 1, &srv);
}

void Pipeline::Particles::Render::Bind::BlendState(ID3D11BlendState* bs)
{
	Base::immediateContext->OMSetBlendState(bs, nullptr,0xffffffff);
}

void Pipeline::Particles::Render::Bind::DepthState(ID3D11DepthStencilState* dss)
{
	Base::immediateContext->OMSetDepthStencilState(dss, 0);
}

void Pipeline::Particles::Render::IndirectInstancedDraw(ID3D11Buffer* argsBuffer)
{
	Base::immediateContext->DrawInstancedIndirect(argsBuffer, 0);
}
