#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>

#include "BaseObject.h"

enum class CameraPreset
{
	HDstandard,
	ShadowCast,
	ReflectionMap
};

struct ProjBuffer
{
	DirectX::XMFLOAT4X4 projectionMatrix;
	
	float widthScalar;
	float heightScalar;

	float projectionConstantA;
	float projectionVonstantB;
	

	ProjBuffer(float FovAngleY, float AspectRatio, UINT width, UINT height, float NearZ, float FarZ) : 

	widthScalar(tan(FovAngleY / 2.0f) / (float)width), 
	heightScalar((float)width / (AspectRatio * (float)height)), 

	projectionConstantA(FarZ / (FarZ - NearZ)), 
	projectionVonstantB((-NearZ * FarZ) / (FarZ - NearZ)),

	projectionMatrix(DirectX::XMFLOAT4X4())
	{
		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ));

		DirectX::XMStoreFloat4x4(&projectionMatrix, transpose);
	}
};

class Camera : public Object
{
	public :
		Camera(CameraPreset preset);
		
		Camera(UINT width, UINT height, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ);

		~Camera();

		virtual void Render() override;

		void SetActiveCamera();

		UINT ViewportWidth();
		UINT ViewportHeight();

	protected : 
		virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
		virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	private :
		bool SetupCamera();
		void SetViewport();
		bool CreateBuffers();

		void UpdateProjection();

		UINT width;
		UINT height;
		UINT topLeftX;
		UINT topLeftY;

		D3D11_VIEWPORT viewport;
		
		float FovAngleY;
		float AspectRatio;
		float NearZ;
		float FarZ;

		bool projModified;

		ID3D11Buffer* projectionBuffer;
		
};