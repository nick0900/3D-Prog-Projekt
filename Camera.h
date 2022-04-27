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
	float projectionConstantB;

	ProjBuffer(float FovAngleY, float AspectRatio, UINT width, UINT height, float NearZ, float FarZ) : 

	widthScalar(tan(FovAngleY / 2.0f)),
	heightScalar(widthScalar * AspectRatio), 

	projectionConstantA(FarZ / (FarZ - NearZ)), 
	projectionConstantB((-NearZ * FarZ) / (FarZ - NearZ)),

	projectionMatrix(DirectX::XMFLOAT4X4())
	{
		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ));

		DirectX::XMStoreFloat4x4(&projectionMatrix, transpose);
	}
};

struct ViewBuffer
{
	float width;
	float height;
	UINT topLeftX;
	UINT topLeftY;

	ViewBuffer(UINT width, UINT height, UINT topLeftX, UINT topLeftY) :
	width(width),
	height(height),

	topLeftX(topLeftX),
	topLeftY(topLeftY)
	{}
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
		UINT ViewportTopLeftX();
		UINT ViewportTopLeftY();

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

		ID3D11Buffer* viewBuffer;
};