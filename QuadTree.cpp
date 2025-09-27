#include "QuadTree.h"

#include <vector>

#include "BaseObject.h"

QuadTree::QuadTree(float worldWidth, UINT partitionDepth)
{
	if (partitionDepth == 0)
	{
		root = new Leaf();
	}
	else
	{
		root = new BranchPoint(partitionDepth, { 0.0f, 0.0f, 0.0f }, worldWidth);
	}
}

QuadTree::~QuadTree()
{
	delete root;
}

void QuadTree::InsertObject(Object* object, DirectX::BoundingSphere* boundingVolume)
{
	root->Emplace(object, boundingVolume);
}

void QuadTree::GetContainedInFrustum(DirectX::BoundingFrustum* viewFrustum, std::vector<Object*>& containedObjects)
{
	std::map<Object*, Object*> containedObjectsMap;

	root->Retrieve(viewFrustum, &containedObjectsMap);

	for (const std::pair<Object*, Object*> object : containedObjectsMap)
	{
		containedObjects.push_back(object.first);
	}
}

QuadTree::BranchPoint::BranchPoint(UINT layer, DirectX::XMFLOAT3 nodeCentre, float nodeWidth) : nodeCentre(nodeCentre)
{
	if (layer <= 1)
	{
		for (int i = 0; i < 4; i++)
		{
			branches[i] = new Leaf();
		}
	}
	else
	{
		layer--;
		nodeWidth /= 2;
		float dx = nodeWidth / 2;

		branches[0] = new BranchPoint(layer, { nodeCentre.x + dx, nodeCentre.y, nodeCentre.z + dx}, nodeWidth);
		branches[1] = new BranchPoint(layer, { nodeCentre.x - dx, nodeCentre.y, nodeCentre.z + dx}, nodeWidth);
		branches[2] = new BranchPoint(layer, { nodeCentre.x + dx, nodeCentre.y, nodeCentre.z - dx}, nodeWidth);
		branches[3] = new BranchPoint(layer, { nodeCentre.x - dx, nodeCentre.y, nodeCentre.z - dx}, nodeWidth);
	}
}

QuadTree::BranchPoint::~BranchPoint()
{
	for (int i = 0; i < 4; i++)
	{
		delete branches[i];
	}
}

void QuadTree::BranchPoint::Emplace(Object* mesh, DirectX::BoundingSphere* boundingVolume)
{
	DirectX::XMVECTOR point = DirectX::XMLoadFloat3(&nodeCentre);

	DirectX::XMFLOAT3 norm = { 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR xPlane = DirectX::XMPlaneFromPointNormal(point, DirectX::XMLoadFloat3(&norm));

	norm = { 0.0f, 0.0f, 1.0f };
	DirectX::XMVECTOR zPlane = DirectX::XMPlaneFromPointNormal(point, DirectX::XMLoadFloat3(&norm));

	DirectX::PlaneIntersectionType xResult = boundingVolume->Intersects(xPlane);
	DirectX::PlaneIntersectionType zResult = boundingVolume->Intersects(zPlane);

	if ((zResult == DirectX::PlaneIntersectionType::INTERSECTING) || (zResult == DirectX::PlaneIntersectionType::FRONT))
	{
		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::FRONT))
		{
			branches[0]->Emplace(mesh, boundingVolume);
		}

		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::BACK))
		{
			branches[1]->Emplace(mesh, boundingVolume);
		}
	}

	if ((zResult == DirectX::PlaneIntersectionType::INTERSECTING) || (zResult == DirectX::PlaneIntersectionType::BACK))
	{
		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::FRONT))
		{
			branches[2]->Emplace(mesh, boundingVolume);
		}

		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::BACK))
		{
			branches[3]->Emplace(mesh, boundingVolume);
		}
	}
}

void QuadTree::BranchPoint::Retrieve(DirectX::BoundingFrustum* viewVolume, std::map<Object*, Object*>* containedObjects)
{
	DirectX::XMVECTOR point = DirectX::XMLoadFloat3(&nodeCentre);

	DirectX::XMFLOAT3 norm = { 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR xPlane = DirectX::XMPlaneFromPointNormal(point, DirectX::XMLoadFloat3(&norm));

	norm = { 0.0f, 0.0f, 1.0f };
	DirectX::XMVECTOR zPlane = DirectX::XMPlaneFromPointNormal(point, DirectX::XMLoadFloat3(&norm));

	DirectX::PlaneIntersectionType xResult = viewVolume->Intersects(xPlane);
	DirectX::PlaneIntersectionType zResult = viewVolume->Intersects(zPlane);

	if ((zResult == DirectX::PlaneIntersectionType::INTERSECTING) || (zResult == DirectX::PlaneIntersectionType::FRONT))
	{
		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::FRONT))
		{
			branches[0]->Retrieve(viewVolume, containedObjects);
		}

		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::BACK))
		{
			branches[1]->Retrieve(viewVolume, containedObjects);
		}
	}

	if ((zResult == DirectX::PlaneIntersectionType::INTERSECTING) || (zResult == DirectX::PlaneIntersectionType::BACK))
	{
		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::FRONT))
		{
			branches[2]->Retrieve(viewVolume, containedObjects);
		}

		if ((xResult == DirectX::PlaneIntersectionType::INTERSECTING) || (xResult == DirectX::PlaneIntersectionType::BACK))
		{
			branches[3]->Retrieve(viewVolume, containedObjects);
		}
	}
}

QuadTree::Leaf::Leaf()
{
}

void QuadTree::Leaf::Emplace(Object* mesh, DirectX::BoundingSphere* boundingVolume)
{
	meshes.push_back(mesh);
}

void QuadTree::Leaf::Retrieve(DirectX::BoundingFrustum* viewVolume, std::map<Object*, Object*>* containedObjects)
{
	for (Object* mesh : meshes)
	{
		if (containedObjects->count(mesh) < 1)
		{
			if (mesh->Contained(*viewVolume))
			{
				containedObjects->insert(std::pair<Object*, Object*>(mesh, mesh));
			}
		}
	}
}
