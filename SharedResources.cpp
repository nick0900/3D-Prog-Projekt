#include "SharedResources.h"

#include <d3d11.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Pipeline.h"

namespace Static
{
	namespace Shaders
	{
		static std::vector<VShader*> Vertex;
		static std::vector<PShader*> Pixel;
		static std::vector<CShader*> Compute;
	}

	static Materials* materials = nullptr;
	static Textures* textures = nullptr;
}

Materials::Materials()
{
	MaterialData defMaterial;
	if (AddMaterial(defMaterial) != 0)
	{
		std::cerr << "Failed to buffer default material" << std::endl;
	}
}

Materials::~Materials()
{
	for (BaseMaterial* material : container)
	{
		delete material;
	}
}

int Materials::AddMaterial(MaterialData& material)
{
	if (materialMap.count(material.name) > 0)
	{
		return materialMap[material.name];
	}

	if (material.textured)
	{
		TexturedMaterial* texMat = new TexturedMaterial(material);
		container.push_back(texMat);
	}
	else
	{
		ParameterMaterial* paramMat = new ParameterMaterial(material);
		container.push_back(paramMat);
	}

	materialMap[material.name] = container.size() - 1;
	return container.size() - 1;
}

bool Materials::Contains(const std::string materialName)
{
	return materialMap.count(materialName) > 0;
}

int Materials::GetID(const std::string materialName)
{
	if (!Contains(materialName))
	{
		return 0;
	}
	return materialMap[materialName];
}

void Materials::Bind(int materialID)
{
	container[materialID]->Bind();
}

Textures::Textures()
{
	if (AddTexture("") != 0)
	{
		std::cerr << "Failed to buffer default texture" << std::endl;
	}
}

Textures::~Textures()
{
	for (ID3D11Texture2D* texture : textures)
	{
		texture->Release();
	}

	for (std::pair<int, ID3D11ShaderResourceView*> element : SRVs)
	{
		element.second->Release();
	}
}

int Textures::AddTexture(const std::string& texturePath)
{
	if (textureMap.count(texturePath) > 0)
	{
		return textureMap[texturePath];
	}

	int x, y, n;
	unsigned char* image;

	if (texturePath == "")
	{
		image = stbi_load("textures/missingTexture.png", &x, &y, &n, 0);
	}
	else
	{
		image = stbi_load(texturePath.c_str(), &x, &y, &n, 0);
	}

	if (image == NULL)
	{
		return 0;
	}

	ID3D11Texture2D* texture;

	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = x;
	textureDesc.Height = y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	switch (n)
	{
	case 1:
		textureDesc.Format = DXGI_FORMAT_A8_UNORM;
		break;

	case 2:
		textureDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		break;

	case 3:
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;

	case 4:
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;

	default:
		std::cerr << "unsupported texture format" << std::endl;
		stbi_image_free(image);
		return 0;
	}

	D3D11_SUBRESOURCE_DATA data;

	data.pSysMem = image;
	data.SysMemPitch = n * x;
	data.SysMemSlicePitch = 0;

	HRESULT hr = Pipeline::Device()->CreateTexture2D(&textureDesc, &data, &texture);
	stbi_image_free(image);

	if (!FAILED(hr))
	{
		textures.push_back(texture);
		textureMap[texturePath] = textures.size() - 1;
		return textures.size() - 1;
	}
	return 0;
}

ID3D11ShaderResourceView* Textures::GetSRV(int textureID)
{
	if (SRVs.count(textureID) <= 0)
	{
		ID3D11ShaderResourceView* newSRV;
		if (FAILED(Pipeline::Device()->CreateShaderResourceView(textures[textureID], nullptr, &newSRV)))
		{
			std::cerr << "Failed to create shader resource view" << std::endl;
			return nullptr;
		}
		SRVs[textureID] = newSRV;
	}

	return SRVs[textureID];
}

BaseMaterial::~BaseMaterial()
{
	materialBuffer->Release();
}

