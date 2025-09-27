#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <array>
#include <list>
#include <map>
#include <vector>

class Object;

class QuadTree
{
private:
	class TreeNode
	{
	public:
		virtual void Emplace(Object* mesh, DirectX::BoundingSphere* boundingVolume) = 0;
		virtual void Retrieve(DirectX::BoundingFrustum* viewVolume, std::map<Object*, Object*>* containedObjects) = 0;
	};

	class BranchPoint : public TreeNode
	{
	public:
		BranchPoint(UINT layer, DirectX::XMFLOAT3 nodeCentre, float nodeWidth);
		~BranchPoint();

		virtual void Emplace(Object* mesh, DirectX::BoundingSphere* boundingVolume) override;
		virtual void Retrieve(DirectX::BoundingFrustum* viewVolume, std::map<Object*, Object*>* containedObjects) override;

	private:
		DirectX::XMFLOAT3 nodeCentre;

		TreeNode* branches[4];
	};

	class Leaf : public TreeNode
	{
	public:
		Leaf();

		virtual void Emplace(Object* mesh, DirectX::BoundingSphere* boundingVolume) override;
		virtual void Retrieve(DirectX::BoundingFrustum* viewVolume, std::map<Object*, Object*>* containedObjects) override;

	private:
		std::list<Object*> meshes;
	};

public:
	QuadTree(float worldWidth, UINT partitionDepth);
	~QuadTree();

	void InsertObject(Object* object, DirectX::BoundingSphere* boundingVolume);
	void GetContainedInFrustum(DirectX::BoundingFrustum* viewFrustum, std::vector<Object*>& containedObjects);

private:
	TreeNode* root;
};