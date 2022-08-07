#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>
#include <DirectXCollision.h>

#include "BaseObject.h"

struct PerspectiveBuffer
{
	DirectX::XMFLOAT4X4 projectionMatrix;
	
	float widthScalar;
	float heightScalar;

	float projectionConstantA;
	float projectionConstantB;

	PerspectiveBuffer(float FovAngleY, float AspectRatio, UINT width, UINT height, float NearZ, float FarZ)
	{
		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FovAngleY * OBJECT_ROTATION_UNIT_DEGREES, AspectRatio, NearZ, FarZ));

		DirectX::XMStoreFloat4x4(&projectionMatrix, transpose);

		widthScalar = 1.0f / projectionMatrix._11;
		heightScalar = 1.0f / projectionMatrix._22;

		projectionConstantA = projectionMatrix._33;
		projectionConstantB = projectionMatrix._34;
	}
};

struct OrthographicBuffer
{
	DirectX::XMFLOAT4X4 projectionMatrix;

	float widthScalar;
	float heightScalar;

	float projectionConstantA;
	float projectionConstantB;

	OrthographicBuffer(float viewWidth, float viewHeight, UINT width, UINT height, float NearZ, float FarZ)
	{
		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, NearZ, FarZ));

		DirectX::XMStoreFloat4x4(&projectionMatrix, transpose);

		widthScalar = 1.0f / projectionMatrix._11;
		heightScalar = 1.0f / projectionMatrix._22;

		projectionConstantA = projectionMatrix._33;
		projectionConstantB = projectionMatrix._34;
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

		virtual void ViewFrustum(DirectX::BoundingFrustum& frustum);

	protected : 
		virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
		virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

		virtual void UpdateProjection() = 0;

		void FlagProjChange();

		bool SetupCamera();
		virtual bool CreateBuffers() = 0;

		float AspectRatio();

		float NearZ;
		float FarZ;

		ID3D11Buffer* projectionBuffer;
		DirectX::BoundingFrustum viewFrustum;

	private :
		void SetViewport();

		UINT width;
		UINT height;
		UINT topLeftX;
		UINT topLeftY;

		D3D11_VIEWPORT viewport;

		bool projModified;

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
	virtual void UpdateProjection() override;
	virtual bool CreateBuffers() override;

private:
	float FovAngleY;
	float aspectRatio;
};

class CameraPerspectiveDebug : public CameraPerspective
{
public:
	CameraPerspectiveDebug(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ, CameraPerspective* frustumCamera);

	virtual void ViewFrustum(DirectX::BoundingFrustum& frustum) override;
private:
	CameraPerspective* frustumCamera;
};

class CameraOrthographic : public Camera
{
public:
	CameraOrthographic(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float viewWidth, float NearZ, float FarZ);

	void SetViewWidth(float viewWidth);

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void UpdateProjection() override;
	virtual bool CreateBuffers() override;

private:
	float WidthScale;
	float HeightScale;
};