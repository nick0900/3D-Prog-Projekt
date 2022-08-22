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
	LightTypeSpot = 3,
	SpotDirArray = 4
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

struct SpotDirStruct
{
	int lightType;
	float diffuse[3];

	int castShadow;
	float specular[3];

	float position[3];
	float falloff;

	float direction[3];
	float lightFOV;

	SpotDirStruct(float lightFOV, float falloff, const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& position, const std::array<float, 3>& direction, int CastShadow) :
		lightType(0),
		castShadow(CastShadow),
		lightFOV(cosf(lightFOV* OBJECT_ROTATION_UNIT_DEGREES / 2)),
		falloff(falloff)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = position[i];
			this->direction[i] = direction[i];
		}
	}

	SpotDirStruct(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, const std::array<float, 3>& direction, int castShadow) :
		lightType(1),
		castShadow(castShadow),
		lightFOV(0.0f),
		falloff(0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			this->diffuse[i] = diffuse[i];
			this->specular[i] = specular[i];
			this->position[i] = 0.0f;
			this->direction[i] = direction[i];
		}
	}
};

class DepthRenderer;
class OmniDistanceRenderer;
class Shadowmap;

class LightBase : public Object
{
public:
	virtual void Bind() = 0;
};

class AmbientLight : public LightBase
{
public:
	AmbientLight(const std::array<float, 3>& ambient);
	~AmbientLight();

	void SetAmbient(std::array<float, 3> lightAmbient);
	std::array<float, 3> Ambient();

	virtual void Render() override;
	virtual void DepthRender() override;

	virtual void Bind() override;

protected:
	void BindBuffer();

	void UpdateParameterBuffer();

	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

private:
	ID3D11Buffer* parameterBuffer;
	bool parametersModifyed;
	std::array<float, 3> lightAmbient;
};

class SingularLight : public LightBase
{
public:
	SingularLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);
	~SingularLight();

	void SetDiffuse(std::array<float, 3> lightDiffuse);
	std::array<float, 3> Diffuse();
	void SetSpecular(std::array<float, 3> lightSpecular);
	std::array<float, 3> Specular();

	virtual void Bind() override;

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

class PointLight : public SingularLight
{
public:
	PointLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float falloff);

	void SetFalloff(float falloff);

	void CreateShadowMap(UINT resolution, float nearPlane, float farPlane, OmniDistanceRenderer* renderer);
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

class DirectionalLight : public SingularLight
{
public:
	DirectionalLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);

	void CreateShadowMap(UINT resolution, float viewWidth, float nearPlane, float farPlane, DepthRenderer* renderer);
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

class SpotLight : public SingularLight
{
public:
	SpotLight(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof);

	void SetFOV(float lightFOV);
	void SetFalloff(float falloff);

	void CreateShadowMap(UINT resolution, float nearPlane, float farPlane, DepthRenderer* renderer);
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
	Shadowmap(UINT resolution, SingularLight* linkedLight);
	virtual ~Shadowmap();

	void Bind();

	void flagShadowChange();

	int Resolution();

protected:
	virtual void BindMap() = 0;
	virtual void MapRender() = 0;

	ID3D11Texture2D* mapTexture;
	ID3D11Buffer* shadowMappingBuffer;

	ID3D11ShaderResourceView* mapSRV;

	SingularLight* linkedLight;

private:
	UINT resolution;
	bool mappingTransformed;

	UINT latestRender;
};

class ShadowMapSingle : public Shadowmap
{
public:
	ShadowMapSingle(UINT resolution, SingularLight* linkedLight, DepthRenderer* renderer);
	virtual ~ShadowMapSingle();

protected:
	virtual void BindMap() override;
	virtual void MapRender() override;

private:
	DepthRenderer* renderer;
	ID3D11DepthStencilView* mapDSV;
};

class ShadowMapCube : public Shadowmap
{
public:
	ShadowMapCube(UINT resolution, SingularLight* linkedLight, OmniDistanceRenderer* renderer);
	virtual ~ShadowMapCube();

protected:
	virtual void BindMap() override;
	virtual void MapRender() override;

private:
	OmniDistanceRenderer* renderer;
	ID3D11RenderTargetView* mapRTV[6];
};

