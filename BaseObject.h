#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <array>

#include "QuadTree.h"

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
		void Rotate(DirectX::XMFLOAT4& quaternion, bool transformSpace, bool transformMode);
		
		void Translate(const std::array<float, 3>& translation, bool transformSpace, bool transformMode);

		virtual void Render() = 0;
		virtual void DepthRender() = 0;

		void UpdateTransformBuffer();

		virtual DirectX::XMFLOAT4X4 TransformMatrix();
		virtual DirectX::XMFLOAT4X4 InverseTransformMatrix();

		virtual bool Contained(DirectX::BoundingFrustum& viewFrustum);

		virtual void AddToQuadTree(QuadTree* tree);

	protected :
		bool CreateTransformBuffer();
		
		ID3D11Buffer* worldTransformBuffer;

		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT4 rotationQuaternion;
		DirectX::XMFLOAT3 position;

		virtual void OnModyfied();
		
	private :
		bool transformed;
};
