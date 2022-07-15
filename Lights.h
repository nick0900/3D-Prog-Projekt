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
	PointLightNoShadow = 0,
	PointLightShadow = 1,
	DirectionalLightNoShadow = 2,
	DirectionalLightShadow = 3,
	SpotLightNoShadow = 4,
	SpotLightShadow = 5
};

struct PointLightBuffer
{
	int lightType;
	float falloff;
	float diffuse[3];
	float specular[3];
	float position[3];
	

	float padding;

	PointLightBuffer(float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position, bool castShadows) :
		falloff(falloff), padding(0.0f)
	{
		lightType = castShadows ? PointLightShadow : PointLightNoShadow;
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
	int lightType;
	float diffuse[3];
	float specular[3];
	float direction[3];

	float padding[2];

	DirectionalLightBuffer(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& direction, bool castShadows)
	{
		lightType = castShadows ? DirectionalLightShadow : DirectionalLightNoShadow;
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->direction[i] = direction[i];
		}
		this->padding[0] = 0.0f;
		this->padding[1] = 0.0f;
	}
};

struct SpotLightBuffer
{
	int lightType;
	float falloff;
	float lightFOV;
	float diffuse[3];
	float specular[3];
	float position[3];
	float direction[3];

	float padding;

	SpotLightBuffer(float lightFOV, float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position, const std::array<float, 3>& direction, bool castShadows) :
		lightFOV(lightFOV),
		falloff(falloff)
	{
		lightType = castShadows ? SpotLightShadow : SpotLightNoShadow;
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = position[i];
			this->direction[i] = direction[i];
		}
		this->padding = 0.0f;
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
	virtual void UpdateParameterBuffer() = 0;
	void flagParameterChange();

	virtual SharedResources::cShader LightShader() = 0;
	

	virtual void OnModyfied() override;
	Shadowmap* shadowmap;

	ID3D11Buffer* parameterBuffer;
	
private:
	bool castShadows;

	std::array<float, 3> lightDiffuse;
	std::array<float, 3> lightSpecular;

	bool parametersModifyed;
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
	virtual void UpdateParameterBuffer() override;

	virtual SharedResources::cShader LightShader() override;

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
	virtual void UpdateParameterBuffer() override;

	virtual SharedResources::cShader LightShader() override;

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
	virtual void UpdateParameterBuffer() override;

	virtual SharedResources::cShader LightShader() override;

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
	~Shadowmap();

	void Bind();

	void flagShadowChange();

	int Resolution();

protected:
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
	~ShadowMapSingle();

protected:
	virtual void MapRender() override;

private:
	ID3D11DepthStencilView* mapDSV;
};

class ShadowMapCube : public Shadowmap
{
public:
	ShadowMapCube(UINT resolution, LightBase* linkedLight, Renderer* renderer);
	~ShadowMapCube();

protected:
	virtual void MapRender() override;

private:
	ID3D11DepthStencilView* mapDSV[6];
};
