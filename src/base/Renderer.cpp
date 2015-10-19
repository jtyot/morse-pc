#include "Renderer.hpp"
//#include "utility.hpp"

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

	void Renderer::initRendering(glGeneratedIndices& gl, Window& window, CameraControls& cam) {
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


		glGenBuffers(1, &fullscreenVBO_uv);
		glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO_uv);
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), fsvbo_uv, GL_STATIC_DRAW);
/*
		framebuffer["shadow"].create(Vec2i(2000, 2000));
		framebuffer["shadow"].addtexturearray(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32, lightsamples);

		framebuffer["model"].create(window.getSize());
		framebuffer["model"].addtexturetarget("canvas", GL_COLOR_ATTACHMENT0, GL_RGBA32F);
		framebuffer["model"].addtexturetarget("depth", GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32);*/

		framebuffer["accumulate"].create(window.getSize());
		framebuffer["accumulate"].addtexturetarget("color", GL_COLOR_ATTACHMENT0, GL_RGBA32F);
		framebuffer["accumulate"].addtexturetarget("weight", GL_COLOR_ATTACHMENT1, GL_R32F);

		shader["dot"] = Shader("dot", ctx);
		shader["dot_color"] = Shader("dot_color", ctx);
		//shader["shadow"] = Shader("shadow", ctx, true);
		//shader["model"] = Shader("model", ctx);
		shader["postprocess"] = Shader("postprocess", ctx);
		//shader["accumulate"] = Shader("accumulate", ctx);
		//shader["particle"] = Shader("particle", ctx);
		GLContext::checkErrors();

		// Create vertex attribute objects and buffers for vertex data.
		glGenBuffers(1, &pointVBO);
		glGenVertexArrays(1, &gl.static_vao);
		glGenBuffers(1, &gl.static_vertex_buffer);
		glGenTextures(1, &imageTexture);
		glBindTexture(GL_TEXTURE_2D, imageTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLContext::checkErrors();
	}

	void Renderer::render(glGeneratedIndices& gl, Window& window, CameraControls& cam, bool drawscreen, float shuttertime, float timestep) {
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
			//framebuffer["model"].resize(lastsize);
			framebuffer["accumulate"].resize(lastsize);
			GLContext::checkErrors();
		}

		// Set up a matrix to transform from world space to clip space.
		// Clip space is a [-1, 1]^3 space where OpenGL expects things to be
		// when it starts drawing them.
		Mat4f C;
		Mat3f rot = cam.getOrientation().transposed();
		C.setCol(0, Vec4f(rot.getCol(0), 0));
		C.setCol(1, Vec4f(rot.getCol(1), 0));
		C.setCol(2, Vec4f(rot.getCol(2), 0));
		C.setCol(3, Vec4f(-cam.getPosition(), 1));
		Mat4f world_to_clip;
		Random rnd(framenum * samples_per_draw);
		GLContext::checkErrors();
		// Simple perspective.
		static const float fnear = 0.01f, ffar = 40.0f;
		Vec4f camvel = Vec4f(.01, .0, .02, .0)*.0;
		float rotation_vel = .0;
		//depth of field
		float dx = 1, dy = 1;
		while (dx*dx + dy*dy > 1)
		{
			dx = rnd.getF32(-1, 1);
			dy = rnd.getF32(-1, 1);
		}
		Vec2f dof_d = Vec2f(dx, dy);
		float r = length(dof_d);
		float r2 = r * 1.4;
		if (r > .5)
			r2 += (r - .5) * 0.6;
		dof_d.normalize();
		dof_d *= r2 * aperture_size;
		dx = dof_d.x;
		dy = dof_d.y;

		//anti aliasing
		float dx1 = 1, dy1 = 1;

		while (dx1*dx1 + dy1*dy1 > 1)
		{
			dx1 = rnd.getF32(-1, 1);
			dy1 = rnd.getF32(-1, 1);
		}
		dx1 /= window.getSize().y / fnear;
		dy1 /= window.getSize().y / fnear;

		float aspect = float(window.getSize().x) / float(window.getSize().y);
		r = tan(fov*.5f * 3.142f / 180.0f)*focal_dist;
		float n_per_f = fnear / focal_dist;
		float r1 = (r*aspect + dx) * n_per_f + dx1;
		float l1 = (-r*aspect + dx) * n_per_f + dx1;
		float t1 = (r + dy) * n_per_f + dy1;
		float b1 = (-r + dy) * n_per_f + dy1;
		Mat4f P;
		P.setRow(0, Vec4f(2.0f * fnear / (r1 - l1), .0f, (r1 + l1) / (r1 - l1), .0f));
		P.setRow(1, Vec4f(.0f, 2.0f * fnear / (t1 - b1), (t1 + b1) / (t1 - b1), .0f));
		P.setRow(2, Vec4f(.0f, .0f, -(ffar + fnear) / (ffar - fnear), -2 * ffar*fnear / (ffar - fnear)));
		P.setRow(3, Vec4f(.0f, .0f, -1.0f, .0f));

		Mat4f offset;
		offset.setIdentity();
		float weight = sqrt(dx1*dx1 + dy1*dy1) * window.getSize().y / fnear;// / 2.0f;
		float t = rnd.getF32();
		weight = (1.0f - weight) * 3.0f / 3.1416f * 2 * (1.0f - 2 * abs(.5f - t));
		offset.setCol(3, Vec4f(dx + dx1, dy + dy1, .0f, 1.0f) + camvel*t);
		Mat4f temp_C;

		temp_C = offset * cam.getWorldToCamera();
		world_to_clip = P * temp_C;

		Mat4f modelToWorld = current_transformation_;
		modelToWorld.m00 *= model_scale;
		modelToWorld.m11 *= model_scale;
		modelToWorld.m22 *= model_scale;

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);

		frameID++;

		framebuffer["accumulate"].bind();
		glDisable(GL_DEPTH_TEST);

		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		shader["postprocess"].use();
		glViewport(0, 0, lastsize.x, lastsize.y);
		auto size = envimap.getSize();

		glBindTexture(GL_TEXTURE_2D, imageTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imageTexture);

		/*glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, envimap.getGLTexture());*/

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


		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_BLEND);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLContext::checkErrors();
		glViewport(0, 0, lastsize.x, lastsize.y);
		imageProcessor.processImage(image_data, size);
		GLContext::checkErrors();

		GLContext::checkErrors();
		/*if (imageProcessor.blit_texture_back && std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - lastswap).count() > 260)
		{
			shading_mode = !shading_mode;
			lastswap = timer.now();
		}*/
		//--------------------------------------------	POSTPROCESS
		//glDrawBuffer(GL_COLOR_ATTACHMENT0);
		shader["postprocess"].use();

		glViewport(0, 0, lastsize.x * 2, lastsize.y * 2);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//if (!process_png)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}


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
