#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <array>

#define OBJECT_TRANSFORM_SPACE_LOCAL true
#define OBJECT_TRANSFORM_SPACE_GLOBAL false

#define PI 3.141592653589793238462643383279502884

#define OBJECT_ROTATION_UNIT_DEGREES PI / 180.0f
#define OBJECT_ROTATION_UINT_RADIANS 1.0f

#define OBJECT_TRANSFORM_REPLACE true
#define OBJECT_TRANSFORM_APPEND false

class Object
{
	public :
		void Scale(const std::array<float, 3>& scaling, bool transformSpace, bool transformMode);
		void Rotate(const std::array<float, 3>& rotation, bool transformSpace, bool transformMode, float rotationUnit);
		void Translate(const std::array<float, 3>& translation, bool transformSpace, bool transformMode);

		virtual void Render() = 0;

	protected :
		bool CreateTransformBuffer();
		void UpdateTransformBuffer();
		
		virtual DirectX::XMMATRIX TransformMatrix();
		virtual DirectX::XMMATRIX InverseTransformMatrix();
		
		ID3D11Buffer* worldTransformBuffer;

		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT4X4 rotationMatrix;
		DirectX::XMFLOAT3 position;
		
	private :
		bool transformed;
};
