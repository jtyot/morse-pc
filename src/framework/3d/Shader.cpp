#include "Shader.hpp"

using namespace FW;
using namespace std;

string loadshader(const string s)
{
	std::ifstream t(s);
	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

Shader::Shader(string namein, GLContext* glCtx)
{
	name = namein;
	load(glCtx);
}

void Shader::load(GLContext* glCtx)
{
	string vertname = name;
	string fragname = name;
	string vert = loadshader(vertname.append(".vert"));
	string frag = loadshader(fragname.append(".frag"));
	auto shader_program = new GLContext::Program(FW::String(vert.c_str()), FW::String(frag.c_str()));
	if (glCtx)
		glCtx->setProgram(name.c_str(), shader_program);
	programID = shader_program->getHandle();
	GLContext::checkErrors();
}

void Shader::unload(GLContext* glCtx)
{
	auto shader_program = glCtx->getProgram(name.c_str());

	glDeleteProgram(shader_program->m_glProgram);
	glDeleteShader(shader_program->m_glVertexShader);
	glDeleteShader(shader_program->m_glGeometryShader);
	glDeleteShader(shader_program->m_glFragmentShader);
}

void Shader::reload(GLContext* glCtx)
{
	unload(glCtx);

	string vertname = name;
	string fragname = name;
	string vert = loadshader(vertname.append(".vert"));
	string frag = loadshader(fragname.append(".frag"));
	auto shader_program = glCtx->getProgram(name.c_str());
	shader_program->init(vert.c_str(), 0, 0, 0, "", frag.c_str());
}

void Shader::use(void) const
{
	glUseProgram(programID);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const float value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform1f(uniformMap[name], value);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const int value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform1i(uniformMap[name], value);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const Vec2f value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform2f(uniformMap[name], value.x, value.y);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const Vec3f value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform3f(uniformMap[name], value.x, value.y, value.z);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const Vec4f value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform4f(uniformMap[name], value.x, value.y, value.z, value.w);
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const Mat4f value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniformMatrix4fv(uniformMap[name], 1, false, value.getPtr());
	GLContext::checkErrors();
}

void Shader::sendUniform(const string name, const Mat3f value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniformMatrix3fv(uniformMap[name], 1, false, value.getPtr());
	GLContext::checkErrors();
}