#pragma once
#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <math.h>
#include <vector>

#include "BaseObject.h"
#include "Camera.h"
#include "SharedResources.h"

struct ShadowMappingBuffer
{
	DirectX::XMFLOAT4X4 invTransform;
	DirectX::XMFLOAT4X4 projMatrix;

	ShadowMappingBuffer(DirectX::XMFLOAT4X4 invTransform, DirectX::XMFLOAT4X4 projMatrix) : 
		invTransform(invTransform),
		projMatrix(projMatrix)
	{}
};


enum LightShaderMode
{
	LightTypeAmbientBasic = 0,
	LightTypePoint = 1,
	LightTypeDirectional = 2,
	LightTypeSpot = 3
};

struct AmbientLightBuffer
{
	float ambient[3];
	float padding;

	AmbientLightBuffer(const std::array<float, 3>& ambient) : padding(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->ambient[i] = ambient[i];
		}
	}
};

struct PointLightBuffer
{
	float diffuse[3];
	float falloff;
	
	float specular[3];
	float padding1;

	float position[3];
	float padding2;

	PointLightBuffer(float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position) :
		falloff(falloff), padding1(0.0f), padding2(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = position[i];
		}
	}
};

struct DirectionalLightBuffer
{
	float diffuse[3];
	float padding1;

	float specular[3];
	float padding2;

	float direction[3];
	float padding3;

	DirectionalLightBuffer(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& direction) :
		padding1(0.0f),
		padding2(0.0f),
		padding3(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->direction[i] = direction[i];
		}
	}
};

struct SpotLightBuffer
{
	float diffuse[3];
	float falloff;

	float specular[3];
	float lightFOV;

	float position[3];
	float padding1;

	float direction[3];
	float padding2;

	SpotLightBuffer(float lightFOV, float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position, const std::array<float, 3>& direction) :
		lightFOV(cosf(lightFOV * OBJECT_ROTATION_UNIT_DEGREES / 2)),
		falloff(falloff),
		padding1(0.0f),
		padding2(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = position[i];
			this->direction[i] = direction[i];
		}
	}
};

class Renderer;
class Shadowmap;

class LightBase : public Object
{
public:
	LightBase(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);
	~LightBase();

	void SetDiffuse(std::array<float, 3> lightDiffuse);
	std::array<float, 3> Diffuse();
	void SetSpecular(std::array<float, 3> lightSpecular);
	std::array<float, 3> Specular();

	void Bind();

	void DeleteShadowMap();
	bool CastingShadows();
	void SetCastShadows(bool castShadows);

	virtual Camera* ShadowMapCamera() = 0;
	virtual DirectX::XMFLOAT4X4 ProjMatrix() = 0;

protected:
	virtual void BindBuffer() = 0;

	virtual void UpdateParameterBuffer() = 0;
	void flagParameterChange();

	virtual void OnModyfied() override;
	Shadowmap* shadowmap;

	ID3D11Buffer* parameterBuffer;
	
private:
	bool castShadows;

	std::array<float, 3> lightDiffuse;
	std::array<float, 3> lightSpecular;

	bool parametersModifyed;
};

class AmbientLight : public LightBase
{
public:
	AmbientLight(const std::array<float, 3>& ambient);

	void SetAmbient(std::array<float, 3> lightAmbient);
	std::array<float, 3> Ambient();

	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void BindBuffer() override;

	virtual void UpdateParameterBuffer() override;

	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	std::array<float, 3> lightAmbient;
};

class PointLight : public LightBase
{
public:
	PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float falloff);

	void SetFalloff(float falloff);

	void CreateShadowMap(UINT resolution, float nearPlane, float farPlane, Renderer* renderer);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void BindBuffer() override;

	virtual void UpdateParameterBuffer() override;

	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	float falloff;

	float nearPlane;
	float farPlane;
};

class DirectionalLight : public LightBase
{
public:
	DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);

	void CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane, Renderer* renderer);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void BindBuffer() override;

	virtual void UpdateParameterBuffer() override;

	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

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

	void SetFOV(float lightFOV);
	void SetFalloff(float falloff);

	void CreateShadowMap(UINT resolution, float nearPlane, float farPlane, Renderer* renderer);
	virtual Camera* ShadowMapCamera() override;

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual void BindBuffer() override;

	virtual void UpdateParameterBuffer() override;

	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	virtual DirectX::XMFLOAT4X4 ProjMatrix() override;

private:
	float lightFOV;
	float fallof;

	float nearPlane;
	float farPlane;
};

class Shadowmap
{
public:
	Shadowmap(UINT resolution, LightBase* linkedLight, Renderer* renderer);
	virtual ~Shadowmap();

	void Bind();

	void flagShadowChange();

	int Resolution();

protected:
	virtual void BindMap() = 0;
	virtual void MapRender() = 0;
	Renderer* renderer;

	ID3D11Texture2D* mapTexture;
	ID3D11Buffer* shadowMappingBuffer;

	ID3D11ShaderResourceView* mapSRV;

	LightBase* linkedLight;

private:
	UINT resolution;
	bool mappingTransformed;

	UINT latestRender;
};

class ShadowMapSingle : public Shadowmap
{
public:
	ShadowMapSingle(UINT resolution, LightBase* linkedLight, Renderer* renderer);
	virtual ~ShadowMapSingle();

protected:
	virtual void BindMap() override;
	virtual void MapRender() override;

private:
	ID3D11DepthStencilView* mapDSV;
};

class ShadowMapCube : public Shadowmap
{
public:
	ShadowMapCube(UINT resolution, LightBase* linkedLight, Renderer* renderer);
	virtual ~ShadowMapCube();

protected:
	virtual void BindMap() override;
	virtual void MapRender() override;

private:
	ID3D11RenderTargetView* mapRTV[6];
	ID3D11Texture2D* depthStencilTexture;
	ID3D11DepthStencilView* DSV;
};
