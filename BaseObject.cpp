#include "BaseObject.h"

#include "Pipeline.h"


void Object::Scale(const std::array<float, 3>& scaling, bool transformSpace, bool transformMode)
{
	DirectX::XMFLOAT3 newScale = DirectX::XMFLOAT3(scaling[0], scaling[1], scaling[2]);
	
	if (transformSpace)
	{

		DirectX::XMVECTOR scaleVector = DirectX::XMLoadFloat3(&newScale);
		DirectX::XMMATRIX rotationTransform = DirectX::XMLoadFloat4x4(&rotationMatrix);
		scaleVector = DirectX::XMVector3Transform(scaleVector, rotationTransform);

		DirectX::XMFLOAT3 scaleTransformed;

		DirectX::XMStoreFloat3(&scaleTransformed, scaleVector);

		if (newScale.x * scaleTransformed.x < 0)
		{
			scaleTransformed.x *= -1;
		}
		if (newScale.y * scaleTransformed.y < 0)
		{
			scaleTransformed.y *= -1;
		}
		if (newScale.z * scaleTransformed.z < 0)
		{
			scaleTransformed.z *= -1;
		}

		newScale = scaleTransformed;
	}

	if (transformMode)
	{
		scale = newScale;
	}
	else
	{
		scale.x += newScale.x;
		scale.y += newScale.y;
		scale.z += newScale.z;
	}

	transformed = true;
}

void Object::Rotate(const std::array<float, 3>& rotation, bool transformSpace, bool transformMode, float rotationUnit)
{
	DirectX::XMMATRIX currentTransform = DirectX::XMLoadFloat4x4(&rotationMatrix);

	DirectX::XMMATRIX transformation = DirectX::XMMatrixRotationRollPitchYaw(rotation[0] * rotationUnit, rotation[1] * rotationUnit, rotation[2] * rotationUnit);

	if (transformSpace)
	{
		transformation = DirectX::XMMatrixTranspose(currentTransform) * transformation * currentTransform;
	}

	if (transformMode)
	{
		currentTransform = DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	}

	currentTransform *= transformation;

	DirectX::XMStoreFloat4x4(&rotationMatrix, currentTransform);

	transformed = true;
}

void Object::Translate(const std::array<float, 3>& translation, bool transformSpace, bool transformMode)
{
	DirectX::XMFLOAT3 newTranslation = DirectX::XMFLOAT3(translation[0], translation[1], translation[2]);

	if (transformSpace)
	{
		DirectX::XMVECTOR translationVector = DirectX::XMLoadFloat3(&newTranslation);
		DirectX::XMMATRIX rotationTransform = DirectX::XMLoadFloat4x4(&rotationMatrix);
		translationVector = DirectX::XMVector3Transform(translationVector, rotationTransform);

		DirectX::XMStoreFloat3(&newTranslation, translationVector);
	}

	if (transformMode)
	{
		position = newTranslation;
	}
	else
	{
		position.x += newTranslation.x;
		position.y += newTranslation.y;
		position.z += newTranslation.z;
	}

	transformed = true;
}

bool Object::CreateTransformBuffer()
{
	transformed = false;

	DirectX::XMFLOAT4X4 matrices[2];
	
	DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(TransformMatrix());

	DirectX::XMStoreFloat4x4(&matrices[0], transpose);

	transpose = DirectX::XMMatrixTranspose(InverseTransformMatrix());

	DirectX::XMStoreFloat4x4(&matrices[1], transpose);

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = matrices;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	HRESULT hr = Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &worldTransformBuffer);
	return !FAILED(hr);
}

void Object::UpdateTransformBuffer()
{
	if (transformed)
	{
		DirectX::XMFLOAT4X4 matrices[2];

		DirectX::XMMATRIX transpose = DirectX::XMMatrixTranspose(TransformMatrix());

		DirectX::XMStoreFloat4x4(&matrices[0], transpose);

		transpose = DirectX::XMMatrixTranspose(InverseTransformMatrix());

		DirectX::XMStoreFloat4x4(&matrices[1], transpose);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		Pipeline::Map::Buffer(worldTransformBuffer, &mappedResource);
		memcpy(mappedResource.pData, matrices, sizeof(DirectX::XMFLOAT4X4) * 2);
		Pipeline::Unmap::Buffer(worldTransformBuffer);

		transformed = false;
	}
}

DirectX::XMMATRIX Object::TransformMatrix()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);

	return DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) * rotation * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}

DirectX::XMMATRIX Object::InverseTransformMatrix()
{
	DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&rotationMatrix);

	return DirectX::XMMatrixScaling((scale.x != 0) ? 1.0f / scale.x : 0.0f, (scale.y != 0) ? 1.0f / scale.y : 0.0f, (scale.z != 0) ? 1.0f / scale.z : 0.0f) * DirectX::XMMatrixTranspose(rotation) * DirectX::XMMatrixTranslation(-position.x, -position.y, -position.z);
}
