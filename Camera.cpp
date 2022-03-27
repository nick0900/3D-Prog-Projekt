#include "Camera.h"

#include <iostream>

#include "Pipeline.h"

Camera::Camera(CameraPreset preset) : projectionBuffer(nullptr)
{
	switch (preset)
	{
	case CameraPreset::HDstandard :
		width = 1024;
		height = 576;
		topLeftX = 0;
		topLeftY = 0;

		FovAngleY = 1.5f;
		AspectRatio = width / height;
		NearZ = 0.01f;
		FarZ = 20.0f;
		break;
	}

	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create camera transform Buffer" << std::endl;
	}
	if (!SetupCamera())
	{
		std::cerr << "Failed to set up camera!" << std::endl;
	}
}

Camera::Camera(UINT width, UINT height, UINT topLeftX, UINT topLeftY, float FovAngleY, float NearZ, float FarZ) : width(width), height(height), topLeftX(topLeftX), topLeftY(topLeftY), FovAngleY(FovAngleY), AspectRatio((float)width / (float)height), NearZ(NearZ), FarZ(FarZ), projectionBuffer(nullptr)
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

Camera::~Camera()
{
	worldTransformBuffer->Release();
	projectionBuffer->Release();
}

void Camera::Render()
{
}

void Camera::SetActiveCamera()
{
	Pipeline::Deferred::GeometryPass::Set::Viewport(viewport);

	UpdateTransformBuffer();
	//Transformbuffer() is overridden to be given as the inverse of camera position and rotation. which is the viewmatrix
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraViewBuffer(worldTransformBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraViewBuffer(worldTransformBuffer);

	UpdateProjection();
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::cameraProjectionBuffer(projectionBuffer);
	Pipeline::Deferred::LightPass::ComputeShader::Bind::CameraProjectionBuffer(projectionBuffer);
}

UINT Camera::ViewportWidth()
{
	return width;
}

UINT Camera::ViewportHeight()
{
	return height;
}

DirectX::XMMATRIX Camera::TransformMatrix()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);

	return rotation * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}

DirectX::XMMATRIX Camera::InverseTransformMatrix()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);

	return DirectX::XMMatrixTranspose(rotation) * DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);
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

	bufferDesc.ByteWidth = sizeof(ProjBuffer);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	projModified = false;
	ProjBuffer bufferStruct = ProjBuffer(FovAngleY, AspectRatio, width, height, NearZ, FarZ);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	HRESULT hr = Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &projectionBuffer);

	return !FAILED(hr);
}

void Camera::UpdateProjection()
{
	if (projModified)
	{
		ProjBuffer bufferStruct = ProjBuffer(FovAngleY, AspectRatio, width, height, NearZ, FarZ);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		Pipeline::Map::Buffer(projectionBuffer, &mappedResource);
		memcpy(mappedResource.pData, &bufferStruct, sizeof(bufferStruct));
		Pipeline::Unmap::Buffer(projectionBuffer);

		projModified = false;
	}
}
