#pragma once
#include <string>
#include <vector>
#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "BaseObject.h"
#include "SharedResources.h"
#include "QuadTree.h"

struct Vertex {
	float pos[3];
	float norm[3];
	float uv[2];

	Vertex(const std::array<float, 3>& position, const std::array<float, 3>& normal, const std::array<float, 2>& uvCoords)
	{
		for (int i = 0; i < 3; i++)
		{
			pos[i] = position[i];
			norm[i] = normal[i];
		}
		for (int i = 0; i < 2; i++)
		{
			uv[i] = uvCoords[i];
		}
	}
};

struct Submesh {
	int Start = 0;
	int size = 0;
	int material = 0;
};

class STDOBJ : public Object
{
	public:
		STDOBJ(const std::string OBJFilepath);

		~STDOBJ();

		virtual void Render() override;
		virtual void DepthRender() override;

		virtual bool Contained(DirectX::BoundingFrustum& viewFrustum) override;
		virtual void AddToQuadTree(QuadTree* tree) override;

	protected:
		std::vector<Submesh> submeshes;
		ID3D11Buffer* vertexBuffer;
		ID3D11Buffer* indexBuffer;

	private:
		DirectX::BoundingSphere boundingVolume;

		bool LoadOBJ(std::string OBJFilepath);

		bool LoadMTL(std::string MTLFilepath);
};

struct TesselationConfigBufferStruct
{
	float maxTesselation;
	float maxDistance;
	float minDistance;
	float interpolationFactor;

	TesselationConfigBufferStruct(float maxTesselation, float maxDistance, float minDistance, float interpolationFactor) :
		maxTesselation(maxTesselation),
		maxDistance(maxDistance),
		minDistance(minDistance),
		interpolationFactor(interpolationFactor)
	{
	}
};

class STDOBJTesselated : public STDOBJ
{
public:
	STDOBJTesselated(const std::string OBJFilepath, float maxTesselation, float maxDistance, float minDistance, float interpolationFactor);
	~STDOBJTesselated();

	virtual void Render() override;
	virtual void DepthRender() override;

private:
	ID3D11Buffer* tesselationConfigBuffer;
};

class DeferredRenderer;

class STDOBJMirror : public STDOBJ
{
public:
	STDOBJMirror(const std::string OBJFilepath, UINT resolution, DeferredRenderer* renderer, float nearPlane, float farPlane);
	~STDOBJMirror();

	virtual void Render() override;

	void ReflectionRender();

private:
	UINT resolution;

	float nearPlane;
	float farPlane;

	DeferredRenderer* renderer;

	ID3D11UnorderedAccessView* UAVs[6];
	ID3D11Texture2D* textureCube;
	ID3D11ShaderResourceView* SRV;

	bool blockRender;
};
