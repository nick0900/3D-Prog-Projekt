#include "OBJParsing.h"
#include <fstream>
#include <iostream>
#include <DirectXCollision.h>

#include "SharedResources.h"
#include "Pipeline.h"

STDOBJ::STDOBJ(const std::string OBJFilepath)
{
	boundingVolume = DirectX::BoundingSphere();
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
	
	SharedResources::BindVertexShader(SharedResources::vShader::VSStandard);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

void STDOBJ::DepthRender()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexBuffer(stride, offset, vertexBuffer);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::IndexBuffer(indexBuffer);

	UpdateTransformBuffer();

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::ObjectTransform(worldTransformBuffer);

	SharedResources::BindVertexShader(SharedResources::vShader::VSStandard);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (Submesh submesh : submeshes)
	{
		Pipeline::DrawIndexed(submesh.size, submesh.Start);
	}
}

bool STDOBJ::Contained(DirectX::BoundingFrustum& viewFrustum)
{
	float biggestScale = 0.0f;
	if (scale.x > biggestScale) biggestScale = scale.x;
	if (scale.y > biggestScale) biggestScale = scale.y;
	if (scale.z > biggestScale) biggestScale = scale.z;

	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(biggestScale, biggestScale, biggestScale);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMMATRIX transform = scaling * rotation * translation;

	DirectX::BoundingSphere transformedVolume;

	boundingVolume.Transform(transformedVolume, transform);
	
	return transformedVolume.Intersects(viewFrustum);
}

void STDOBJ::AddToQuadTree(QuadTree* tree)
{
	float biggestScale = 0.0f;
	if (scale.x > biggestScale) biggestScale = scale.x;
	if (scale.y > biggestScale) biggestScale = scale.y;
	if (scale.z > biggestScale) biggestScale = scale.z;

	DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(biggestScale, biggestScale, biggestScale);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuaternion));

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	DirectX::XMMATRIX transform = scaling * rotation * translation;

	DirectX::BoundingSphere transformedVolume;

	boundingVolume.Transform(transformedVolume, transform);
	
	tree->InsertObject(this, &transformedVolume);
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

	float boundingRadius = 0.0f;

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

					float distance = sqrtf(powf(temp.pos[0], 2.0f) + powf(temp.pos[1], 2.0f) + powf(temp.pos[2], 2.0f));
					if (distance > boundingRadius)
					{
						boundingRadius = distance;
					}
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

	boundingVolume = DirectX::BoundingSphere({ 0.0f, 0.0f, 0.0f }, boundingRadius);

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

STDOBJTesselated::STDOBJTesselated(const std::string OBJFilepath, float maxTesselation, float maxDistance, float minDistance, float interpolationFactor) : STDOBJ(OBJFilepath)
{
	D3D11_BUFFER_DESC bufferDesc;

	TesselationConfigBufferStruct bufferStruct = TesselationConfigBufferStruct(maxTesselation, maxDistance, minDistance, interpolationFactor);

	bufferDesc.ByteWidth = sizeof(TesselationConfigBufferStruct);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &bufferStruct;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (FAILED(Pipeline::Device()->CreateBuffer(&bufferDesc, &data, &tesselationConfigBuffer)))
	{
		std::cerr << "Failed to create tesselation config buffer" << std::endl;
	}
}

STDOBJTesselated::~STDOBJTesselated()
{
	tesselationConfigBuffer->Release();
}

void STDOBJTesselated::Render()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexBuffer(stride, offset, vertexBuffer);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::IndexBuffer(indexBuffer);

	UpdateTransformBuffer();

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::ObjectTransform(worldTransformBuffer);

	SharedResources::BindVertexShader(SharedResources::vShader::Tesselation);

	Pipeline::Deferred::GeometryPass::HullShader::Bind::HSConfigBuffer(tesselationConfigBuffer);
	
	SharedResources::BindHullShader(SharedResources::hShader::HSStandard);

	Pipeline::Deferred::GeometryPass::DomainShader::Bind::DSConfigBuffer(tesselationConfigBuffer);

	SharedResources::BindDomainShader(SharedResources::dShader::DSStandard);

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

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

	Pipeline::Deferred::GeometryPass::HullShader::UnBind::HullShader();
	Pipeline::Deferred::GeometryPass::DomainShader::UnBind::DomainShader();
}

