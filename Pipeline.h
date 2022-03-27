#pragma once
#include <d3d11.h>
#include <vector>
#include <Windows.h>

namespace Pipeline
{
	bool SetupRender(UINT width, UINT height, HWND window);
	void Release();

	ID3D11Device* Device();
	bool GetBackbufferRTV(ID3D11RenderTargetView* backBufferRTV);
	bool GetBackbufferUAV(ID3D11UnorderedAccessView* backBufferUAV);
	UINT BackBufferWidth();
	UINT BackBufferHeight();
	void DrawIndexed(UINT size, UINT start);
	void Switch();

	namespace Deferred
	{
		namespace GeometryPass
		{
			namespace Set
			{
				void Viewport(D3D11_VIEWPORT& viewport);
			}

			namespace Clear
			{
				void DepthStencilView(ID3D11DepthStencilView* dsv);
			}

			namespace VertexShader
			{
				namespace Bind
				{
					void VertexShader(ID3D11VertexShader* vShader);
					void InputLayout(ID3D11InputLayout* inputLayout);
					void PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);

					void VertexBuffer(UINT stride, UINT offset, ID3D11Buffer* vBuffer);
					void IndexBuffer(ID3D11Buffer* iBuffer);

					void cameraViewBuffer(ID3D11Buffer* cameraViewBuffer);
					void cameraProjectionBuffer(ID3D11Buffer* cameraProjectionBuffer);
					void ObjectTransform(ID3D11Buffer* transformBuffer);
				}
			}

			namespace PixelShader
			{
				namespace Bind
				{
					void PixelShader(ID3D11PixelShader* pShader);

					void MaterialParameters(ID3D11Buffer* paramBuffer);

					void AmbientMap(ID3D11ShaderResourceView* SRV);
					void DiffuseMap(ID3D11ShaderResourceView* SRV);
					void SpecularMap(ID3D11ShaderResourceView* SRV);

					void GBuffers(ID3D11RenderTargetView* normal, ID3D11RenderTargetView* ambient, ID3D11RenderTargetView* diffuse, ID3D11RenderTargetView* specular, ID3D11DepthStencilView* dsView);
				}

				namespace Clear
				{
					void SRVs();
				}
			}
		}

		namespace LightPass
		{
			namespace ComputeShader
			{
				namespace Bind
				{
					void ComputeShader(ID3D11ComputeShader* cShader);

					void CameraViewBuffer(ID3D11Buffer* cameraViewBuffer);
					void CameraProjectionBuffer(ID3D11Buffer* cameraProjectionBuffer);

					void Pointlights(ID3D11Buffer* pointlights, UINT numLights);
					void Spotlights(ID3D11Buffer* spotlights, UINT numLights);

					void DepthBuffer(ID3D11ShaderResourceView* SRV);
					void NormalBuffer(ID3D11ShaderResourceView* SRV);
					void AmbientBuffer(ID3D11ShaderResourceView* SRV);
					void DiffuesBuffer(ID3D11ShaderResourceView* SRV);
					void SpecularBuffer(ID3D11ShaderResourceView* SRV);

					void BackBufferUAV(ID3D11UnorderedAccessView* UAV);
				}
				
				bool Dispatch16by9(UINT width, UINT height);
			}
		}
	}

	namespace Map
	{
		void Buffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mappedResource);
	}

	namespace Unmap
	{
		void Buffer(ID3D11Buffer* buffer);
	}
}