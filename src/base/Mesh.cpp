#include "Mesh.hpp"
#include "RayTracer.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace std;
using namespace FW;


vector<Vertex> unpackIndexedData(
	const vector<Vec3f>& positions,
	const vector<Vec3f>& normals,
	const vector<Vec2f>& uvs,
	vector<Face>& faces)
{
	vector<Vertex> vertices;
	for (auto& f : faces) {
		Vertex v0, v1, v2;

		if (f.position_indices.x < 0)
			f.position_indices.x = positions.size() + 1 + f.position_indices.x;
		if (f.position_indices.y < 0)
			f.position_indices.y = positions.size() + 1 + f.position_indices.y;
		if (f.position_indices.z < 0)
			f.position_indices.z = positions.size() + 1 + f.position_indices.z;

		v0.position = positions[f.position_indices.x];
		v1.position = positions[f.position_indices.y];
		v2.position = positions[f.position_indices.z];

		if (normals.size() > 0)
		{
			if (f.normal_indices.x < 0)
				f.normal_indices.x = normals.size() + 1 + f.normal_indices.x;
			if (f.normal_indices.y < 0)
				f.normal_indices.y = normals.size() + 1 + f.normal_indices.y;
			if (f.normal_indices.z < 0)
				f.normal_indices.z = normals.size() + 1 + f.normal_indices.z;

			v0.normal = normals[f.normal_indices.x];
			v1.normal = normals[f.normal_indices.y];
			v2.normal = normals[f.normal_indices.z];
		}
		else
			v0.normal = v1.normal = v2.normal = 0;

		if (uvs.size() > 0)
		{
			if (f.uv_indices.x < 0)
				f.uv_indices.x = uvs.size() + 1 + f.uv_indices.x;
			if (f.uv_indices.y < 0)
				f.uv_indices.y = uvs.size() + 1 + f.uv_indices.y;
			if (f.uv_indices.z < 0)
				f.uv_indices.z = uvs.size() + 1 + f.uv_indices.z;
			v0.uv = uvs[f.uv_indices.x];
			v1.uv = uvs[f.uv_indices.y];
			v2.uv = uvs[f.uv_indices.z];
		}
		else
		{
			v0.uv = v1.uv = v2.uv = 0;
		}
		v0.material_name = v1.material_name = v2.material_name = f.material_name;
		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v2);
	}

	for (int i = 0; i < vertices.size(); i += 3)
	{
		Vertex v0 = vertices[i + 0];
		Vertex v1 = vertices[i + 1];
		Vertex v2 = vertices[i + 2];
		Vec3f Q1 = v1.position - v0.position;
		Vec3f Q2 = v2.position - v0.position;
		Vec2f st1 = v1.uv - v0.uv;
		Vec2f st2 = v2.uv - v0.uv;
		Mat3f st, Q;
		st.setRow(0, Vec3f(st2.y, -st1.y, 0));
		st.setRow(1, Vec3f(-st2.x, st1.x, 0));
		st.setRow(2, Vec3f(0));
		Q.setRow(0, Q1);
		Q.setRow(1, Q2);
		Q.setRow(2, Vec3f(0));
		Mat3f TB = st * Q / (st1.x * st2.y - st2.x * st1.y);
		Vec3f T = TB.getRow(0);
		Vec3f B = TB.getRow(1);
		vertices[i + 0].tangent += T;
		vertices[i + 1].tangent += T;
		vertices[i + 2].tangent += T;

		vertices[i + 0].bitangent += B;
		vertices[i + 1].bitangent += B;
		vertices[i + 2].bitangent += B;
	}
	return vertices;
};


void Mesh::updatetransform(Random rnd, float rotmult, float velmult, bool advanceframe)
{
	//time++;
	float t = rnd.getF32(-1, 1);
	if (advanceframe)
		t = 1;
	transformation = transformation_previous;
	Vec3f pos = Vec4f(transformation.getCol(3)).getXYZ();
	Vec3f velocity = vel;
	if (orbitspoint)
	{
		Mat3f orbitrot = Mat3f::rotation(Vec3f(0, 1, 0), orbitspeed*t*velmult);
		pos = orbitpoint + orbitrot * orbitoffset;
	}
	transformation.setCol(3, Vec4f(pos + velocity, 1.0f));

	Mat3f Rx = Mat3f::rotation(Vec3f(0, 0, 1), rotation_vel.z*t*rotmult);
	Mat3f Ry = Mat3f::rotation(Vec3f(0, 1, 0), rotation_vel.y*t*rotmult);
	Mat3f Rz = Mat3f::rotation(Vec3f(1, 0, 0), rotation_vel.x*t*rotmult);
	Rx = Rx * Ry * Rz;

	Mat4f rotate = Mat4f();
	rotate.setCol(0, Vec4f(Rx.getCol(0), .0f));
	rotate.setCol(1, Vec4f(Rx.getCol(1), .0f));
	rotate.setCol(2, Vec4f(Rx.getCol(2), .0f));
	transformation *= rotate;
	transformation_previous = transformation;
}

