#pragma once
#include <string>
#include <d3d11.h>

class VShader
{
	public :
		VShader(const std::string shaderPath);
		~VShader();

		void Bind();

	private:
		ID3D11VertexShader* vShader;
		ID3D11InputLayout* inputLayout;
};

class PShader
{
	public :
		PShader(const std::string shaderPath);
		~PShader();

		void Bind();

	private :
		ID3D11PixelShader* pShader;
};

class CShader
{
public:
	CShader(const std::string shaderPath);
	~CShader();

	void Bind();

private:
	ID3D11ComputeShader* cShader;
};

class HShader
{
public:
	HShader(const std::string shaderPath);
	~HShader();

	void Bind();

private:
	ID3D11HullShader* hShader;
};

class DShader
{
public:
	DShader(const std::string shaderPath);
	~DShader();

	void Bind();

private:
	ID3D11DomainShader* dShader;
};
