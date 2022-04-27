#include "OBJParsing.h"
#include <fstream>
#include <iostream>

#include "SharedResources.h"
#include "Pipeline.h"

STDOBJ::STDOBJ(const std::string OBJFilepath)
{
	if (!CreateTransformBuffer())
	{
		std::cerr << "Failed to create transform buffer!" << std::endl;
	}

	if (!LoadOBJ(OBJFilepath))
	{
		std::cerr << "failed to load OBJ" << std::endl;
	}
}



STDOBJ::~STDOBJ()
{
	worldTransformBuffer->Release();
	vertexBuffer->Release();
	indexBuffer->Release();
}

void STDOBJ::Render()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexBuffer(stride, offset, vertexBuffer);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::IndexBuffer(indexBuffer);

	UpdateTransformBuffer();

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::ObjectTransform(worldTransformBuffer);
	
	SharedResources::BindVertexShader(SharedResources::vShader::Standard);

	int previousMaterial = -1;
	for (Submesh submesh : submeshes)
	{
		if (submesh.material == -1)
		{
			//extra precaution, should not happen during correct execution.
			submesh.material = 0;
		}

		if (submesh.material != previousMaterial)
		{
			previousMaterial = submesh.material;
			SharedResources::BindMaterial(submesh.material);
		}

		Pipeline::DrawIndexed(submesh.size, submesh.Start);
	}
}

bool GetWord(std::string& word, std::string& line, char splitChar)
{
	if (line == "")
	{
		return false;
	}

	int pos = line.find(splitChar);

	if (pos != std::string::npos)
	{
		word = line.substr(0, pos);

		line.erase(0, pos + 1);

		return true;
	}

	word = line;
	line.clear();
	return true;
}

bool STDOBJ::LoadOBJ(std::string OBJFilepath)
{
	
	std::ifstream OBJ;
	OBJ.open(OBJFilepath);

	if (!OBJ.is_open())
	{
		std::cerr << "Failed to open obj filepath: " << OBJFilepath << std::endl;
		return false;
	}

	std::vector<std::array<float, 3>> pos;
	std::vector<std::array<float, 2>> uv;
	std::vector<std::array<float, 3>> norm;

	std::map<std::string, int> vertMap;
	std::vector<Vertex> vertecies;

	std::vector<UINT> indecies;

	Submesh currentSubmesh;
	bool newSubmesh = false;

	std::string line = "";

	while (std::getline(OBJ, line))
	{
		std::string word = "";
		
		GetWord(word, line, ' ');

		if (word == "mtllib")
		{
			if (!LoadMTL(line))
			{
				std::cerr << "Failed to load: " << line << std::endl;
			}
		}
		else if (word == "v")
		{
			std::array<float, 3> temp;
			
			GetWord(word, line, ' ');
			temp[0] = std::stof(word);
			GetWord(word, line, ' ');
			temp[1] = std::stof(word);
			temp[2] = std::stof(line);

			pos.push_back(temp);
		}
		else if (word == "vt")
		{
			std::array<float, 2> temp;

			GetWord(word, line, ' ');
			temp[0] = std::stof(word);
			temp[1] = std::stof(line);

			uv.push_back(temp);
		}
		else if (word == "vn")
		{
			std::array<float, 3> temp;

			GetWord(word, line, ' ');
			temp[0] = std::stof(word);
			GetWord(word, line, ' ');
			temp[1] = std::stof(word);
			temp[2] = std::stof(line);

			norm.push_back(temp);
		}
		else if (word == "s")
		{
			if (newSubmesh)
			{
				submeshes.push_back(currentSubmesh);
			}
			else
			{
				newSubmesh = true;
			}
			currentSubmesh.Start = indecies.size();
			currentSubmesh.size = 0;
		}
		else if (word == "usemtl")
		{
			if (!newSubmesh)
			{
				std::cerr << ".obj must use submeshes partitioned by s!" << std::endl;
				return false;
			}

			currentSubmesh.material = SharedResources::GetMaterialID(line);
		}
		else if (word == "f")
		{
			if (!newSubmesh)
			{
				std::cerr << ".obj must use submeshes partitioned by s!" << std::endl;
				return false;
			}
			while (GetWord(word, line, ' '))
			{
				std::string key = word;
				if (!(vertMap.count(key) > 0))
				{
					vertMap[key] = vertecies.size();

					std::string letter;
					int index[3];

					GetWord(letter, word, '/');
					index[0] = std::stoi(letter) - 1;
					GetWord(letter, word, '/');
					index[1] = std::stoi(letter) - 1;
					index[2] = std::stoi(word) - 1;

					Vertex temp = { {pos[index[0]][0], pos[index[0]][1], pos[index[0]][2]}, {norm[index[2]][0], norm[index[2]][1], norm[index[2]][2]}, {uv[index[1]][0], -uv[index[1]][1]} };

					vertecies.push_back(temp);
				}

				indecies.push_back(vertMap[key]);
				currentSubmesh.size++;
			}
		}
	}
	if (newSubmesh)
	{
		submeshes.push_back(currentSubmesh);
	}
	
	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.ByteWidth = sizeof(Vertex) * vertecies.size();
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &vertecies[0];
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &vertexBuffer)))
	{
		std::cerr << "Failed to create VertexBuffer!" << std::endl;
		return false;
	}

	bufferDesc.ByteWidth = sizeof(UINT) * indecies.size();
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	data.pSysMem = indecies.data();
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &indexBuffer)))
	{
		std::cerr << "Failed to create indexbuffer!" << std::endl;
		return false;
	}

	return true;
}