Mesh::Mesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Set up vertex attribute object for dynamic data. We'll load the actual data later, whenever the model changes.
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	GLContext::checkErrors();
}

void Mesh::loadMaterials(string filename, string filepath)
{
	ifstream input(filename, ios::in);
	string line;
	string current_material = "";
	while (getline(input, line)) {

		istringstream        iss(line);

		Vec3f               v;
		string              s;
		float				f;

		iss >> s;

		if (s == "#")
			continue;
		if (s == "newmtl")
		{
			iss >> current_material;
			submeshes[current_material].buffersExist = true;
		}
		else if (s == "Ka" || s == "Ke")
		{
			iss >> v.x >> v.y >> v.z;
			materialMap[current_material].emissioncol = v;
		}
		else if (s == "Kd")
		{
			iss >> v.x >> v.y >> v.z;
			materialMap[current_material].diffusecol = v;
		}
		else if (s == "Ks")
		{
			iss >> v.x >> v.y >> v.z;
			materialMap[current_material].specularcol = v;
		}
		else if (s == "d")
		{
			iss >> f;
			materialMap[current_material].alpha = f;
		}
		else if (s == "Ns")
		{
			iss >> f;
			materialMap[current_material].glossiness = f;
		}
		else if (s == "Kt")
		{
			iss >> v.x >> v.y >> v.z;
			materialMap[current_material].transmitcol = v;
		}
		else if (s == "specular_col_mult")
		{
			iss >> f;
			materialMap[current_material].specularColorMult = f;
		}
		else if (s == "specular_col_pow")
		{
			iss >> f;
			materialMap[current_material].specularColorPow = f;
		}
		else if (s == "specular_exp_mult")
		{
			iss >> f;
			materialMap[current_material].specularExpMult = f;
		}
		else if (s == "specular_exp_pow")
		{
			iss >> f;
			materialMap[current_material].specularExpPow = f;
		}
		else if (s == "map_Ka")
		{
			iss >> s;
			string str = filepath;
			str.append(s);
			materialMap[current_material].hasEmissionTexture = true;
			materialMap[current_material].emissionTexture = Texture::import(str.c_str());
		}
		else if (s == "map_Kd")
		{
			iss >> s;
			string str = filepath;
			str.append(s);
			materialMap[current_material].hasDiffuseTexture = true;
			materialMap[current_material].diffuseTexture = Texture::import(str.c_str());
		}
		else if (s == "map_Ks")
		{
			iss >> s;
			string str = filepath;
			str.append(s);
			materialMap[current_material].hasSpecularTexture = true;
			materialMap[current_material].specularTexture = Texture::import(str.c_str());
		}
		else if (s == "map_d")
		{
			iss >> s;
			string str = filepath;
			str.append(s);
			materialMap[current_material].hasAlphaTexture = true;
			materialMap[current_material].alphaTexture = Texture::import(str.c_str());
		}
		else if (s == "bump" || s == "map_bump")
		{
			if (s == "map_bump")
				materialMap[current_material].normalTextureInObjectSpace = true;
			iss >> s;
			string str = filepath;
			str.append(s);
			materialMap[current_material].hasNormalTexture = true;
			materialMap[current_material].normalTexture = Texture::import(str.c_str());
		}
	}
}

void Mesh::unload()
{
	for (auto it : submeshes)
	{
		auto submesh = it.second;
		if (!submesh.buffersExist)
			continue;
		glDeleteBuffers(1, &(submesh.posFBO));
		glDeleteBuffers(1, &(submesh.uvFBO));
		glDeleteBuffers(1, &(submesh.normalFBO));
		glDeleteBuffers(1, &(materialMap[submesh.materialName].UBO));
		GLContext::checkErrors();
		submesh.buffersExist = false;
	}
	positions.clear();
	normals.clear();
	uvs.clear();
	faces.clear();
	vertices.clear();
	lightTris.clear();
	triangles.clear();
}

void Mesh::upload()
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	needsupload = false;
}

