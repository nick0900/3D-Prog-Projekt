#pragma once
#include <string>
#include <vector>
#include <array>
#include <d3d11.h>

#include "BaseObject.h"
#include "SharedResources.h"

struct Vertex {
	float pos[3];
	float norm[3];
	float uv[2];

	Vertex(const std::array<float, 3>& position, const std::array<float, 3>& normal, const std::array<float, 2>& uvCoords)
	{
		for (int i = 0; i < 3; i++)
		{
			pos[i] = position[i];
			norm[i] = normal[i];
		}
		for (int i = 0; i < 2; i++)
		{
			uv[i] = uvCoords[i];
		}
	}
};

struct Submesh {
	int Start = 0;
	int size = 0;
	int material = 0;
};

class STDOBJ : public Object
{
	public:
		STDOBJ(const std::string OBJFilepath);

		~STDOBJ();

		virtual void Render() override;
		

	private:
		ID3D11Buffer* vertexBuffer;
		ID3D11Buffer* indexBuffer;
		
		std::vector<Submesh> submeshes;

		bool LoadOBJ(std::string OBJFilepath);

		bool LoadMTL(std::string MTLFilepath);
};