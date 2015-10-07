#define _CRT_SECURE_NO_WARNINGS

#include "RayTracer.hpp"
#include "base/Defs.hpp"
#include "base/Math.hpp"
#include <stdio.h>
#include "rtIntersect.inl"
#include <fstream>
#include <algorithm>
#include <iostream>

#define MAX_MEMS 8

// Helper function for hashing scene data for caching BVHs
extern "C" void MD5Buffer( void* buffer, size_t bufLen, unsigned int* pDigest );


namespace FW
{

Mat3f formBasis(const Vec3f& n) {
	// YOUR CODE HERE (R3)
	Mat3f R;
	Vec3f Q = n;
	float min = 99;
	int ind = -1;
	for(int i = 0; i<3; ++i)
	if(abs(n[i]) < min)
	{
		min = abs(n[i]);
		ind = i;
	}
	Q[ind] = 1;
	Q.normalize();
	Vec3f T = cross(Q, n).normalized();
	Vec3f B = cross(n, T).normalized();
	R.setCol(0, T);
	R.setCol(1, B);
	R.setCol(2, n);
	float f = det(R);
	return R;
}

//String RayTracer::computeMD5( const std::vector<Vec3f>& vertices )
//{
//	unsigned char digest[16];
//	MD5Buffer( (void*)&vertices[0], sizeof(Vec3f)*vertices.size(), (unsigned int*)digest );
//
//	// turn into string
//	char ad[33];
//	for ( int i = 0; i < 16; ++i )
//		::sprintf( ad+i*2, "%02x", digest[i] );
//	ad[32] = 0;
//
//	return FW::String( ad );
//}
// --------------------------------------------------------------------------


RayTracer::RayTracer()
{
}


void RayTracer::loadHierarchy(const char* filename, std::vector<Triangle>& triangles)
{
	// YOUR CODE HERE (R2)
	m_triangles = &triangles;
}

void RayTracer::saveHierarchy(const char* filename, const std::vector<Triangle>& triangles) {
	// YOUR CODE HERE (R2)
}


AABB RayTracer::CalculateBBIter(const Triangle& tri, AABB& box) const
{
	for(int i = 0; i<3; ++i)
	{
		box.x1 = std::min(box.x1, tri.vertices[i]->position.x);
		box.x2 = std::max(box.x2, tri.vertices[i]->position.x);
		box.y1 = std::min(box.y1, tri.vertices[i]->position.y);
		box.y2 = std::max(box.y2, tri.vertices[i]->position.y);
		box.z1 = std::min(box.z1, tri.vertices[i]->position.z);
		box.z2 = std::max(box.z2, tri.vertices[i]->position.z);
	}
	return box;
}

void RayTracer::CalculateBB(std::vector<Triangle>& triangles, AABB& box, const int start, const int end) const
{
	box.x1=999999,box.x2=-999999,box.y1=999999,box.y2=-999999,box.z1=999999,box.z2=-999999;
	for(int j = start; j <= end; ++j)
	{
		auto tri = triangles[j];
		for(int i = 0; i<3; ++i)
		{
			box.x1 = std::min(box.x1, tri.vertices[i]->position.x);
			box.x2 = std::max(box.x2, tri.vertices[i]->position.x);
			box.y1 = std::min(box.y1, tri.vertices[i]->position.y);
			box.y2 = std::max(box.y2, tri.vertices[i]->position.y);
			box.z1 = std::min(box.z1, tri.vertices[i]->position.z);
			box.z2 = std::max(box.z2, tri.vertices[i]->position.z);
		}
	}
}

struct VectorSortX {
    bool operator()(const Triangle& a, const Triangle& b) const 
	{
		float mid1 = 0;
		float mid2 = 0;
		for(int i = 0; i<3; ++i)
		{
			mid1 += a.vertices[i]->position.x / 3.0f;
			mid2 += b.vertices[i]->position.x / 3.0f;
		}
        return mid1 < mid2;
    }
};

struct VectorSortY {
    bool operator()(const Triangle& a, const Triangle& b) const 
	{
		float mid1 = 0;
		float mid2 = 0;
		for(int i = 0; i<3; ++i)
		{
			mid1 += a.vertices[i]->position.y / 3.0f;
			mid2 += b.vertices[i]->position.y / 3.0f;
		}
        return mid1 < mid2;
    }
};

struct VectorSortZ {
    bool operator()(const Triangle& a, const Triangle& b) const 
	{
		float mid1 = 0;
		float mid2 = 0;
		for(int i = 0; i<3; ++i) 
		{
			mid1 += a.vertices[i]->position.z / 3.0f;
			mid2 += b.vertices[i]->position.z / 3.0f;
		}
        return mid1 < mid2;
    }
};

int RayTracer::determineSplit(const int start, const int end, std::vector<Triangle>& triangles, float* rightcosts) const
{
	//return (start+end)/2;
	int minsplit = (start+end)/2;
	int tris = end-start+1;
	int step = sqrt(float(tris));
	float leftcost, mincost = 99999999, cost;
	int minaxis = 0;
	//float rightcosts[300000];
	for(int j = 0; j<3; ++j)
	{
		if(j==0)
			std::sort(triangles.begin()+start, triangles.begin()+end+1, VectorSortX());
		if(j==1)
			std::sort(triangles.begin()+start, triangles.begin()+end+1, VectorSortY());
		if(j==2)
			std::sort(triangles.begin()+start, triangles.begin()+end+1, VectorSortZ());
		AABB box;
		box.x1=999999,box.x2=-999999,box.y1=999999,box.y2=-999999,box.z1=999999,box.z2=-999999;
		for(int i = end; i>=start; --i)
		{
			box = CalculateBBIter(triangles[i], box);
			rightcosts[i] = 2*(box.x2/1000.0f - box.x1/1000.0f)*(box.y2/1000.0f - box.y1/1000.0f)+2*(box.y2/1000.0f-box.y1/1000.0f)*(box.z2/1000.0f-box.z1/1000.0f)+2*(box.x2/1000.0f-box.x1/1000.0f)*(box.z2/1000.0f-box.z1/1000.0f);
		}
		box.x1=999999,box.x2=-999999,box.y1=999999,box.y2=-999999,box.z1=999999,box.z2=-999999;
		for(int i = start; i<=end; i++)
		{
			box = CalculateBBIter(triangles[i], box);
			leftcost = 2*(box.x2/1000.0f - box.x1/1000.0f)*(box.y2/1000.0f - box.y1/1000.0f)+2*(box.y2/1000.0f-box.y1/1000.0f)*(box.z2/1000.0f-box.z1/1000.0f)+2*(box.x2/1000.0f-box.x1/1000.0f)*(box.z2/1000.0f-box.z1/1000.0f);
			cost = leftcost * (i-start+1) + rightcosts[i] * (end-i);
			if(cost < mincost && i != start && i != end)
			{
				minsplit = i;
				mincost = cost;
				minaxis = j;
			}
		}
	}
	if(minaxis==0)
		std::sort(triangles.begin()+start, triangles.begin()+end+1, VectorSortX());
	if(minaxis==1)
		std::sort(triangles.begin()+start, triangles.begin()+end+1, VectorSortY());
	return minsplit;
}

void RayTracer::construcTree (std::vector<Triangle>& triangles, Node& N, float* rightcosts) const
{
	N.leftChild = NULL;
	N.rightChild = NULL;
	CalculateBB(triangles, N.box, N.startPrim, N.endPrim);
	int n = N.endPrim - N.startPrim+1;
	if(n>MAX_MEMS)
	{
		N.leftChild = new Node();
		N.rightChild = new Node();
		float size = max(abs(N.box.x1-N.box.x2), abs(N.box.y1-N.box.y2), abs(N.box.z1-N.box.z2));
		int sorter = -1;
		int split = determineSplit(N.startPrim, N.endPrim, triangles, rightcosts);
		N.leftChild->startPrim = N.startPrim;
		N.leftChild->endPrim = split;
		N.rightChild->startPrim = split+1;
		N.rightChild->endPrim = N.endPrim;
		construcTree(triangles, *N.rightChild, rightcosts);
		construcTree(triangles, *N.leftChild, rightcosts);
	}
};

float calculateBVHcost(const Node& N)
{
	float cost = 0;
	if(N.leftChild!=NULL)
		cost+=calculateBVHcost(*N.leftChild);
	if(N.rightChild!=NULL)
		cost+=calculateBVHcost(*N.rightChild);
	cost+=(N.box.x1 - N.box.x2)*(N.box.x1 - N.box.x2)+(N.box.y1-N.box.y2)*(N.box.y1-N.box.y2)+(N.box.z1-N.box.z2)*(N.box.z1-N.box.z2);
	return cost;
}

void RayTracer::updateAABB(std::vector<Triangle>& triangles, Node& N)
{
	CalculateBB(triangles, N.box, N.startPrim, N.endPrim);
	if (N.leftChild != NULL)
		updateAABB(triangles, *N.leftChild);
	if (N.rightChild != NULL)
		updateAABB(triangles, *N.rightChild);
}

void RayTracer::updateAABBs(std::vector<Triangle>& triangles)
{
	if (m_root != NULL)
		updateAABB(triangles, *m_root);
}

void RayTracer::constructHierarchy(std::vector<Triangle>& triangles)
{
	// YOUR CODE HERE (R1)
	m_root = new Node();//std::unique_ptr<Node>(new Node());
	m_triangles = &triangles;
	int n = triangles.size();
	float* SAH_rightcosts = new float[n];
	m_root->endPrim = n-1;
	m_root->startPrim = 0;
	construcTree(*m_triangles, *m_root, SAH_rightcosts);
	delete SAH_rightcosts;
}

void RayTracer::traceTris(const int start, const int stop, const Vec3f& dir, const Vec3f& orig, int& imin, float& t, float& umin, float& vmin, RaycastResult& result, std::vector<Vec3f> *lines) const
{
	for ( int i = start; i <= stop; ++i )
	{

		float rayt, u, v;
		if ( intersect_triangle1(	&orig.x,
									&dir.x,
									&(*m_triangles)[i].vertices[0]->position.x,
									&(*m_triangles)[i].vertices[1]->position.x,
									&(*m_triangles)[i].vertices[2]->position.x,
									rayt, u, v ) )
		{
			if ( rayt >= 0.0f && rayt < t )
			{
				imin = i;
				t = rayt;
				umin = u;
				vmin = v;
				result = RaycastResult(&(*m_triangles)[imin], t, umin, vmin, orig + t*dir, orig, dir, -1);
				result.hit = true;
			}
		}
	}
}

void RayTracer::travelBox(const Node& n, const Vec3f& dir, const Vec3f& idir, const Vec3f& orig, float& t, RaycastResult& rayresult, std::vector<Vec3f> *lines) const
{
	Node& leftChild = *n.leftChild;
	Node& rightChild = *n.rightChild;
	bool leftleaf = (leftChild.leftChild == NULL && leftChild.rightChild == NULL);
	bool rightleaf = (rightChild.leftChild == NULL && rightChild.rightChild == NULL);
	bool lefthit, righthit;
	float t1x,t2x,t1y,t2y,t1z,t2z, t1, t2;
	{
		if(dir.x > 0)
		{
			t1x = (leftChild.box.x1 - orig.x) * idir.x;
			t2x = (leftChild.box.x2 - orig.x) * idir.x;
		}
		else
		{
			t1x = (leftChild.box.x2 - orig.x) * idir.x;
			t2x = (leftChild.box.x1 - orig.x) * idir.x;
		}
		if(dir.y > 0)
		{
			t1y = (leftChild.box.y1 - orig.y) * idir.y;
			t2y = (leftChild.box.y2 - orig.y) * idir.y;
		}
		else
		{
			t1y = (leftChild.box.y2 - orig.y) * idir.y;
			t2y = (leftChild.box.y1 - orig.y) * idir.y;
		}
		if(dir.z > 0)
		{
			t1z = (leftChild.box.z1 - orig.z) * idir.z;
			t2z = (leftChild.box.z2 - orig.z) * idir.z;
		}
		else
		{
			t1z = (leftChild.box.z2 - orig.z) * idir.z;
			t2z = (leftChild.box.z1 - orig.z) * idir.z;
		}
		t1 = std::max(std::max(t1x,t1y),t1z);
		t2 = std::min(std::min(t2x,t2y),t2z);
		lefthit = (t1 <= t2 && t1 < t && t2 > 0);

		if(dir.x > 0)
		{
			t1x = (rightChild.box.x1 - orig.x) * idir.x;
			t2x = (rightChild.box.x2 - orig.x) * idir.x;
		}
		else
		{
			t1x = (rightChild.box.x2 - orig.x) * idir.x;
			t2x = (rightChild.box.x1 - orig.x) * idir.x;
		}
		if(dir.y > 0)
		{
			t1y = (rightChild.box.y1 - orig.y) * idir.y;
			t2y = (rightChild.box.y2 - orig.y) * idir.y;
		}
		else
		{
			t1y = (rightChild.box.y2 - orig.y) * idir.y;
			t2y = (rightChild.box.y1 - orig.y) * idir.y;
		}
		if(dir.z > 0)
		{
			t1z = (rightChild.box.z1 - orig.z) * idir.z;
			t2z = (rightChild.box.z2 - orig.z) * idir.z;
		}
		else
		{
			t1z = (rightChild.box.z2 - orig.z) * idir.z;
			t2z = (rightChild.box.z1 - orig.z) * idir.z;
		}
		t1x = std::max(std::max(t1x,t1y),t1z);
		t2x = std::min(std::min(t2x,t2y),t2z);
		righthit = (t1x <= t2x && t1x < t && t2x > 0);
	}
	int imin = -1;
	float umin = .0f, vmin = .0f;

	if(righthit && lefthit)
	{
		if(t1<t1x)
		{
			if(!leftleaf)
				travelBox(leftChild, dir, idir, orig, t, rayresult, lines);
			else
			{
				traceTris(leftChild.startPrim, leftChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
			}
			if(t1x < t)
			{
				if(!rightleaf)
					travelBox(rightChild, dir, idir, orig, t, rayresult, lines);
				else
				{
					traceTris(rightChild.startPrim, rightChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
				}
			}
		}
		else
		{
			if(!rightleaf)
				travelBox(rightChild, dir, idir, orig, t, rayresult, lines);
			else
			{
				traceTris(rightChild.startPrim, rightChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
			}
			if(t1 < t)
			{
				if(!leftleaf)
					travelBox(leftChild, dir, idir, orig, t, rayresult, lines);
				else
				{
					traceTris(leftChild.startPrim, leftChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
				}
			}
		}
	}
	else if(lefthit)
	{
		if(!leftleaf)
			travelBox(leftChild, dir, idir, orig, t, rayresult, lines);
		else if(t1<t)
		{
			traceTris(leftChild.startPrim, leftChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
		}
	}
	else if(righthit)
	{
		if(!rightleaf)
			travelBox(rightChild, dir, idir, orig, t, rayresult, lines);
		else if(t1x < t)
		{
			traceTris(rightChild.startPrim, rightChild.endPrim, dir, orig, imin, t, umin, vmin, rayresult, lines);
		}
	}
}


RaycastResult RayTracer::raycast(const Vec3f& orig, const Vec3f& dir, const int lightignore, std::vector<Vec3f> *lines) const {
	float tmin = 1;
	RaycastResult castresult;
	castresult.t = 2;
	castresult.lightmin = -1;
	castresult.point = orig+dir;
	Vec3f idir(1.0f/dir.x, 1.0f/dir.y, 1.0f/dir.z);
	if (m_root != NULL && m_root->leftChild != NULL)
		travelBox(*m_root, dir, idir, orig, tmin, castresult, lines);
	return castresult;
}

} // namespace FW
