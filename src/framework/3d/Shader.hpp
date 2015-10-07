#pragma once
#include "gui/Window.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace FW
{
	class Shader
	{
	public:
		GLuint programID;
		std::string name;
		std::map<std::string, GLuint> uniformMap;

		Shader::Shader(){};
		Shader::Shader(std::string name, GLContext* glCtx);

		void				unload(GLContext* glCtx);
		void				load(GLContext* glCtx);
		void				reload(GLContext* glCtx);
		void				use(void) const; 

		void				sendUniform(const std::string name, const float value);
		void				sendUniform(const std::string name, const int value);
		void				sendUniform(const std::string name, const Vec2f value);
		void				sendUniform(const std::string name, const Vec3f value);
		void				sendUniform(const std::string name, const Vec4f value);
		void				sendUniform(const std::string name, const Mat3f value);
		void				sendUniform(const std::string name, const Mat4f value);
	};
}