void Mesh::draw(Shader& shdr, bool usematerials)
{
	if (needsupload)
		upload();

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
	GLContext::checkErrors();


	for (auto it : submeshes)
	{
		if (it.second.start < 0)
			continue;
		auto submesh = it.second;
		MaterialData& mat = materialMap[submesh.materialName];

		shdr.sendUniform("uModelToWorld", transformation);
		shdr.sendUniform("uModelToWorld_previous", transformation_previous);
		shdr.sendUniform("uNormalTransform", transformation.inverted().transposed());
		GLContext::checkErrors();
		if (usematerials)
		{
			//glBindBuffer(GL_UNIFORM_BUFFER, mat.UBO);

			vector<Vec4f> UBO;
			UBO.push_back(Vec4f(mat.diffusecol, mat.alpha));
			UBO.push_back(Vec4f(mat.specularcol, mat.glossiness));
			UBO.push_back(Vec4f(mat.emissioncol, .0f));
			//cout << mat.diffusecol.x << endl;
			shdr.sendUniform("col_alpha", UBO[0]);
			shdr.sendUniform("specular_glossiness", UBO[1]);
			shdr.sendUniform("emission_padding", UBO[2]);
			shdr.sendUniform("transmit_color", mat.transmitcol / scale);
			shdr.sendUniform("specular_col_mult", mat.specularColorMult);
			shdr.sendUniform("specular_col_pow", mat.specularColorPow);
			shdr.sendUniform("specular_exp_mult", mat.specularExpMult);
			shdr.sendUniform("specular_exp_pow", mat.specularExpPow);
			GLContext::checkErrors();

			if (mat.hasDiffuseTexture)
			{
				GLuint tex = mat.diffuseTexture.getGLTexture();
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, tex);
				shdr.sendUniform("albedotex", 4);
				shdr.sendUniform("hasalbedotex", 1.0f);
			}
			else
				shdr.sendUniform("hasalbedotex", .0f);

			if (mat.hasSpecularTexture)
			{
				GLuint tex = mat.specularTexture.getGLTexture(ImageFormat::ID_Max, true, false);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, tex);
				shdr.sendUniform("speculartex", 5);
				shdr.sendUniform("hasspeculartex", 1.0f);
			}
			else
				shdr.sendUniform("hasspeculartex", .0f);

			if (mat.hasEmissionTexture)
			{
				GLuint tex = mat.emissionTexture.getGLTexture();
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, tex);
				shdr.sendUniform("emissiontex", 6);
				shdr.sendUniform("hasemissiontex", 1.0f);
			}
			else
				shdr.sendUniform("hasemissiontex", .0f);

			if (mat.hasNormalTexture)
			{
				GLuint tex = mat.normalTexture.getGLTexture(ImageFormat::ID_Max, true, false);
				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, tex);
				shdr.sendUniform("bumpmap", 7);
				shdr.sendUniform("hasbumpmap", 1.0f);
			}
			else
				shdr.sendUniform("hasbumpmap", .0f);
			shdr.sendUniform("bumpmap_objectspace", mat.normalTextureInObjectSpace ? 1.0f : .0f);
			if (mat.normalTextureInObjectSpace)
				shdr.sendUniform("normal_transform", transformation.inverted().transposed());
			else
				shdr.sendUniform("normal_transform", Mat4f());

		}
		if (mat.hasAlphaTexture)
		{
			GLuint tex = mat.alphaTexture.getGLTexture();
			glActiveTexture(GL_TEXTURE7 + 1);
			glBindTexture(GL_TEXTURE_2D, tex);
			shdr.sendUniform("stenciltexture", 7 + 1);
			shdr.sendUniform("hasstenciltexture", 1.0f);
		}
		else
			shdr.sendUniform("hasstenciltexture", .0f);
		GLContext::checkErrors();
		glDrawArrays(GL_TRIANGLES, submesh.start * 3, (submesh.end - submesh.start) * 3);
	}
}