bool STDOBJ::LoadMTL(std::string MTLFilepath)
{
	std::ifstream MTL;
	MTL.open("OBJ/" + MTLFilepath);

	if (!MTL.is_open())
	{
		std::cerr << "Failed to open mtl filepath: " << MTLFilepath << std::endl;
		return false;
	}

	std::string line = "";
	MaterialData currentMatData = MaterialData();
	bool newMaterial = false;

	while (std::getline(MTL, line))
	{
		std::string word = "";

		GetWord(word, line, ' ');

		if (word == "newmtl")
		{
			if (newMaterial)
			{
				SharedResources::AddMaterial(currentMatData);

				currentMatData = MaterialData();
			}

			if (!SharedResources::MaterialExists(line))
			{
				currentMatData.name = line;
				newMaterial = true;
			}
			else
			{
				newMaterial = false;
			}
		}
		else if (newMaterial)
		{
			if ((word == "Ns") || (word == "ns"))
			{
				currentMatData.Ns = std::stof(line);
			}
			else if ((word == "Ka") || (word == "ka"))
			{
				GetWord(word, line, ' ');
				currentMatData.Ka[0] = std::stof(word);
				GetWord(word, line, ' ');
				currentMatData.Ka[1] = std::stof(word);
				currentMatData.Ka[2] = std::stof(line);
			}
			else if ((word == "Kd") || (word == "kd"))
			{
				GetWord(word, line, ' ');
				currentMatData.Kd[0] = std::stof(word);
				GetWord(word, line, ' ');
				currentMatData.Kd[1] = std::stof(word);
				currentMatData.Kd[2] = std::stof(line);
			}
			else if ((word == "Ks") || (word == "ks"))
			{
				GetWord(word, line, ' ');
				currentMatData.Ks[0] = std::stof(word);
				GetWord(word, line, ' ');
				currentMatData.Ks[1] = std::stof(word);
				currentMatData.Ks[2] = std::stof(line);
			}
			else if ((word == "map_Ka") || (word == "map_ka"))
			{
				currentMatData.textured = true;
				currentMatData.map_Ka = line;
			}
			else if ((word == "map_Kd") || (word == "map_kd"))
			{
				currentMatData.textured = true;
				currentMatData.map_Kd = line;
			}
			else if ((word == "map_Ks") || (word == "map_ks"))
			{
				currentMatData.textured = true;
				currentMatData.map_Ks = line;
			}
		}
	}

	if (newMaterial)
	{
		SharedResources::AddMaterial(currentMatData);
	}

	return true;
}
