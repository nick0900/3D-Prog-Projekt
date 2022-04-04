#include <Windows.h>

#include <iostream>

#include "Pipeline.h"
//#include "PipelineResources.h"

namespace Base
{
	static ID3D11Device* device;
	static ID3D11DeviceContext* immediateContext;
	static IDXGISwapChain* swapChain;

	static UINT backBufferWidth;
	static UINT backBufferHeight;
}

namespace PixelShading
{
	static ID3D11SamplerState* sampler;
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

	if (FAILED(Base::device->CreateSamplerState(&samplerDesc, &PixelShading::sampler)))
	{
		std::cerr << "Failed to create sampler!" << std::endl;
		return false;
	}

	Base::immediateContext->PSSetSamplers(0, 1, &PixelShading::sampler);
	return true;
}

void Pipeline::Release()
{
	Base::swapChain->Release();
	Base::immediateContext->Release();
	Base::device->Release();

	PixelShading::sampler->Release();
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

void Pipeline::Deferred::GeometryPass::Clear::DepthStencilView(ID3D11DepthStencilView* dsv)
{
	Base::immediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
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
	Base::immediateContext->PSGetShaderResources(0, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::DiffuseMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSGetShaderResources(1, 1, &SRV);
}

void Pipeline::Deferred::GeometryPass::PixelShader::Bind::SpecularMap(ID3D11ShaderResourceView* SRV)
{
	Base::immediateContext->PSGetShaderResources(2, 1, &SRV);
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

void Pipeline::Map::Buffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mappedResource)
{
	Base::immediateContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, mappedResource);
}

void Pipeline::Unmap::Buffer(ID3D11Buffer* buffer)
{
	Base::immediateContext->Unmap(buffer, 0);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::ComputeShader(ID3D11ComputeShader* cShader)
{
	Base::immediateContext->CSSetShader(cShader, nullptr, 0);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewBuffer(ID3D11Buffer* cameraViewBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(0, 1, &cameraViewBuffer);
}

void Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraProjectionBuffer(ID3D11Buffer* cameraProjectionBuffer)
{
	Base::immediateContext->CSSetConstantBuffers(0, 1, &cameraProjectionBuffer);
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

void Pipeline::Deferred::LightPass::ComputeShader::Bind::BackBufferUAV(ID3D11UnorderedAccessView* UAV)
{
	Base::immediateContext->CSSetUnorderedAccessViews(0, 1, &UAV, nullptr);
}

bool Pipeline::Deferred::LightPass::ComputeShader::Dispatch16by9(UINT width, UINT height)
{
	UINT computeWidth = 32;
	UINT computeHeight = 18;

	UINT dimension = width / computeWidth;

	if ((computeWidth * dimension != width) || (computeHeight * dimension != height))
	{
		std::cerr << "width and height must both be a multiple of " << computeWidth << " and " << computeHeight << " respectively!" << std::endl;
		return false;
	}

	Base::immediateContext->Dispatch(dimension, dimension, 1);
	return true;
}