void Mesh::load(string filename) {
	unload();
	positions.reserve(10000);
	faces.reserve(10000);
	uvs.reserve(10000);
	normals.reserve(10000);

	string file_temp = filename;
	for (auto& c : file_temp)
		if (c == '\\')
			c = ' ';
	istringstream        iss(file_temp);
	string s;
	string filepath;
	while (true)
	{
		iss >> s;
		if (s == "" || s.find(".obj") != string::npos || s.find(".OBJ") != string::npos)
			break;
		else
		{
			filepath.append(s);
			filepath.append("\\");
		}
	}

	ifstream input(filename, ios::in);

	string line;
	string currentmtl = "";

	GLContext::checkErrors();
	submeshes[currentmtl].buffersExist = true;
	materialMap[currentmtl].diffusecol = 1;
	materialMap[currentmtl].specularcol = 1;
	materialMap[currentmtl].glossiness = 1;

	cout << "loading mesh " << filename << endl;

	bool foundobj = false;
	while (getline(input, line)) {
		foundobj = true;
		// Replace any '/' characters with spaces ' ' so that all of the
		// values we wish to read are separated with whitespace.
		for (int i = 0; i < line.size(); i++)
		{
			auto& c = line[i];
			if (c == '/')
			{
				c = ' ';
				auto& c2 = line[i + 1];
				if (c2 == '/')
				{
					i++;
					line.insert(i, 1, '/');
				}
			}
		}

		// Temporary objects to read data into.
		Face  f, f1, f2; // Face index array
		Vec3f               v;
		Vec2f				uv;
		string              s;

		// Create a stream from the string to pick out one value at a time.
		istringstream        iss(line);

		// Read the first token from the line into string 's'.
		// It identifies the type of object (vertex or normal or ...)
		iss >> s;

		if (s == "#")
			continue;
		if (s == "v") { // vertex position
			iss >> v.x >> v.y >> v.z;
			positions.push_back(v);
		}
		else if (s == "vn") { // normal
			iss >> v.x >> v.y >> v.z;
			normals.push_back(v);
		}
		else if (s == "vt") { // texture
			iss >> uv.x >> uv.y;
			uv.y = 1.0 - uv.y;
			uvs.push_back(uv);
		}
		else if (s == "mtllib") { // load material library
			iss >> s;
			string str = filepath;
			str.append(s);
			loadMaterials(str, filepath);
			cout << "loaded materiallib " << str << endl;
		}
		else if (s == "usemtl") { // use new material
			iss >> currentmtl;
			submeshes[currentmtl].materialName = currentmtl;
			submeshes[currentmtl].start = faces.size();
		}
		else if (s == "f") { // face
			for (int i = 0; i < 3; i++)
			{
				//iss >> f.position_indices[i] >> f.uv_indices[i] >> f.normal_indices[i];
				iss >> s;
				if (s != "/")
					f.position_indices[i] = atoi(s.c_str());
				else
					f.position_indices[i] = 1;
				iss >> s;
				if (s != "/")
					f.uv_indices[i] = atoi(s.c_str());
				else
					f.uv_indices[i] = 1;
				iss >> s;
				if (s != "/")
					f.normal_indices[i] = atoi(s.c_str());
				else
					f.normal_indices[i] = 1;
				f.position_indices[i]--;
				f.uv_indices[i]--;
				f.normal_indices[i]--;
				f1 = f;
				f2 = f;
			}
			if (submeshes[currentmtl].start < 0)
				submeshes[currentmtl].start = 0;
			f.material_name = currentmtl;
			faces.push_back(f);
			//submeshes[currentmtl].faces.push_back(faces.size() - 1);
			submeshes[currentmtl].end = faces.size();
			while (!iss.eof())
			{
				iss >> s;
				if (s != "/")
					f.position_indices[2] = atoi(s.c_str());
				else
					f.position_indices[2] = 1;
				iss >> s;
				if (s != "/")
					f.uv_indices[2] = atoi(s.c_str());
				else
					f.uv_indices[2] = 1;
				iss >> s;
				if (s != "/")
					f.normal_indices[2] = atoi(s.c_str());
				else
					f.normal_indices[2] = 1;
				f.position_indices[2]--;
				f.uv_indices[2]--;
				f.normal_indices[2]--;
				f.position_indices[0] = f1.position_indices[0];
				f.normal_indices[0] = f1.normal_indices[0];
				f.uv_indices[0] = f1.uv_indices[0];
				f.position_indices[1] = f2.position_indices[2];
				f.normal_indices[1] = f2.normal_indices[2];
				f.uv_indices[1] = f2.uv_indices[2];
				faces.push_back(f);
				submeshes[currentmtl].end = faces.size();
				f2 = f;
				//iss >> s;
			}
		}
	}
	if (!foundobj)
		return;
	cout << "computing vertices...\n";
	vertices = unpackIndexedData(positions, normals, uvs, faces);
	lightTris.clear();
	triangles.clear();
	triangles.resize(vertices.size() / 3);
	for (int i = 0; i < vertices.size(); i += 3)
	{
		triangles[i / 3].vertices[0] = &vertices[i + 0];
		triangles[i / 3].vertices[1] = &vertices[i + 1];
		triangles[i / 3].vertices[2] = &vertices[i + 2];
		if (materialMap[vertices[i + 0].material_name].emissioncol.length() > 0)
			lightTris.push_back(triangles[i / 3]);
	}
	cout << "mesh " << filename << " loaded!\n";
	cout << "Triangles: " << triangles.size() << endl;
	cout << "Vecrtices: " << vertices.size() << endl;
	needsupload = true;
}