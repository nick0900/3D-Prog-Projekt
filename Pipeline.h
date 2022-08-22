#pragma once
#include <d3d11.h>
#include <vector>
#include <Windows.h>

namespace Pipeline
{
	bool SetupRender(UINT width, UINT height, HWND window);
	void Release();

	ID3D11Device* Device();
	bool GetBackbufferRTV(ID3D11RenderTargetView*& backBufferRTV);
	bool GetBackbufferUAV(ID3D11UnorderedAccessView*& backBufferUAV);
	UINT BackBufferWidth();
	UINT BackBufferHeight();
	void DrawIndexed(UINT size, UINT start);
	void Switch();

	void IncrementCounter();
	UINT FrameCounter();

	namespace Deferred
	{
		namespace GeometryPass
		{
			namespace Set
			{
				void Viewport(D3D11_VIEWPORT& viewport);
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
					void Reflectionmap(ID3D11ShaderResourceView* SRV);

					void GBuffers(ID3D11RenderTargetView* normal, ID3D11RenderTargetView* ambient, ID3D11RenderTargetView* diffuse, ID3D11RenderTargetView* specular, ID3D11DepthStencilView* dsView);
				}

				namespace Clear
				{
					void SRVs();
					void GBuffers();
				}
			}

			namespace HullShader
			{
				namespace Bind
				{
					void HullShader(ID3D11HullShader* hShader);
					void HSConfigBuffer(ID3D11Buffer* buffer);
				}

				namespace UnBind
				{
					void HullShader();
				}
			}

			namespace DomainShader
			{
				namespace Bind
				{
					void DomainShader(ID3D11DomainShader* dShader);
					void DSConfigBuffer(ID3D11Buffer* buffer);
					void viewBuffer(ID3D11Buffer* buffer);
					void ProjectionBuffer(ID3D11Buffer* buffer);
				}

				namespace UnBind
				{
					void DomainShader();
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
					void CameraViewportBuffer(ID3D11Buffer* cameraViewportBuffer);
					void AmbientLightParameterBuffer(ID3D11Buffer* parameterBuffer);
					void PointLightParameterBuffer(ID3D11Buffer* parameterBuffer);
					void DirectionalLightParameterBuffer(ID3D11Buffer* parameterBuffer);
					void SpotLightParameterBuffer(ID3D11Buffer* parameterBuffer);
					void ShadowmappingBuffer(ID3D11Buffer* shadowmappingBuffer);

					void DepthBuffer(ID3D11ShaderResourceView* SRV);
					void NormalBuffer(ID3D11ShaderResourceView* SRV);
					void AmbientBuffer(ID3D11ShaderResourceView* SRV);
					void DiffuesBuffer(ID3D11ShaderResourceView* SRV);
					void SpecularBuffer(ID3D11ShaderResourceView* SRV);
					void ShadowMap(ID3D11ShaderResourceView* SRV);
					void ShadowCubeMap(ID3D11ShaderResourceView* SRV);
					void LightArrayParameters(ID3D11ShaderResourceView* SRV);
					void LightArrayShadowMapping(ID3D11ShaderResourceView* SRV);
					void LightArrayShadowmaps(ID3D11ShaderResourceView* SRV);

					void BackBufferUAV(ID3D11UnorderedAccessView* UAV);
				}

				namespace Clear
				{
					void ComputeSRVs();
					void TargetUAV();
				}
				
				bool Dispatch32X32(UINT width, UINT height, UINT topLeftX, UINT topLeftY);

				bool ColorDispatch32X32(UINT width, UINT height, UINT topLeftX, UINT topLeftY);

				namespace Settings
				{
					void LightType(int type);
					void ShadowMapType(int type);
					void Shadowcaster(bool castsShadows);
					void LightCount(UINT count);

					void BindBuffer();
				}
			}
		}
	}

	namespace ShadowMapping
	{
		void ClearPixelShader();

		void BindDepthStencil(ID3D11DepthStencilView* dsv);
		void BindDistanceBuffer(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv);
		void UnbindDepthStencil();
		void UnbindDistanceBuffer();

		void BorderSampleBlack();
		void BorderSampleWhite();
	}

	namespace Clean
	{
		void RenderTargetView(ID3D11RenderTargetView* rtv);
		void DepthStencilView(ID3D11DepthStencilView* dsv);
		void UnorderedAccessView(ID3D11UnorderedAccessView* uav);
	}

	namespace ResourceManipulation
	{
		void MapBuffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mappedResource);
		void UnmapBuffer(ID3D11Buffer* buffer);
		void MapStagingBuffer(ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mappedResource);
		void StageResource(ID3D11Buffer* dstResource, UINT dstIndex, UINT elementSize, ID3D11Buffer* stagingResource);
		void StageResource(ID3D11Texture2D* dstResource, UINT dstIndex, ID3D11Texture2D* stagingResource);
	}

	namespace Particles
	{
		void CopyCount(ID3D11Buffer* dstBuffer, ID3D11UnorderedAccessView* srcView);

		namespace Render
		{
			namespace Bind
			{
				void GeometryShader(ID3D11GeometryShader* gShader);
				void ParticleBuffer(ID3D11ShaderResourceView* bufferSRV);

				void GSViewBuffer(ID3D11Buffer* viewBuffer);
				void GSProjectionBuffer(ID3D11Buffer* projBuffer);
				void GSTransformBuffer(ID3D11Buffer* transformBuffer);

				void PSParticleTexture(ID3D11ShaderResourceView* srv);

				void BlendState(ID3D11BlendState* bs);
				void DepthState(ID3D11DepthStencilState* dss);
			}

			namespace Clear
			{
				void GeometryShader();
				void InputAssembler();
				void ParticleBuffer();

				void BlendState();
				void DepthState();
			}

			void IndirectInstancedDraw(ID3D11Buffer* argsBuffer);
		}

		namespace Update
		{
			namespace Bind
			{
				void AppendConsumeBuffers(ID3D11UnorderedAccessView* uav[2], UINT count[2]);

				void ConstantBuffer(ID3D11Buffer* countBuffer);
			}

			namespace Clear
			{
				void UAVs();
			}

			void Dispatch32(UINT particleCount);

			void Dispatch1();
		}
	}
}