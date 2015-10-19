#pragma once

#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "3d/Texture.hpp"
#include "base/Random.hpp"
#include "3d/CameraControls.hpp"
#include "RayTracer.hpp"
#include "Light.hpp"
#include "ImageProcessor.hpp"

#include "Mesh.hpp"
#include "Shader.hpp"
#include "Framebuffer.hpp"

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstddef>

namespace FW
{

	struct glGeneratedIndices
	{
		GLuint static_vao;
		GLuint static_vertex_buffer;
		GLuint model_to_world_uniform, normal_transform_uniform, world_to_clip_uniform;
		GLuint time_uniform, shading_toggle_uniform, cam_pos;
		GLuint shader_program, postprocess_program, accumulate_program;
		GLuint envimap_sampler, postprocess_sampler, accumulate_sampler, weight_sampler;
	};
	class Renderer
	{

	public:
							Renderer();		// constructor
		virtual				~Renderer() {}	// destructor

		ImageProcessor imageProcessor;
		std::vector<RayTracer>rt;
		std::vector<Light>	primarylights;
		Light				templight;

		Mat4f				last_transformation;
		Mat4f				current_transformation_;
		Mat3f				rotation;
		Mat3f				rotation_moving;

		std::map<std::string, Shader> shader;
		std::map<std::string, Framebuffer> framebuffer;
		std::vector<Vec3f>	lightPoses;
		std::vector<Mesh>	mesh;
		std::vector<Vec3f> lightpoints;
		std::vector<Vec3u8> image_data;
		std::chrono::high_resolution_clock timer;
		std::chrono::high_resolution_clock::time_point lastswap;
		Vec2f				projectionoffset = 0;
		float				framenum = .0;
		int					frameID = 0;

		int					path_depth = 3;
		int					samples_per_draw = 2;
		int					motionblur_shutter_time = 1;
		int					frames_since_shutter = 0;
		int					lightsamples = 10;
		Vec3f				ambient_light = 0;

		float				model_rotation = .0f;
		float				model_velocity = .0f;
		float				model_scale = 1.0f;
		float				fov = 60.0f;
		float				focal_dist = .4f;
		float				aperture_size = 0.0005f;
		Texture				envimap;
		bool				bloom_toggle = false;
		bool				clear_screen = false;
		bool				reload_shaders = false;
		bool				shading_mode = true;
		float				camera_rotation_angle_ = .0f;
		bool				sprinkler_makes_lights = true;
		bool				renderer_accumulate_indefinitely = false;
		bool				process_png = false;
		bool				render_greyscale = false;
		bool				moviemode = false;
		bool				show_postprocessing = true;
		Vec3f				hitposition = 0;


		void				initRendering(glGeneratedIndices& gl, Window& window, CameraControls& cam);
		void				render(glGeneratedIndices& gl, Window& window, CameraControls& cam, bool drawscreen, float shuttertime, float timestep);

		void				sampleLightTri();
		void				renderVector(std::vector<Vec3f>& vec, GLenum mode, Mat4f world_to_clip);
		void				renderVectorColored(std::vector<Vec3f>& vec, GLenum mode, Mat4f world_to_clip);


	private:
							Renderer(const Renderer&);		// forbid copy
		Renderer&			operator=(const Renderer&);	// forbid assignment
		void				sendUniform(GLuint program, const char* name, const float value);
		void				sendUniformVec2(GLuint program, const char* name, const Vec2f value);
		void				sendUniformVec3(GLuint program, const char* name, const Vec3f value);
		void				sendUniformVec4(GLuint program, const char* name, const Vec4f value);
		void				reloadshaders(Window& window);


		Vec2i				lastsize = 0;
		GLuint				canvas;
		GLuint				postprocesstexture;
		GLuint				bloomtex1, bloomtex2, bloomtex3, bloomtex12, bloomtex22, bloomtex32;
		GLuint				weighttexture;
		GLuint				modelfbo;
		GLuint				postprocessfbo;
		GLuint				depthtexture;
		GLuint				fullscreenVBO_uv, fullscreenVAO_uv;
		GLuint				pointVBO;
		GLuint				viewtexture;
		GLuint				imageTexture;
	};
}