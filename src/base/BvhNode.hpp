#pragma once

#include <memory>
#include <vector>

struct AABB
{
	float x1, x2, y1, y2, z1, z2;
};

struct RayNode
{
public:
	int startprim, endprim;
	float t1, t2;
};

struct Node
{
public:
	AABB box; // Axis-aligned bounding box
	int startPrim, endPrim; // Indices in the global list
	Node* leftChild = nullptr;
	Node* rightChild = nullptr;
	Node();
	Node(const Node& n)
	{
		startPrim = n.startPrim;
		endPrim = n.endPrim;
		box = n.box;
		if (n.leftChild != nullptr)
		{
			leftChild = new Node(*n.leftChild);
			rightChild = new Node(*n.rightChild);
		}
	}

	~Node()
	{ 
		if (leftChild != nullptr)
		{
			std::free(leftChild);
			std::free(rightChild);
		}
	}
};