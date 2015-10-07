#pragma once

#include "gui/Window.hpp"
#include "3d/Texture.hpp"
#include "Shader.hpp"
#include "base\Random.hpp"

#include <string>
#include<iostream>
#include<vector>
#include<map>
#include<array>


namespace FW
{
	struct MaterialData
	{
		Vec3f diffusecol = .6f, specularcol = 1.0f, emissioncol = .0f, transmitcol = .0f;
		float glossiness = 1;
		bool hasDiffuseTexture = false;
		Texture diffuseTexture;
		bool hasSpecularTexture = false;
		Texture specularTexture;
		float specularColorMult = 1.0f;
		float specularColorPow = 1.0f;
		float specularExpMult = 1.0f;
		float specularExpPow = 1.0f;
		bool hasEmissionTexture = false;
		Texture normalTexture;
		bool hasNormalTexture = false;
		bool normalTextureInObjectSpace = false;
		Texture emissionTexture;
		bool hasAlphaTexture = false;
		Texture alphaTexture;
		float alpha = 1.0f;
		GLuint UBO;
	};

	struct Vertex
	{
		Vec3f position;
		Vec3f normal;
		Vec3f tangent;
		Vec3f bitangent;
		Vec2f uv;
		int particle_index = -1;
		std::string material_name;
	};

	struct Triangle
	{
		std::array<Vertex*, 3> vertices;
	};

	struct Face
	{
		Vec3i position_indices;
		Vec3i normal_indices;
		Vec3i uv_indices;
		std::string material_name;
	};

	struct Submesh
	{
		int start = -1;
		int end = -1;
		std::vector<int> faces;
		std::string materialName;
		GLuint posFBO, uvFBO, normalFBO, tangentFBO, bitangentFBO;
		bool buffersExist = false;
		Submesh::Submesh()
		{
			glGenBuffers(1, &posFBO);
			glGenBuffers(1, &uvFBO);
			glGenBuffers(1, &normalFBO);
			glGenBuffers(1, &tangentFBO);
			glGenBuffers(1, &bitangentFBO);
		}
	};

	class Mesh
	{
	public:
		Mesh::Mesh();

		std::vector<Triangle> lightTris;
		std::vector<Triangle> triangles;
		std::vector<Face> faces;
		std::vector<Vertex> vertices;
		std::vector<Vec3f> positions, normals;
		std::vector<Vec2f> uvs;
		Mat4f transformation;
		Mat4f transformation_previous;
		Vec3f vel, rotation_vel;
		Vec3f orbitpoint;
		float orbitspeed = .01;
		float scale = 1.0;
		Vec3f orbitoffset;
		int time = 0;
		bool orbitspoint = false;
		std::map<std::string, MaterialData> materialMap;
		std::map<std::string, Submesh> submeshes;
		bool needsupload = false;

		GLuint VAO, VBO;

		void updatetransform(Random rnd, float rotmult, float velmult, bool advanceframe);
		void unload();
		void load(std::string filename);
		void loadMaterials(std::string filename, std::string filepath);
		void draw(Shader& shdr, bool usematerials = true);
		void upload();

		//Random rand;
	};

}