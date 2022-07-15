#include "Camera.h"

#include <iostream>

#include "Pipeline.h"

Camera::Camera(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float NearZ, float FarZ) : width(widthPixels), height(heightPixels), topLeftX(topLeftX), topLeftY(topLeftY), NearZ(NearZ), FarZ(FarZ), projectionBuffer(nullptr), projModified(false)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create camera transform Buffer" << std::endl;
	}

	D3D11_BUFFER_DESC bufferDesc;

	ViewBuffer viewStruct = ViewBuffer(width, height, topLeftX, topLeftY);

	bufferDesc.ByteWidth = sizeof(ViewBuffer);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &viewStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &viewBuffer)))
	{
		std::cerr << "Failed to create camera viewport buffer!" << std::endl;
	}
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

	if (projModified)
	{
		UpdateProjection();
		projModified = false;
	}
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraProjectionBuffer(projectionBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraProjectionBuffer(projectionBuffer);

	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewportBuffer(viewBuffer);
}

void Camera::SetNearPlane(float nearZ)
{
	NearZ = nearZ;
	FlagProjChange();
}

void Camera::SetFarPlane(float farZ)
{
	FarZ = farZ;
	FlagProjChange();
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

void Camera::FlagProjChange()
{
	projModified = true;
}

CameraPerspective::CameraPerspective(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ) :
	Camera(widthPixels, heightPixels, topLeftX, topLeftY, NearZ, FarZ), 
	FovAngleY(FovAngleY), 
	aspectRatio(AspectRatio())
{
	if (!SetupCamera())
	{
		std::cerr << "Failed to set up camera!" << std::endl;
	}
}

void CameraPerspective::SetFOV(float fovAngleY)
{
	FovAngleY = fovAngleY;
	FlagProjChange();
}

void CameraPerspective::Render()
{
}

void CameraPerspective::DepthRender()
{
}

void CameraPerspective::UpdateProjection()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	PerspectiveBuffer bufferStruct = PerspectiveBuffer(FovAngleY, aspectRatio, ViewportWidth(), ViewportHeight(), NearZ, FarZ);

	Pipeline::ResourceManipulation::MapBuffer(projectionBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(projectionBuffer);
}

bool CameraPerspective::CreateBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;
	
	PerspectiveBuffer bufferStruct = PerspectiveBuffer(FovAngleY, aspectRatio, ViewportWidth(), ViewportHeight(), NearZ, FarZ);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &projectionBuffer)))
	{
		return false;
	}

	return true;
}

CameraOrthographic::CameraOrthographic(UINT widthPixels, UINT heightPixels, UINT topLeftX, UINT topLeftY, float viewWidth, float NearZ, float FarZ) :
	Camera(widthPixels, heightPixels, topLeftX, topLeftY, NearZ, FarZ),
	WidthScale(viewWidth),
	HeightScale(viewWidth / AspectRatio())
{
	if (!SetupCamera())
	{
		std::cerr << "Failed to set up camera!" << std::endl;
	}
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

void CameraOrthographic::UpdateProjection()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	OrthographicBuffer bufferStruct = OrthographicBuffer(WidthScale, HeightScale, ViewportWidth(), ViewportHeight(), NearZ, FarZ);

	Pipeline::ResourceManipulation::MapBuffer(projectionBuffer, &mappedResource);
	memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
	Pipeline::ResourceManipulation::UnmapBuffer(projectionBuffer);
}

bool CameraOrthographic::CreateBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;

	OrthographicBuffer bufferStruct = OrthographicBuffer(WidthScale, HeightScale, ViewportWidth(), ViewportHeight(), NearZ, FarZ);

	bufferDesc.ByteWidth = sizeof(bufferStruct);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &projectionBuffer)))
	{
		return false;
	}

	return true;
}
