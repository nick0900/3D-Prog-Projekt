#include "Shaders.h"
#include <fstream>
#include <iostream>

#include "Pipeline.h"

VShader::VShader(const std::string shaderPath) : inputLayout(nullptr), vShader(nullptr)
{
	std::string shaderData;
	std::ifstream reader;

	reader.open(shaderPath, std::ios::binary | std::ios::ate);
	if (!reader.is_open())
	{
		std::cerr << "Could not open vertex shader file!" << std::endl;
		return;
	}

	reader.seekg(0, std::ios::end);
	shaderData.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	shaderData.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());

	if (FAILED(Pipeline::Device()->CreateVertexShader(shaderData.c_str(), shaderData.length(), nullptr, &vShader)))
	{
		std::cerr << "Failed to create vertex shader!" << std::endl;
		return;
	}



	D3D11_INPUT_ELEMENT_DESC inputDesc[3] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	if (FAILED(Pipeline::Device()->CreateInputLayout(inputDesc, 3, shaderData.c_str(), shaderData.length(), &inputLayout)))
	{
		std::cerr << "failed to set up input layout!" << std::endl;
	}
}

VShader::~VShader()
{
	inputLayout->Release();
	vShader->Release();
}

void VShader::Bind()
{
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::VertexShader(vShader);
	Pipeline::Deferred::GeometryPass::VertexShader::Bind::InputLayout(inputLayout);
}

PShader::PShader(const std::string shaderPath) : pShader(nullptr)
{
	std::string shaderData;
	std::ifstream reader;

	shaderData.clear();
	reader.close();
	reader.open(shaderPath, std::ios::binary | std::ios::ate);
	if (!reader.is_open())
	{
		std::cerr << "Could not open pixel shader file!" << std::endl;
		return;
	}

	reader.seekg(0, std::ios::end);
	shaderData.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	shaderData.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());

	if (FAILED(Pipeline::Device()->CreatePixelShader(shaderData.c_str(), shaderData.length(), nullptr, &pShader)))
	{
		std::cerr << "Failed to create pixel shader!" << std::endl;
	}
}

PShader::~PShader()
{
	pShader->Release();
}

void PShader::Bind()
{
	Pipeline::Deferred::GeometryPass::PixelShader::Bind::PixelShader(pShader);
}

CShader::CShader(const std::string shaderPath) : cShader(nullptr)
{
	std::string shaderData;
	std::ifstream reader;

	shaderData.clear();
	reader.close();
	reader.open(shaderPath, std::ios::binary | std::ios::ate);
	if (!reader.is_open())
	{
		std::cerr << "Could not open compute shader file!" << std::endl;
		return;
	}

	reader.seekg(0, std::ios::end);
	shaderData.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	shaderData.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());

	if (FAILED(Pipeline::Device()->CreateComputeShader(shaderData.c_str(), shaderData.length(), nullptr, &cShader)))
	{
		std::cerr << "Failed to create compute shader!" << std::endl;
	}
}

CShader::~CShader()
{
	cShader->Release();
}

void CShader::Bind()
{
	Pipeline::Deferred::LightPass::ComputeShader::Bind::ComputeShader(cShader);
}

HShader::HShader(const std::string shaderPath)
{
	std::string shaderData;
	std::ifstream reader;

	shaderData.clear();
	reader.close();
	reader.open(shaderPath, std::ios::binary | std::ios::ate);
	if (!reader.is_open())
	{
		std::cerr << "Could not open hull shader file!" << std::endl;
		return;
	}

	reader.seekg(0, std::ios::end);
	shaderData.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	shaderData.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());

	if (FAILED(Pipeline::Device()->CreateHullShader(shaderData.c_str(), shaderData.length(), nullptr, &hShader)))
	{
		std::cerr << "Failed to create hull shader!" << std::endl;
	}
}

HShader::~HShader()
{
	hShader->Release();
}

void HShader::Bind()
{
	Pipeline::Deferred::GeometryPass::HullShader::Bind::HullShader(hShader);
}

DShader::DShader(const std::string shaderPath)
{
	std::string shaderData;
	std::ifstream reader;

	shaderData.clear();
	reader.close();
	reader.open(shaderPath, std::ios::binary | std::ios::ate);
	if (!reader.is_open())
	{
		std::cerr << "Could not open domain shader file!" << std::endl;
		return;
	}

	reader.seekg(0, std::ios::end);
	shaderData.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	shaderData.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());

	if (FAILED(Pipeline::Device()->CreateDomainShader(shaderData.c_str(), shaderData.length(), nullptr, &dShader)))
	{
		std::cerr << "Failed to create domain shader!" << std::endl;
	}
}

DShader::~DShader()
{
	dShader->Release();
}

void DShader::Bind()
{
	Pipeline::Deferred::GeometryPass::DomainShader::Bind::DomainShader(dShader);
}