void STDOBJTesselated::DepthRender()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexBuffer(stride, offset, vertexBuffer);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::IndexBuffer(indexBuffer);

	UpdateTransformBuffer();

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::ObjectTransform(worldTransformBuffer);

	SharedResources::BindVertexShader(SharedResources::vShader::Tesselation);

	Pipeline::Deferred::GeometryPass::HullShader::Bind::HSConfigBuffer(tesselationConfigBuffer);

	SharedResources::BindHullShader(SharedResources::hShader::HSStandard);

	Pipeline::Deferred::GeometryPass::DomainShader::Bind::DSConfigBuffer(tesselationConfigBuffer);

	SharedResources::BindDomainShader(SharedResources::dShader::DSStandard);

	Pipeline::Deferred::GeometryPass::VertexShader::Bind::PrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	for (Submesh submesh : submeshes)
	{
		Pipeline::DrawIndexed(submesh.size, submesh.Start);
	}

	Pipeline::Deferred::GeometryPass::HullShader::UnBind::HullShader();
	Pipeline::Deferred::GeometryPass::DomainShader::UnBind::DomainShader();
}

STDOBJMirror::STDOBJMirror(const std::string OBJFilepath, UINT resolution, Renderer* renderer, float nearPlane, float farPlane) : STDOBJ(OBJFilepath), renderer(renderer), resolution(resolution), nearPlane(nearPlane), farPlane(farPlane)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 6;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &textureCube)))
	{
		std::cerr << "Failed to set up texture cube resource" << std::endl;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	D3D11_TEXCUBE_SRV texSRV;
	texSRV.MipLevels = 1;
	texSRV.MostDetailedMip = 0;
	srvDesc.TextureCube = texSRV;

	if (FAILED(Pipeline::Device()->CreateShaderResourceView(textureCube, &srvDesc, &SRV)))
	{
		std::cerr << "Failed to set up SRV" << std::endl;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	D3D11_TEX2D_ARRAY_UAV texUAV;
	texUAV.ArraySize = 1;
	texUAV.MipSlice = 0;
	uavDesc.Texture2DArray = texUAV;

	for (int i = 0; i < 6; i++)
	{
		uavDesc.Texture2DArray.FirstArraySlice = i;
		if (FAILED(Pipeline::Device()->CreateUnorderedAccessView(textureCube, &uavDesc, &UAVs[i])))
		{
			std::cerr << "Error: Failed to set up UAVs" << std::endl;
		}
	}

	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;

	if (FAILED(Pipeline::Device()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture)))
	{
		std::cerr << "Failed to set up depth stencil texture" << std::endl;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	D3D11_TEX2D_DSV texDSV;
	texDSV.MipSlice = 0;
	dsvDesc.Texture2D = texDSV;

	if (FAILED(Pipeline::Device()->CreateDepthStencilView(depthStencilTexture, &dsvDesc, &DSV)))
	{
		std::cerr << "Error: Failed to set up DSV" << std::endl;
	}
}

STDOBJMirror::~STDOBJMirror()
{
	for (int i = 0; i < 6; i++)
	{
		UAVs[i]->Release();
	}
	textureCube->Release();
	SRV->Release();
	depthStencilTexture->Release();
	DSV->Release();
}

void STDOBJMirror::ReflectionRender()
{
	CameraPerspective view = CameraPerspective(resolution, resolution, 0.0f, 0.0f, 90.0f, nearPlane, farPlane);

	view->Rotate({ 0.0f, 90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[0], DSV, view);

	view->Rotate({ 0.0f, -90.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[1], DSV, view);

	view->Rotate({ -90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[2], DSV, view);

	view->Rotate({ 90.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[3], DSV, view);

	view->Rotate({ 0.0f, 0.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[4], DSV, view);

	view->Rotate({ 0.0f, 180.0f, 0.0f }, OBJECT_TRANSFORM_SPACE_GLOBAL, OBJECT_TRANSFORM_REPLACE, OBJECT_ROTATION_UNIT_DEGREES);
	view->UpdateTransformBuffer();

	renderer->CameraDistanceRender(mapRTV[5], DSV, view);

	delete view;
}
