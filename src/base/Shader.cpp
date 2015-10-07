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

Shader::Shader(string namein, GLContext* glCtx, bool loadgeometryshader)
{
	name = namein;
	load(glCtx, loadgeometryshader);
}

void Shader::load(GLContext* glCtx, bool loadgeometryshader)
{
	string vert = loadshader("shaders/" + name + ".vert");
	string frag = loadshader("shaders/" + name + ".frag");
	string geom;
	if (loadgeometryshader)
		geom = loadshader("shaders/" + name + ".geom");
	GLContext::Program *shader_program;
	if(!loadgeometryshader)
		shader_program = new GLContext::Program(FW::String(vert.c_str()), FW::String(frag.c_str()));
	else
		shader_program = new GLContext::Program(FW::String(vert.c_str()), 0, 0, 0, FW::String(geom.c_str()), FW::String(frag.c_str()));
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

void Shader::use(void)
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

void Shader::sendUniform(const std::string name, const vector<int> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform1iv(uniformMap[name], value.size(), value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<float> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform1fv(uniformMap[name], value.size(), value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<Vec2f> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform2fv(uniformMap[name], value.size(), (float*)value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<Vec3f> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform3fv(uniformMap[name], value.size(), (float*)value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<Vec4f> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniform4fv(uniformMap[name], value.size(), (float*)value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<Mat3f> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniformMatrix3fv(uniformMap[name], value.size(), false, (float*)value.data());
	GLContext::checkErrors();
}

void Shader::sendUniform(const std::string name, const vector<Mat4f> value)
{
	if (uniformMap.count(name) == 0)
		uniformMap[name] = glGetUniformLocation(programID, name.c_str());
	glUniformMatrix4fv(uniformMap[name], value.size(), false, (float*)value.data());
	GLContext::checkErrors();
}