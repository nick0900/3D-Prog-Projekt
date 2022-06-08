#pragma once
#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>

#include "BaseObject.h"

//Revise whole shadowmap management. Make single shadowmap class with different states for mapping modes and supporting general resolutions. Use vector of texture arrays pointers to order and bind shadowmaps. light parameters will tell what srv position and what array index it has, aswell what mapping mode to use.
//renderer will have own camera for depth mapping and will query settings from each light with a shadowmap.

#define SMALLEST_PARAM_CAPACITY 8
#define SMALLES_SHADOW_CAPACITY 4

enum LightType
{
	MissingLight = -1,
	PointLight = 0,
	SpotLight = 1,
	DirectionalLight = 2,
	NumberOfLightTypes = 3 // the number of lights excluding missing light
};

enum ShadowMapType
{
	Single320X320 = 0
};

enum PointLightShadowMapType
{
	Single340X340 = 0
};

struct GeneralLightInfoBuffer
{
	int lightCount;
	int padding[3];

};

struct LightBufferStruct
{
	int lightType;
	int shadowMapIndex; //index of subresource that contains correct map. If -1 then don't cast shadows
	int shadowMapTypeIndex; //Tells shader where texturearray is located
	int mappingMode; //tells shader what kind of sampling to perform
	float falloff;	//Falloff for point/spot -lights
	float lightFOV; //specific to spotlight for frustrum. Also used for light camera fov to render shadow maps
	float diffuse[3];
	float specular[3];
	float position[3];
	float direction[3];
	DirectX::XMFLOAT4X4 invTransform; //Needed for shadow mapping

	float padding[2];

	LightBufferStruct() :
		lightType(LightType::MissingLight),
		lightFOV(0.0f),
		falloff(0.0f),
		shadowMapIndex(0),
		shadowMapTypeIndex(0),
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

class LightBase : public Object
{
public:
	LightBase();
	~LightBase();

	bool SetupBase();
	void ReleaseBase();
	void BindBase();

	bool ShadowMapCreate(int mapType);
	void ShadowMapDestroy();
	bool HasShadowMap();
	void SetCastShadows(bool castShadows);
	bool CastShadowsStatus();
	void RenderShadowMap();

	void SetLightDiffuse(const std::array<float, 3>& diffuse);
	void SetLightSpecular(const std::array<float, 3>& specular);

	void Bind();
	void Unbind();

	void ParamMove(int newPosition);

protected:
	void SetParams(LightBufferStruct& parameters);
	void BindParams();
	void UnbindParams();

	void LightFOV(float angle);
	void Falloff(float falloff);

	virtual ShadowHandlerBase* SpecificShadowOptions(int mapType) = 0;

	virtual void OnModyfied() override;
	virtual void LightDirection(DirectX::XMFLOAT3& direction) = 0;

private:
	bool transformed;
	bool paramsModified;
	LightBufferStruct parameters;
	bool UpdateParams();
	ID3D11Buffer* paramsBuffer;

	ShadowHandlerBase* shadowMap = nullptr;

	int arrayPosition;
	int FreePosition();

	static UINT paramCount;
	static UINT paramCapacity;
	static LightBase** slotTracker;
	static ID3D11Buffer* paramsStructuredBuffer;

	bool Reallocate(UINT newCapacity);
};

class ShadowHandlerBase
{
public:
	ShadowHandlerBase();
	~ShadowHandlerBase();

	bool SetupShadows();
	void ReleaseShadows();
	virtual void BindShadows() = 0;

	virtual int ShadowTypeIndex() = 0;

	int ShadowResourceIndex();

	virtual void RenderShadowMap() = 0;

	void BindShadowMap();
	void UbindShadowMap();

	void ShadowMove(int newPosition);

	bool SetupComplete();

protected:
	void UpdateShadowMap();

	bool shadowMapUpdated;
	ID3D11Texture2D* shadowMap;

	virtual void ShadowMapDesc(D3D11_TEXTURE2D_DESC& textureDesc) = 0;
	virtual void ShadowMapArraySRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) = 0;

	virtual UINT& StaticCount() = 0;
	virtual UINT& StaticCapacity() = 0;
	virtual ShadowHandlerBase**& StaticSlotTracker() = 0;

	virtual ID3D11Texture2D*& StaticShadowMapArray() = 0;
	virtual ID3D11ShaderResourceView*& StaticShadowMapArraySRV() = 0;

	bool setupComplete;

private:
	int arrayPosition;
	int FreePosition();

	bool Reallocate(UINT newCapacity);
};

class ShadowMapSingle320X320 : public ShadowHandlerBase
{
public:
	ShadowMapSingle320X320();
	~ShadowMapSingle320X320();

	virtual void BindShadows() override;

	virtual int ShadowTypeIndex() override;

	virtual void RenderShadowMap() override;

protected:
	virtual void ShadowMapDesc(D3D11_TEXTURE2D_DESC& textureDesc) override;
	virtual void ShadowMapArraySRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc) override;

	virtual UINT& StaticCount() override;
	virtual UINT& StaticCapacity() override;
	virtual ShadowHandlerBase**& StaticSlotTracker() override;

	virtual ID3D11Texture2D*& StaticShadowMapArray() override;
	virtual ID3D11ShaderResourceView*& StaticShadowMapArraySRV() override;

private:
	ID3D11DepthStencilView* mapDSV;

	static UINT count;
	static UINT capacity;
	static ShadowHandlerBase** slotTracker;

	static ID3D11Texture2D* shadowMapArray;
	static ID3D11ShaderResourceView* shadowMapArraySRV;
};

class PointLight : public LightBase
{
public:
	PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;
	
	virtual void LightDirection(DirectX::XMFLOAT3& direction) override;

private:
};