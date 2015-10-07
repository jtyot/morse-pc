#pragma once
#include "gui/Window.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

namespace FW
{
	class Shader
	{
	public:
		GLuint programID;
		std::string name;
		std::map<std::string, GLuint> uniformMap;

		Shader::Shader(){};
		Shader::Shader(std::string name, GLContext* glCtx, bool loadgeometryshader = false);

		void				unload(GLContext* glCtx);
		void				load(GLContext* glCtx, bool loadgeometryshader);
		void				reload(GLContext* glCtx);
		void				use(void); 

		void				sendUniform(const std::string name, const float value);
		void				sendUniform(const std::string name, const int value);
		void				sendUniform(const std::string name, const Vec2f value);
		void				sendUniform(const std::string name, const Vec3f value);
		void				sendUniform(const std::string name, const Vec4f value);
		void				sendUniform(const std::string name, const Mat3f value);
		void				sendUniform(const std::string name, const Mat4f value);

		void				sendUniform(const std::string name, const std::vector<float> value);
		void				sendUniform(const std::string name, const std::vector<int> value);
		void				sendUniform(const std::string name, const std::vector<Vec2f> value);
		void				sendUniform(const std::string name, const std::vector<Vec3f> value);
		void				sendUniform(const std::string name, const std::vector<Vec4f> value);
		void				sendUniform(const std::string name, const std::vector<Mat3f> value);
		void				sendUniform(const std::string name, const std::vector<Mat4f> value);
	};
}