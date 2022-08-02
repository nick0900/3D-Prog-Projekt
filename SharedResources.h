#pragma once

#include <string>
#include <map>
#include <vector>
#include <d3d11.h>
#include <array>

#include "Shaders.h"

struct MaterialData {
	bool textured = false;
	std::string name = "";
	float Ns = 0;
	float Ka[3] = { 1.0f, 0.0f, 1.0f };
	float Kd[3] = { 1.0f, 0.0f, 1.0f };
	float Ks[3] = { 1.0f, 0.0f, 1.0f };
	std::string map_Kd = "";
	std::string map_Ks = "";
	std::string map_Ka = "";
};

struct MatBufTex
{
	float Ns = 0;
	float padding[3] = { 0, 0, 0 };

	MatBufTex(const float specularExponent)
	{
		Ns = specularExponent;
	}
};

struct MatBufParam
{
	float Ka[3] = { 1.0f, 0.0f, 1.0f };
	float padding1 = 0;
	float Kd[3] = { 1.0f, 0.0f, 1.0f };
	float padding2 = 0;
	float Ks[3] = { 1.0f, 0.0f, 1.0f };
	float Ns = 0;

	MatBufParam(const float specularExponent, const std::array<float, 3>& ambientKoef, const std::array<float, 3>& diffuseKoef, const std::array<float, 3>& specularKoef)
	{
		Ns = specularExponent;
		for (int i = 0; i < 3; i++)
		{
			Ka[i] = ambientKoef[i];
			Kd[i] = diffuseKoef[i];
			Ks[i] = specularKoef[i];
		}
	}
};

class BaseMaterial
{
	public :
		~BaseMaterial();
		virtual void Bind() = 0;

	protected :
		ID3D11Buffer* materialBuffer;
};

class TexturedMaterial : public BaseMaterial
{
	public :
		TexturedMaterial(MaterialData& matData);

		virtual void Bind() override;

	private :
		int map_Ka = 0;
		int map_Kd = 0;
		int map_Ks = 0;
};

class ParameterMaterial : public BaseMaterial
{
	public:
		ParameterMaterial(MaterialData& matData);

		virtual void Bind() override;
};

class Materials
{
	public :
		Materials();
		~Materials();

		int AddMaterial(MaterialData& material);
		bool Contains(const std::string materialName);
		int GetID(const std::string materialName);
		void Bind(int materialID);

	private :
		std::map<std::string, int> materialMap;
		std::vector<BaseMaterial*> container;
};

class Textures
{
	public:
		Textures();
		~Textures();

		int AddTexture(const std::string& texturePath);

		ID3D11ShaderResourceView* GetSRV(int textureID);

	private :
		std::map<std::string, int> textureMap;
		std::vector<ID3D11Texture2D*> textures;
		std::map<int, ID3D11ShaderResourceView*> SRVs;
};

namespace SharedResources
{
	void Setup();
	void Release();

	bool MaterialExists(const std::string materialName);
	int AddMaterial(MaterialData& material);
	int GetMaterialID(const std::string materialName);
	void BindMaterial(int materialID);


	enum vShader
	{
		VSStandard = 0,
		Tesselation = 1
	};
	void BindVertexShader(vShader ID);

	enum pShader
	{
		TextureMaterial = 0,
		ParameterMaterial = 1,
		DistanceWrite = 2
	};
	void BindPixelShader(pShader ID);

	enum cShader
	{
		Standard32x32 = 0
	};
	void BindComputeShader(cShader ID);

	enum hShader
	{
		HSStandard = 0
	};
	void BindHullShader(hShader ID);

	enum dShader
	{
		DSStandard = 0
	};
	void BindDomainShader(dShader ID);
}