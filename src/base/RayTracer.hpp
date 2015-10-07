#pragma once

#include "Mesh.hpp"
#include "BvhNode.hpp"
#include "base/String.hpp"
#include <vector>
#include <memory>

namespace FW
{

//
// The ray tracer uses its own, extremely simple interface for vertices and triangles.
// This will make it extremely simple for you to rip it out and reuse in subsequent projects!
// Here's what you do:
//  1. put all vertices in a big vector,
//  2. put all triangles in a big vector,
//  3. make the triangle structs vertex pointers point to the correct places in the big vertex chunk.
//

// actual return point information for callers
struct RaycastResult {
	const Triangle* tri;
	float t;
	float u, v;
	bool hit = false;
	Vec3f point;
	Vec3f orig, dir; // convenience for tracing and visualization
	int lightmin;

	RaycastResult(const Triangle* tri, float t, float u, float v,
		Vec3f point, const Vec3f& orig, const Vec3f& dir, const int lightmin, bool inside_refraction = false)
		: tri(tri), t(t), u(u), v(v), point(point), orig(orig), dir(dir), lightmin(lightmin) {}
	RaycastResult() : tri(nullptr), t(std::numeric_limits<float>::max()),
		u(), v(), point(), orig(), dir(), lightmin() {}

	inline operator bool() { return tri != nullptr; }
};


// Given a vector n, form an orthogonal matrix with n as the last column, i.e.,
// a coordinate system aligned such that n is its local z axis.
// You'll have to fill in the implementation for this.
Mat3f formBasis(const Vec3f& n);
// Main class for tracing rays using BVHs
class RayTracer
{
public:
						RayTracer				(void);
						//~RayTracer				(void);

	void				updateAABB				(std::vector<Triangle>& triangles, Node& N);
	void				updateAABBs				(std::vector<Triangle>& triangles);
	void				constructHierarchy		(std::vector<Triangle>& triangles);
	void				construcTree			(std::vector<Triangle>& triangles, Node& N, float* rightcosts) const;
	int					determineSplit			(const int start, const int end, std::vector<Triangle>& triangles, float* rightcosts) const;
	float				getTriArea				(const Triangle& tri) const;

	void				saveHierarchy			(const char* filename, const std::vector<Triangle>& triangles);
	void				loadHierarchy			(const char* filename, std::vector<Triangle>& triangles);
	void				travelBox				(const Node& n, const Vec3f& dir, const Vec3f& idir, const Vec3f& orig, float& t, RaycastResult& rayresult, std::vector<Vec3f> *lines = NULL) const;
	void				traceTris				(const int start, const int stop, const Vec3f& dir, const Vec3f& orig, int& imin, float& t, float& umin, float& vmin, RaycastResult& result, std::vector<Vec3f> *lines = NULL) const;

	RaycastResult			raycast					(const Vec3f& orig, const Vec3f& dir, const int lightignore = -1, std::vector<Vec3f> *lines = NULL) const;

	// This function computes an MD5 checksum of the input scene data,
	// WITH the assumption that all vertices are allocated in one big chunk.
	//static FW::String	computeMD5				(const std::vector<Vec3f>& vertices);

	std::vector<Triangle>*		m_triangles;
	Node* m_root = nullptr;
	void CalculateBB(std::vector<Triangle>& triangles, AABB& box, const int start, const int end) const;
	AABB CalculateBBIter(const Triangle& tri, AABB& box) const;

	~RayTracer()
	{
		if (m_root != nullptr)
			std::free(m_root);
	}

	RayTracer(const RayTracer& rt)
	{
		m_triangles = rt.m_triangles;
		if (rt.m_root != nullptr)
			m_root = new Node(*rt.m_root);
	}
};


} // namespace FW
