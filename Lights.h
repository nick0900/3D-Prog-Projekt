#pragma once
#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>
#include <unordered_map>
#include <vector>

#include "BaseObject.h"
#include "Renderer.h"

//Revise whole shadowmap management. Make single shadowmap class with different states for mapping modes and supporting general resolutions. Use vector of texture arrays pointers to order and bind shadowmaps. light parameters will tell what srv position and what array index it has, aswell what mapping mode to use.
//renderer will have own camera for depth mapping and will query settings from each light with a shadowmap.

#define SMALLEST_PARAM_CAPACITY 8

#define MAX_BOUND_SHADOW_MAPS 128 - 5

enum LightType
{
	MissingLight = -1,
	TypePointLight = 0,
	TypeSpotLight = 1,
	TypeDirectionalLight = 2,
	NumberOfLightTypes = 3 // the number of lights excluding missing light
};

struct GeneralLightInfoBuffer
{
	int lightCount;
	int padding[3];

	GeneralLightInfoBuffer(UINT count) : lightCount(count) {}
};

struct LightBufferStruct
{
	int lightType;
	int shadowMapIndex; //index of subresource that contains correct map. If -1 then don't cast shadows
	int mappingMode; //tells shader what kind of sampling to perform
	float falloff;	//Falloff for point/spot -lights
	float lightFOV; //specific to spotlight for frustrum. Also used for light camera fov to render shadow maps
	float diffuse[3];
	float specular[3];
	float position[3];
	float direction[3];
	DirectX::XMFLOAT4X4 invTransform; //Needed for shadow mapping
	DirectX::XMFLOAT4X4 projMatrix; //Needed for shadow mapping

	float padding[3];

	LightBufferStruct(int lightType, float lightFOV, float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position, const std::array<float, 3>& direction, DirectX::XMFLOAT4X4 invTransform, DirectX::XMFLOAT4X4 projMatrix) :
		lightType(lightType),
		lightFOV(lightFOV),
		falloff(falloff),
		shadowMapIndex(-1),
		mappingMode(-1),
		invTransform(invTransform),
		projMatrix(projMatrix)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = position[i];
			this->direction[i] = direction[i];
			this->padding[i] = 0.0f;
		}
	}

	LightBufferStruct() :
		lightType(LightType::MissingLight),
		lightFOV(0.0f),
		falloff(0.0f),
		shadowMapIndex(-1),
		mappingMode(-1),
		invTransform()
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = 0.0f;
			this->specular[i] = 0.0f;
			this->position[i] = 0.0f;
			this->direction[i] = 0.0f;
			this->padding[i] = 0.0f;
		}
	}
};

class Shadowmap
{
public:
	Shadowmap(UINT resolution);
	~Shadowmap();

	virtual MappingMode ShadowmapMappingMode() = 0;
	int Resolution();

	ID3D11ShaderResourceView* mapSRV;

	virtual ID3D11DepthStencilView** DSVArray() = 0;

protected:
	virtual void Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc) = 0;
	virtual void SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) = 0;

	ID3D11Texture2D* mapTexture;

private:
	UINT resolution;
};

class ShadowMapSingle : public Shadowmap
{
public:
	ShadowMapSingle(UINT resolution);
	~ShadowMapSingle();

	virtual MappingMode ShadowmapMappingMode() override;

	virtual ID3D11DepthStencilView** DSVArray() override;

protected:
	virtual void Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc) override;
	virtual void SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) override;

private:
	ID3D11DepthStencilView* mapDSV;
};

class ShadowMapDouble : public Shadowmap
{
public:
	ShadowMapDouble(UINT resolution);
	~ShadowMapDouble();

	virtual MappingMode ShadowmapMappingMode() override;

	virtual ID3D11DepthStencilView** DSVArray() override;

protected:
	virtual void Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc) override;
	virtual void SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) override;

private:
	ID3D11DepthStencilView* mapDSV[2];
};

class ShadowMapCube : public Shadowmap
{
public:
	ShadowMapCube(UINT resolution);
	~ShadowMapCube();

	virtual MappingMode ShadowmapMappingMode() override;

	virtual ID3D11DepthStencilView** DSVArray() override;

protected:
	virtual void Texture2DDesc(D3D11_TEXTURE2D_DESC& textureDesc) override;
	virtual void SRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) override;

private:
	ID3D11DepthStencilView* mapDSV[6];
};

class LightBase : public Object
{
public:
	LightBase();
	~LightBase();

	void GetParameters(LightBufferStruct* dest);

	void DeleteShadowMap();

	bool CastShadows();
	void SetCastShadows(bool castShadows);

	ID3D11ShaderResourceView* ShadowmapSRV();

	std::array<float, 3> lightDiffuse;
	std::array<float, 3> lightSpecular;

	virtual Camera* ShadowMapCamera() = 0;
	ID3D11DepthStencilView** ShadowmapDSVs();
	MappingMode ShadowMappingMode();

protected:
	void SetupShadowmap(MappingMode mapType, UINT resolution);

	virtual LightType Type() = 0;

	virtual float LightFOV() = 0;
	virtual float LightFalloff() = 0;

	virtual std::array<float, 3> LightDirection() = 0;
	virtual std::array<float, 3> LightPosition() = 0;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() = 0;

	Shadowmap* shadowmap;
private:
	bool castShadows;
};

class LightBinder
{
public:
	LightBinder();
	~LightBinder();

	void Bind();

	void AddLight(LightBase* light);
	void ClearLights();

private:
	ID3D11Buffer* LightsGeneralData;

	ID3D11Buffer* AllocateParamBuffer(UINT capacity);

	UINT paramCapacity;
	ID3D11Buffer* paramsStructuredBuffer;
	std::vector<LightBase*> boundLights;
};

class PointLight : public LightBase
{
public:
	PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float fallof);

	void CreateShadowMap(MappingMode mapType, UINT resolution, float nearPlane, float farPlane);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

	float fallof;

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual LightType Type() override;

	virtual float LightFOV() override;
	virtual float LightFalloff() override;

	virtual std::array<float, 3> LightDirection() override;
	virtual std::array<float, 3> LightPosition() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	float nearPlane;
	float farPlane;
};

class DirectionalLight : public LightBase
{
public:
	DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);

	void CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual LightType Type() override;

	virtual float LightFOV() override;
	virtual float LightFalloff() override;

	virtual std::array<float, 3> LightDirection() override;
	virtual std::array<float, 3> LightPosition() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	float viewWidth;
	float nearPlane;
	float farPlane;
};

class SpotLight : public LightBase
{
public:
	SpotLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof);

	void CreateShadowMap(UINT resolution, float nearPlane, float farPlane);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

	float lightFOV;
	float fallof;

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual LightType Type() override;

	virtual float LightFOV() override;
	virtual float LightFalloff() override;

	virtual std::array<float, 3> LightDirection() override;
	virtual std::array<float, 3> LightPosition() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	float nearPlane;
	float farPlane;
};
