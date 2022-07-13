#include "Camera.h"

#include <iostream>

#include "Pipeline.h"

Camera::Camera(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float NearZ, float FarZ) : width(widthPixels), height(heightPixels), topLeftX(topLeftX), topLeftY(topLeftY), NearZ(NearZ), FarZ(FarZ), projectionBuffer(nullptr)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create camera transform Buffer" << std::endl;
	}
	if (!SetupCamera())
	{
		std::cerr << "Failed to set up camera!" << std::endl;
	}
}

Camera::Camera(UINT width, UINT height, UINT topLeftX, UINT topLeftY, float NearZ, float FarZ)
{
}

Camera::~Camera()
{
	worldTransformBuffer->Release();
	projectionBuffer->Release();
}

void Camera::SetActiveCamera()
{
	Pipeline::Deferred::GeometryPass::Set::Viewport(viewport);

	UpdateTransformBuffer();
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraViewBuffer(worldTransformBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewBuffer(worldTransformBuffer);

	UpdateProjection();
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraProjectionBuffer(projectionBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraProjectionBuffer(projectionBuffer);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewportBuffer(viewBuffer);
}

void Camera::SetNearPlane(float nearZ)
{
	NearZ = nearZ;
	ProjModified();
}

void Camera::SetFarPlane(float farZ)
{
	FarZ = farZ;
	ProjModified();
}

UINT Camera::ViewportWidth()
{
	return width;
}

UINT Camera::ViewportHeight()
{
	return height;
}

UINT Camera::ViewportTopLeftX()
{
	return topLeftX;
}

UINT Camera::ViewportTopLeftY()
{
	return topLeftY;
}

DirectX::XMFLOAT4X4 Camera::TransformMatrix()
{
	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(scaling * rotation * translation));

	return output;
}

DirectX::XMFLOAT4X4 Camera::InverseTransformMatrix()
{
	DirectX::XMMATRIX invScaling = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);

	DirectX::XMMATRIX invRotation = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&rotationMatrix));

	DirectX::XMMATRIX invTranslation = DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);

	DirectX::XMFLOAT4X4 output;

	DirectX::XMStoreFloat4x4(&output, DirectX::XMMatrixTranspose(invScaling * invRotation * invTranslation));

	return output;
}

void Camera::ProjModified()
{
	projModified = true;
}

float Camera::AspectRatio()
{
	return (float)width / (float)height;
}

bool Camera::SetupCamera()
{
	SetViewport();
	if (!CreateBuffers())
	{
		return false;
	}

	return true;
}

void Camera::SetViewport()
{
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
}

bool Camera::CreateBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;

	projModified = false;
	void* projStruct = nullptr;
	UINT structSize;
	ProjBufferData(projStruct, structSize);

	bufferDesc.ByteWidth = structSize;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = projStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &projectionBuffer)))
	{
		return false;
	}

	if (projStruct != nullptr) delete projStruct;

	bufferDesc.ByteWidth = sizeof(ViewBuffer);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.CPUAccessFlags = 0;

	ViewBuffer viewStruct = ViewBuffer(width, height, topLeftX, topLeftY);

	data.pSysMem = &viewStruct;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &viewBuffer)))
	{
		return false;
	}

	return true;
}

void Camera::UpdateProjection()
{
	if (projModified)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		
		void* projStruct = nullptr;
		UINT structSize;
		ProjBufferData(projStruct, structSize);

		Pipeline::ResourceManipulation::MapBuffer(projectionBuffer, &mappedResource);
		memcpy(mappedResource.pData, projStruct, sizeof(structSize));
		Pipeline::ResourceManipulation::UnmapBuffer(projectionBuffer);

		if (projStruct != nullptr) delete projStruct;

		projModified = false;
	}
}

CameraPerspective::CameraPerspective(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ) :
	Camera(widthPixels, heightPixels, topLeftX, topLeftY, NearZ, FarZ), 
	FovAngleY(FovAngleY), 
	aspectRatio(AspectRatio())
{
}

void CameraPerspective::SetFOV(float fovAngleY)
{
	FovAngleY = fovAngleY;
	ProjModified();
}

void CameraPerspective::Render()
{
}

void CameraPerspective::DepthRender()
{
}

void CameraPerspective::ProjBufferData(void* data, UINT& dataSize)
{
	data = new PerspectiveBuffer(FovAngleY, aspectRatio, ViewportWidth(), ViewportHeight(), NearZ, FarZ);
	dataSize = sizeof(PerspectiveBuffer);
}

CameraOrthographic::CameraOrthographic(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float viewWidth, float NearZ, float FarZ) :
	Camera(widthPixels, heightPixels, topLeftX, topLeftY, NearZ, FarZ),
	WidthScale(viewWidth),
	HeightScale(viewWidth / AspectRatio())
{
}

void CameraOrthographic::SetViewWidth(float viewWidth)
{
	WidthScale = viewWidth;
	HeightScale = viewWidth / AspectRatio();
}

void CameraOrthographic::Render()
{
}

void CameraOrthographic::DepthRender()
{
}

void CameraOrthographic::ProjBufferData(void* data, UINT& dataSize)
{
	data = new OrthographicBuffer(WidthScale, HeightScale, ViewportWidth(), ViewportHeight(), NearZ, FarZ);
	dataSize = sizeof(OrthographicBuffer);
}