TexturedMaterial::TexturedMaterial(MaterialData& matData)
{
	map_Ka = Static::textures->AddTexture(matData.map_Ka);
	map_Ks = Static::textures->AddTexture(matData.map_Ks);
	map_Kd = Static::textures->AddTexture(matData.map_Kd);

	MatBufTex parameters = MatBufTex(matData.Ns);

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(parameters);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &parameters;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &materialBuffer)))
	{
		std::cerr << "Failed to buffer Textured Material" << std::endl;
	}
}

void TexturedMaterial::Bind()
{
	Static::Shaders::Pixel[SharedResources::pShader::TextureMaterial]->Bind();

	Pipeline::Deferred::GeometryPass::PixelShader::Bind::AmbientMap(Static::textures->GetSRV(map_Ka));
	Pipeline::Deferred::GeometryPass::PixelShader::Bind::DiffuseMap(Static::textures->GetSRV(map_Kd));
	Pipeline::Deferred::GeometryPass::PixelShader::Bind::SpecularMap(Static::textures->GetSRV(map_Ks));

	Pipeline::Deferred::GeometryPass::PixelShader::Bind::MaterialParameters(materialBuffer);
}

ParameterMaterial::ParameterMaterial(MaterialData& matData)
{
	MatBufParam parameters = MatBufParam(matData.Ns, { matData.Ka[0], matData.Ka[1], matData.Ka[2] }, { matData.Kd[0], matData.Kd[1], matData.Kd[2] }, { matData.Ks[0], matData.Ks[1], matData.Ks[2] });

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(parameters);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &parameters;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &materialBuffer)))
	{
		std::cerr << "Failed to buffer Textured Material" << std::endl;
	}
}

void ParameterMaterial::Bind()
{
	Static::Shaders::Pixel[SharedResources::pShader::ParameterMaterial]->Bind();
	
	Pipeline::Deferred::GeometryPass::PixelShader::Clear::SRVs();

	Pipeline::Deferred::GeometryPass::PixelShader::Bind::MaterialParameters(materialBuffer);
}

void SharedResources::Setup()
{
	Static::Shaders::Vertex.push_back(new VShader("VSMeshGeometryPass.cso"));

	Static::Shaders::Pixel.push_back(new PShader("PSTexturedGeometryPass.cso"));

	Static::Shaders::Pixel.push_back(new PShader("PSParameterGeometryPass.cso"));

	Static::Shaders::Compute.push_back(new CShader("CSPointLight.cso"));
	Static::Shaders::Compute.push_back(new CShader("CSPointLightShadow.cso"));

	Static::Shaders::Compute.push_back(new CShader("CSDirectionalLight.cso"));
	Static::Shaders::Compute.push_back(new CShader("CSDirectionalLightShadow.cso"));

	Static::Shaders::Compute.push_back(new CShader("CSSpotLight.cso"));
	Static::Shaders::Compute.push_back(new CShader("CSSpotLightShadow.cso"));

	Static::materials = new Materials();
	Static::textures = new Textures();
}

void SharedResources::Release()
{
	for (VShader* shader : Static::Shaders::Vertex)
	{
		delete shader;
	}
	for (PShader* shader : Static::Shaders::Pixel)
	{
		delete shader;
	}
	for (CShader* shader : Static::Shaders::Compute)
	{
		delete shader;
	}

	delete Static::materials;
	delete Static::textures;
}

bool SharedResources::MaterialExists(const std::string materialName)
{
	return Static::materials->Contains(materialName);
}

int SharedResources::AddMaterial(MaterialData& material)
{
	return Static::materials->AddMaterial(material);
}

int SharedResources::GetMaterialID(const std::string materialName)
{
	return Static::materials->GetID(materialName);
}

void SharedResources::BindMaterial(int materialID)
{
	Static::materials->Bind(materialID);
}

void SharedResources::BindVertexShader(vShader ID)
{
	Static::Shaders::Vertex[ID]->Bind();
}

void SharedResources::BindPixelShader(pShader ID)
{
	Static::Shaders::Pixel[ID]->Bind();
}

void SharedResources::BindComputeShader(cShader ID)
{
	Static::Shaders::Compute[ID]->Bind();
}
