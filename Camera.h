#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>

#include "BaseObject.h"

/*
width = 1024;
height = 576;
topLeftX = 0;
topLeftY = 0;

FovAngleY = 1.5f;
AspectRatio = width / height;
NearZ = 0.01f;
FarZ = 20.0f;
*/

struct PerspectiveBuffer
{
	DirectX::XMFLOAT4X4 projectionMatrix;
	
	float widthScalar;
	float heightScalar;

	float projectionConstantA;
	float projectionConstantB;

	PerspectiveBuffer(float FovAngleY, float AspectRatio, UINT width, UINT height, float NearZ, float FarZ) : 

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

struct OrthographicBuffer
{
	DirectX::XMFLOAT4X4 projectionMatrix;

	float widthScalar;
	float heightScalar;

	float projectionConstantA;
	float projectionConstantB;

	OrthographicBuffer(float viewWidth, float viewHeight, UINT width, UINT height, float NearZ, float FarZ) :

		widthScalar(2/viewWidth),
		heightScalar(2/viewHeight),

		projectionConstantA(1 / (FarZ - NearZ)),
		projectionConstantB(NearZ / (NearZ - FarZ)),

		projectionMatrix(DirectX::XMFLOAT4X4())
	{
		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, NearZ, FarZ));

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
		Camera(UINT width, UINT height, UINT topLeftX, UINT topLeftY, float NearZ, float FarZ);

		~Camera();

		void SetActiveCamera();

		void SetNearPlane(float nearZ);
		void SetFarPlane(float farZ);

		UINT ViewportWidth();
		UINT ViewportHeight();
		UINT ViewportTopLeftX();
		UINT ViewportTopLeftY();

		void UpdateProjection();

	protected : 
		virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
		virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

		virtual void ProjBufferData(void* data, UINT& dataSize) = 0;

		void ProjModified();

		float AspectRatio();

		float NearZ;
		float FarZ;

	private :
		bool SetupCamera();
		void SetViewport();
		bool CreateBuffers();

		UINT width;
		UINT height;
		UINT topLeftX;
		UINT topLeftY;

		D3D11_VIEWPORT viewport;

		bool projModified;

		ID3D11Buffer* projectionBuffer;

		ID3D11Buffer* viewBuffer;
};

class CameraPerspective : public Camera
{
public:
	CameraPerspective(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ);

	void SetFOV(float fovAngleY);

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void ProjBufferData(void* data, UINT& dataSize) override;

private:
	float FovAngleY;
	float aspectRatio;
};

class CameraOrthographic : public Camera
{
public:
	CameraOrthographic(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float viewWidth, float NearZ, float FarZ);

	void SetViewWidth(float viewWidth);

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void ProjBufferData(void* data, UINT& dataSize) override;

private:
	float WidthScale;
	float HeightScale;
};