#pragma once
#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>

#include "BaseObject.h"

enum LightType
{
	MissingLight = -1,
	PointLight = 0,
	SpotLight = 1,
	DirectionalLight = 2,
	NumberOfLightTypes = 3 // the number of lights excluding missing light
};

struct LightBufferStruct
{
	int lightType;
	int useShadowMap; //Cast shadows if = 1
	int shadowMapIndex; //index of subresource thhat contains correct map
	int shadowMapSpecialType; //if a light can have different shadow map structures this informs the shader what to do
	float angleLimit; //specific to spotlight for frustrum
	float diffuse[3];
	float padding;
	float specular[3];


	LightBufferStruct(LightType type, float anglelimit, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular) : 
		lightType(type), 
		angleLimit(angleLimit),
		useShadowMap(0),
		shadowMapIndex(0),
		shadowMapSpecialType(0),
		padding(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
		}
	}

	LightBufferStruct() :
		lightType(LightType::MissingLight),
		angleLimit(0.0f),
		useShadowMap(0),
		shadowMapIndex(0),
		shadowMapSpecialType(0),
		padding(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = 0.0f;
			this->specular[i] = 0.0f;
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

	virtual void Bind() = 0;
	virtual void Unbind() = 0;

	void ParamMove(int newPosition);

protected:
	void SetParams(LightBufferStruct& parameters);
	void BindParams();
	void UnbindParams();

private:
	bool paramsModified;
	LightBufferStruct parameters;
	bool UpdateParams();

	ID3D11Buffer* paramsBuffer;

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

protected:
	void UpdateShadowMap();

	ID3D11RenderTargetView* ShadowMapRTV;

	virtual void ShadowMapDesc(D3D11_TEXTURE2D_DESC& textureDesc) = 0;
	virtual void ShadowMapArraySRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& textureDesc, UINT arraySize) = 0;
	virtual void ShadowMapRTVDesc(D3D11_RENDER_TARGET_VIEW_DESC& textureDesc) = 0;

	virtual UINT& StaticCount() = 0;
	virtual UINT& StaticCapacity() = 0;
	virtual ShadowHandlerBase**& StaticSlotTracker() = 0;

	virtual ID3D11Texture2D*& StaticShadowMapArray() = 0;
	virtual ID3D11ShaderResourceView*& StaticShadowMapArraySRV() = 0;

private:
	bool shadowMapUpdated;
	ID3D11Texture2D* shadowMap;

	int arrayPosition;
	int FreePosition();

	bool Reallocate(UINT newCapacity);
};

class ShadowMapSingle320X320 : public ShadowHandlerBase
{
public:
	virtual void BindShadows() override;

	virtual int ShadowTypeIndex() override;

	virtual void RenderShadowMap() override;

protected:
	virtual void ShadowMapDesc(D3D11_TEXTURE2D_DESC& textureDesc) override;
	virtual void ShadowMapArraySRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC& textureDesc, UINT arraySize) override;
	virtual void ShadowMapRTVDesc(D3D11_RENDER_TARGET_VIEW_DESC& textureDesc) override;

	virtual UINT& StaticCount() override;
	virtual UINT& StaticCapacity() override;
	virtual ShadowHandlerBase**& StaticSlotTracker() override;

	virtual ID3D11Texture2D*& StaticShadowMapArray() override;
	virtual ID3D11ShaderResourceView*& StaticShadowMapArraySRV() override;

private:
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

private:
	

	float pos[3];
	float padding;
	float col[3];
	float padding2;
};