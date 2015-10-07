#include "Light.hpp"
#include "utility.hpp"

using namespace std;
using namespace FW;


Light::Light(Vec3f pos, Vec3f front, Vec3f emission)
{
	position = pos;
	camera.setKeepAligned(false);
	camera.setPosition(pos);
	camera.setFOV(140);
	camera.setForward(front);
	E = emission;
}

void Light::copy(const Light& light)
{
	position = light.position;
	size = light.size;
	area = light.area;
	camera.setKeepAligned(false);
	camera.setPosition(light.position);
	camera.setFOV(light.camera.getFOV());
	camera.setForward(light.camera.getForward());
	E = light.E;
}

Light Light::getbounce(std::vector<RayTracer>& rt, std::vector<Mesh>& mesh, Random rnd)
{
	Mat3f basis = formBasis(camera.getForward());
	float r = sin(camera.getFOV() * .5 * 3.1415f / 180.0f);
	float phi = rnd.getF32(0, 2.0f * FW_PI);
	float u = rnd.getF32(0, r);
	float pdf = 1.0f / (FW_PI * r * r);
	Vec3f dir = Vec3f(cos(phi) * u, sin(phi) * u, 0);
	dir.z = sqrt(1 - dir.x*dir.x - dir.y*dir.y);
	dir.normalize();
	dir = basis * dir;
	bool hitcloth = false;
	Light returnlight;
	RaycastResult result = castRay(camera.getPosition(), camera.getPosition() + dir, mesh, rt);//rt.raycast(camera.getPosition(), dir * 99.0);
	//RaycastResult result2 = cloth_rt.raycast(camera.getPosition(), dir * 99.0);
	/*if (result2.t < result.t)
	{
		result = result2;
		hitcloth = true;
	}*/
	if (result.tri != nullptr)
	{
		Vec3f n = normalize(result.tri->vertices[0]->normal * (1.0f - result.u - result.v) + result.tri->vertices[1]->normal * result.u + result.tri->vertices[2]->normal * result.v);
		n = (mesh[result.lightmin].transformation.inverted().transposed() * Vec4f(n, .0f)).getXYZ();
		Vec3f diffusecol;
		MaterialData mat = mesh[result.lightmin].materialMap[result.tri->vertices[0]->material_name];
		if (mat.hasDiffuseTexture)
		{
			Vec2f texcoord = (result.tri->vertices[0]->uv * (1.0f - result.u - result.v) + result.tri->vertices[1]->uv * result.u + result.tri->vertices[2]->uv * result.v);
			texcoord.x -= floor(texcoord.x);
			texcoord.y -= floor(texcoord.y);
			if (texcoord.x == 1)
				texcoord.x = 0.9999;
			if (texcoord.y == 1)
				texcoord.y = 0.9999;
			Vec2i uv = texcoord * Vec2f(mat.diffuseTexture.getSize());
			diffusecol = mat.diffuseTexture.getImage()->getVec4f(uv).getXYZ();
		}
		else
			diffusecol = mat.diffusecol;
		float cosa = max(.0f, dot(n, -dir));
		float cos_light = max(.0f, dot(camera.getForward(), dir));
		Vec3f new_E = E  * diffusecol / pdf / FW_PI; // * cosa * cos_light
		returnlight.E = new_E * area;
		if (n.y > .99f)
			n = Vec3f(.01, .99, .0);
		if (-n.y > .99f)
			n = Vec3f(.01, -.99, .0);
		returnlight.camera.setForward(n);
		returnlight.camera.setPosition(result.point + n * .0001);
		returnlight.position = result.point + n * .0001;
		returnlight.camera.setFOV(170);
	}
	else
	{
		returnlight.isvalid = false;
	}
	return returnlight;
}