#pragma once

#include "Mesh.hpp"
#include "RayTracer.hpp"
#include <string>
#include<iostream>
#include<vector>
#include<map>
#include<array>

template <typename T, size_t N>
char (&static_sizeof_array( T(&)[N] ))[N]; // declared, not defined
#define SIZEOF_ARRAY( x ) sizeof(static_sizeof_array(x))

namespace FW
{
	RaycastResult castRay(Vec3f start, Vec3f stop, std::vector<Mesh>& meshVector, std::vector<RayTracer>& rts)
	{
		RaycastResult rmin;
		rmin.t = 99;
		int i = 0;
		Vec3f orig_dir = stop - start;
		for (int i = 0; i < rts.size(); ++i)
		//for (auto& rt : rts)
		{
			auto& mesh = meshVector[i];
			auto& rt = rts[i];
			Vec4f orig = Vec4f(start, 1.0f);
			Vec4f dir = Vec4f(stop-start, .0f);

			orig = mesh.transformation.inverted() * orig;
			dir = mesh.transformation.inverted() * dir;
			//Vec3f dir = (dest - orig).getXYZ();

			RaycastResult r2 = rt.raycast(orig.getXYZ(), dir.getXYZ() * 99.0f);

			float t = r2.t;
			if (t < rmin.t)
			{
				rmin = r2;
				rmin.lightmin = i;
				rmin.dir = orig_dir*99.0f;//(mesh.transformation.inverted() * Vec4f(rmin.dir, .0f)).getXYZ();
				rmin.point = start + orig_dir*99.0f * t;//(mesh.transformation.inverted() * Vec4f(rmin.point, 1.0f)).getXYZ();
				//if (i > 0)
				//	rmin.lightmin = 1;
			}

			/*
			r_t.origin = (transform * Vec4f(r_t.origin, 1.0f)).getXYZ();
			r_t.direction = (transform * Vec4f(r_t.direction, .0f)).getXYZ();
			Hit h_t = Hit(h);
			bool hit = object_->intersect(r_t, h_t, tmin) && h_t.t < h.t;
			if (hit)
			{
				h.normal = ((inverse_transpose_ * Vec4f(h_t.normal, .0f)).getXYZ()).normalized();
				h.material = h_t.material;
				h.t = h_t.t;
			}*/
			//++i;
		}
		return rmin;
	}

}
