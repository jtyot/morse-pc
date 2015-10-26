#include "Renderer.hpp"

#include <array>
#include <cassert>
#include <string>
#include <type_traits>

using namespace std;

namespace FW
{

	enum VertexShaderAttributeLocations {
		ATTRIB_POSITION = 0,
		ATTRIB_NORMAL = 1,
		ATTRIB_COLOR = 2
	};
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


	void Renderer::sendUniform(const GLuint program, const char* name, const float value)
	{
		GLuint location = glGetUniformLocation(program, name);
		glUniform1f(location, value);
	}

	void Renderer::sendUniformVec2(const GLuint program, const char* name, const Vec2f value)
	{
		GLuint location = glGetUniformLocation(program, name);
		glUniform2f(location, value.x, value.y);
	}

	void Renderer::sendUniformVec3(const GLuint program, const char* name, const Vec3f value)
	{
		GLuint location = glGetUniformLocation(program, name);
		glUniform3f(location, value.x, value.y, value.z);
	}

	void Renderer::sendUniformVec4(const GLuint program, const char* name, const Vec4f value)
	{
		GLuint location = glGetUniformLocation(program, name);
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	Renderer::Renderer() : imageProcessor(this){
		samples_per_draw = 3;
		path_depth = 3;
		aperture_size = 0;
		ambient_light = .1;
	}

	void Renderer::reloadshaders(Window& window)
	{
		auto ctx = window.getGL();
		for (auto shdr : shader)
		{
			shdr.second.reload(ctx);
		}
		reload_shaders = false;
	}

	void Renderer::renderVectorColored(std::vector<Vec3f>& vec, GLenum mode, Mat4f world_to_clip)
	{
		glDisable(GL_DEPTH_TEST);
		GLContext::checkErrors();

		shader["dot_color"].use();
		glPointSize(8.0f);

		glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
		glBufferData(GL_ARRAY_BUFFER, vec.size() * sizeof(Vec3f), vec.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)sizeof(Vec3f));

		shader["dot_color"].sendUniform("worldToClip", world_to_clip);
		glDrawArrays(mode, 0, vec.size() / 2);

		GLContext::checkErrors();
		glEnable(GL_DEPTH_TEST);
		GLContext::checkErrors();
	}

	void Renderer::renderVector(std::vector<Vec3f>& vec, GLenum mode, Mat4f world_to_clip)
	{
		glDisable(GL_DEPTH_TEST);
		GLContext::checkErrors();

		shader["dot"].use();
		glPointSize(8.0f);

		glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
		glBufferData(GL_ARRAY_BUFFER, vec.size() * sizeof(Vec3f), vec.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), (GLvoid*)0);

		shader["dot"].sendUniform("worldToClip", world_to_clip);
		glDrawArrays(mode, 0, vec.size());

		GLContext::checkErrors();
		glEnable(GL_DEPTH_TEST);
		GLContext::checkErrors();
	}

	void Renderer::initRendering(Window& window) {
		// Ask the Nvidia framework for the GLContext object associated with the window.
		// As a side effect, this initializes the OpenGL context and lets us call GL functions.
		auto ctx = window.getGL();

		float fsvbo_uv[] = {
			.0f, .0f,
			1.0f, .0f,
			1.0f, 1.0f,

			.0f, .0f,
			1.0f, 1.0f,
			.0f, 1.0f
		};

		gl = window.getGL();

		glGenBuffers(1, &fullscreenVBO_uv);
		glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO_uv);
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), fsvbo_uv, GL_STATIC_DRAW);

		framebuffer["accumulate"].create(window.getSize());
		framebuffer["accumulate"].addtexturetarget("color", GL_COLOR_ATTACHMENT0, GL_RGBA32F);
		framebuffer["accumulate"].addtexturetarget("weight", GL_COLOR_ATTACHMENT1, GL_R32F);

		shader["dot"] = Shader("dot", ctx);
		shader["dot_color"] = Shader("dot_color", ctx);
		shader["postprocess"] = Shader("postprocess", ctx);
		GLContext::checkErrors();

		// Create vertex attribute objects and buffers for vertex data.
		glGenBuffers(1, &pointVBO);
		glGenTextures(1, &imageTexture);
		glBindTexture(GL_TEXTURE_2D, imageTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLContext::checkErrors();
	}

	void Renderer::render(Window& window) {
		//glUseProgram(0);
		//if (reload_shaders)
		//	reloadshaders(window);
		// Clear screen.
		// Enable depth testing.
		glViewport(0, 0, lastsize.x, lastsize.y);
		GLContext::checkErrors();
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		GLContext::checkErrors();
		// Create textures if they haven't been created yet and update their size if the screen has changed
		if (window.getSize() / 2 != lastsize)
		{
			lastsize = window.getSize() / 2;
			framebuffer["accumulate"].resize(lastsize);
			GLContext::checkErrors();
		}

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);

		frameID++;

		framebuffer["accumulate"].bind();
		glDisable(GL_DEPTH_TEST);

		glDisable(GL_BLEND);
		shader["postprocess"].use();
		glViewport(0, 0, lastsize.x, lastsize.y);
		auto size = envimap.getSize();

		glBindTexture(GL_TEXTURE_2D, imageTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imageTexture);


		shader["postprocess"].sendUniform("tex", 0);
		shader["postprocess"].sendUniform("flip_y", Vec2f(1.0f, -1.0f));
		shader["postprocess"].sendUniform("shadowCutOff", .0f);
		shader["postprocess"].sendUniform("greyscale", 0);

		shader["postprocess"].sendUniform("use_weight", 0);
		shader["postprocess"].sendUniform("projectionoffset", Vec2f());

		glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO_uv);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid*)0);

		glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
		GLContext::checkErrors();


		glEnable(GL_BLEND);
		GLContext::checkErrors();
		glViewport(0, 0, lastsize.x, lastsize.y);
		imageProcessor.processImage(image_data, size);
		GLContext::checkErrors();

		//--------------------------------------------	POSTPROCESS

		shader["postprocess"].use();

		glViewport(0, 0, lastsize.x * 2, lastsize.y * 2);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO_uv);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, (GLvoid*)0);
		GLContext::checkErrors();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer["accumulate"].textures["color"].ID);

		shader["postprocess"].sendUniform("tex", 0);
		shader["postprocess"].sendUniform("flip_y", Vec2f(1.0f, 1.0f));
		shader["postprocess"].sendUniform("shadowCutOff", show_postprocessing ? imageProcessor.shadowCutOff : .0f);
		shader["postprocess"].sendUniform("greyscale", render_greyscale && show_postprocessing ? 1 : 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, framebuffer["accumulate"].textures["weight"].ID);
		shader["postprocess"].sendUniform("weight", 1);
		shader["postprocess"].sendUniform("use_weight", 1);
		shader["postprocess"].sendUniform("projectionoffset", show_postprocessing ? projectionoffset : Vec2f(.0f));
		cout << projectionoffset.x << ", " << projectionoffset.y << endl;
		glEnable(GL_FRAMEBUFFER_SRGB);
		glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
		glDisable(GL_FRAMEBUFFER_SRGB);

		glEnable(GL_CULL_FACE);
		// Undo our bindings.
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glBindVertexArray(0);
		glUseProgram(0);

		// Check for OpenGL errors.
		GLContext::checkErrors();
	}

}
