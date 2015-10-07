#pragma once

#include "gui/CommonControls.hpp"
#include "3d/CameraControls.hpp"
#include "RayTracer.hpp"
#include "base/Random.hpp"
#include "Mesh.hpp"

namespace FW
{
	class Light
	{
	public:
		CameraControls camera;
		Vec3f position;
		Vec3f E;
		float size = 0.0f;
		float area = 1.0f;
		float specular_exp = 1;
		Vec3f diffuse_col = 1;
		Vec3f specular_col = 0;
		Vec3f indir;
		Vec3f n;
		bool isvalid = true;

		Light::Light(){};
		Light::Light(Vec3f pos, Vec3f front, Vec3f E);
		void copy(const Light& light);
		Light getbounce(std::vector<RayTracer>& rt, std::vector<Mesh>& mesh, Random rnd);
	};

}