class SpotDirLightArray;

class LightBaseStaging : public Object
{
public:
	LightBaseStaging();
	virtual ~LightBaseStaging();

	void RemoveShadowMapping();
	void SetCastShadow(bool castShadow);
	bool CastShadow();
	UINT ShadowmapResolution();

	virtual Camera* ShadowMapCamera() = 0;

	void StageLightParameters(UINT index, ID3D11Buffer* parametersStructuredBuffer);
	void StageLightShadowmap(UINT index, ID3D11Buffer* shadowProjectionsStructuredBuffer, ID3D11Texture2D* shadowmapArray);

	ID3D11DepthStencilView* StagingShadowmapDSV();

	void MarkShadowUpdate();
	bool ShadowIsUpdated();

	virtual void Render() override;
	virtual void DepthRender() override;

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

	void FlagModification();
	virtual void OnModyfied() override;

	virtual void StagingBuffersUpdate() = 0;

	UINT mapResolution;
	ID3D11Buffer* parameterStagingBuffer;
	ID3D11Buffer* shadowProjectionStagingBuffer = nullptr;
	ID3D11Texture2D* shadowmapTexture = nullptr;
	ID3D11DepthStencilView* shadowmapDSV = nullptr;

private:
	bool modyfied = false;
	bool castShadow;
	UINT latestUpdate;
};

class SpotDirLightArray : public LightBase
{
public:
	SpotDirLightArray(UINT lightCapacity, UINT shadowmapResolution, DepthRenderer& renderer, std::vector<LightBaseStaging*>* StagingLights);
	~SpotDirLightArray();

	virtual void Render() override;
	virtual void DepthRender() override;

	virtual void Bind() override;

	UINT ShadowmapsResolution();

protected:
	virtual DirectX::XMFLOAT4X4 TransformMatrix() override;
	virtual DirectX::XMFLOAT4X4 InverseTransformMatrix() override;

private:
	UINT lightCapacity;
	UINT mapResolution;
	DepthRenderer* shadowmapRenderer;

	std::vector<LightBaseStaging*>* StagingLights;

	ID3D11Buffer* parametersStructuredBuffer;
	ID3D11ShaderResourceView* parametersSRV;

	ID3D11Buffer* shadowProjectionsStructuredBuffer;
	ID3D11ShaderResourceView* shadowProjectionsSRV;

	ID3D11Texture2D* shadowmapArray;
	ID3D11ShaderResourceView* shadowmapsSRV;
};

class SpotLightStaging : public LightBaseStaging
{
public:
	SpotLightStaging(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular, float lightFOV, float fallof);
	void SetupShadowmapping(SpotDirLightArray& lightArray, float nearPlane, float farPlane);
	virtual Camera* ShadowMapCamera() override;

	void SetDiffuse(std::array<float, 3> lightDiffuse);
	std::array<float, 3> Diffuse();
	void SetSpecular(std::array<float, 3> lightSpecular);
	std::array<float, 3> Specular();

	void SetFOV(float lightFOV);
	float FOV();
	void SetFalloff(float falloff);
	float Falloff();

protected:
	virtual void StagingBuffersUpdate() override;

private:
	std::array<float, 3> lightDiffuse;
	std::array<float, 3> lightSpecular;

	float lightFOV;
	float fallof;

	float nearPlane;
	float farPlane;
};

class DirectionalLightStaging : public LightBaseStaging
{
public:
	DirectionalLightStaging(const std::array<float, 3>& diffuse, const std::array<float, 3>& specular);
	void SetupShadowmapping(SpotDirLightArray& lightArray, float viewWidth, float nearPlane, float farPlane);
	virtual Camera* ShadowMapCamera() override;

	void SetDiffuse(std::array<float, 3> lightDiffuse);
	std::array<float, 3> Diffuse();
	void SetSpecular(std::array<float, 3> lightSpecular);
	std::array<float, 3> Specular();

protected:
	virtual void StagingBuffersUpdate() override;

private:
	std::array<float, 3> lightDiffuse;
	std::array<float, 3> lightSpecular;

	float viewWidth;
	float nearPlane;
	float farPlane;
};
