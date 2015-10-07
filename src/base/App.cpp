#define _CRT_SECURE_NO_WARNINGS

#include "App.hpp"

//#include "utility.hpp"
#include "base/Main.hpp"
#include "gpu/GLContext.hpp"
#include "gpu/Buffer.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace FW;
using namespace std;


void loadscene(const std::string filename, std::vector<RayTracer>& rts, std::vector<Mesh>& meshes)
{
	rts.clear();
	meshes.clear();
	std::string file_temp = filename;
	for (auto& c : file_temp)
		if (c == '\\')
			c = ' ';
	std::istringstream        iss(file_temp);
	std::string s;
	std::string filepath;
	while (true)
	{
		iss >> s;
		if (s == "" || s.find(".txt") != std::string::npos)
			break;
		else
		{
			filepath.append(s);
			filepath.append("\\");
		}
	}

	std::ifstream input(filename, std::ios::in);

	std::string line;

	std::cout << "loading scene " << filename << std::endl;

	//vector<Mesh> meshes;
	bool foundobj = false;
	Mesh* cur_mesh = nullptr;
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
		std::string              s;

		// Create a stream from the string to pick out one value at a time.
		std::istringstream        iss(line);

		// Read the first token from the line into string 's'.
		// It identifies the type of object (vertex or normal or ...)
		iss >> s;

		if (s == "#")
			continue;
		if (s == "pos") { // vertex position
			iss >> v.x >> v.y >> v.z;
			cur_mesh->transformation.setCol(3, Vec4f(v, 1.0f));
			cur_mesh->transformation_previous = cur_mesh->transformation;
		}
		if (s == "vel") {
			iss >> v.x >> v.y >> v.z;
			cur_mesh->vel = v;
		}
		if (s == "vel_rot") {
			iss >> v.x >> v.y >> v.z;
			cur_mesh->rotation_vel = v;
		}
		if (s == "scale") {
			iss >> v.x >> v.y >> v.z;
			cur_mesh->transformation *= Mat4f::scale(Vec4f(v, 1.0f));
			cur_mesh->scale = v.x;
			cur_mesh->transformation_previous = cur_mesh->transformation;
		}
		if (s == "orbit") {
			iss >> v.x >> v.y >> v.z;
			cur_mesh->orbitpoint = v;
			cur_mesh->orbitspoint = true;
			cur_mesh->orbitoffset = Vec4f(cur_mesh->transformation.getCol(3)).getXYZ() - v;
		}
		if (s == "orbit_speed") {
			iss >> v.x;
			cur_mesh->orbitspeed = v.x;
		}
		else if (s == "object") { // load new object
			meshes.push_back(Mesh());
			rts.push_back(RayTracer());
			cur_mesh = &meshes[meshes.size() - 1];
			iss >> s;
			std::string str = filepath;
			str.append(s);
			cur_mesh->load(str);
		}
	}
	//return meshes;
}

// Anonymous namespace. This is a C++ way to define things which
// do not need to be visible outside this file.

float rot_velocity = 1.0f;
float model_velocity = 1.0f;
Vec3f rot_axis = Vec3f(0, 1, 0);
Random rnd;

App::App(void)
: common_ctrl_(CommonControls::Feature_Default & ~CommonControls::Feature_RepaintOnF5),
current_model_(MODEL_NONE),
model_changed_(true),
cameraCtrl(&common_ctrl_, CameraControls::Feature_Default | CameraControls::Feature_StereoControls),
shading_mode_changed_(false),
step_(0.0001f),
steps_per_update_(5)
{
	//static_assert(is_standard_layout<Vertex>::value, "struct Vertex must be standard layout to use offsetof");
	renderer.initRendering(gl_, window_, cameraCtrl);
	renderer.current_transformation_.setIdentity();
	renderer.mesh.push_back(Mesh());
	renderer.mesh.push_back(Mesh());
	renderer.mesh[1].load("");
	renderer.mesh[1].materialMap[""].diffuseTexture = Texture::import("sponza_curtain_blue_diff.png");
	renderer.mesh[1].materialMap[""].hasDiffuseTexture = true;
	renderer.mesh[1].materialMap[""].specularTexture = Texture::import("sponza_curtain_blue_diff.png");
	renderer.mesh[1].materialMap[""].hasSpecularTexture = true;
	renderer.mesh[1].materialMap[""].normalTexture = Texture::import("normalsponza_curtain_blue_diff.png");
	renderer.mesh[1].materialMap[""].hasNormalTexture = true;


	common_ctrl_.showFPS(true);
	common_ctrl_.addButton(&model_changed_, FW_KEY_M, "Load scene (M)");
	common_ctrl_.addSeparator();
	common_ctrl_.addToggle(&renderer.shading_mode, FW_KEY_T, "Toggle shading mode (T)", &shading_mode_changed_);
	common_ctrl_.addToggle(&renderer.imageProcessor.blit_texture_back, FW_KEY_NONE, "Toggle point blinking (Y)");
	common_ctrl_.addSeparator();
	common_ctrl_.addToggle(&renderer.imageProcessor.stabilize_image, FW_KEY_NONE, "Toggle image stabilization");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_histograms, FW_KEY_NONE, "Toggle histogram display");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_searchpoints, FW_KEY_NONE, "Toggle search point display");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_unseenblinkers, FW_KEY_NONE, "Toggle unseen blinker visibility");
	common_ctrl_.addSeparator();


	common_ctrl_.showFPS(true);

	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((S32*)&renderer.samples_per_draw, 1, 100, false, FW_KEY_NONE, FW_KEY_NONE, "Render samples: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.downscalefactor_slider, 1, 60, false, FW_KEY_NONE, FW_KEY_NONE, "Downscale factor: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_max, 3, 20, false, FW_KEY_NONE, FW_KEY_NONE, "point search outer size: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_min, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "point search inner size: %d");
	common_ctrl_.endSliderStack();


	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.pointsearch_threshold, .0001f, 1.3f, true, FW_KEY_NONE, FW_KEY_NONE, "point search local max threshold: %.4f");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.lightsearch_threshold, .001f, 1.3f, true, FW_KEY_NONE, FW_KEY_NONE, "light search local max threshold: %.4f");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.smoothing_kernel_size, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "preprocess smoothing size: %d");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.light_min_distance, .0f, .3f, false, FW_KEY_NONE, FW_KEY_NONE, "lightpoint min distance: %.4f");
	common_ctrl_.endSliderStack();

	window_.addListener(&cameraCtrl);
	cameraCtrl.setKeepAligned(true);
	cameraCtrl.setPosition(0);
	cameraCtrl.setFar(20.0f);

	window_.setTitle("Assignment 0");

	window_.addListener(this);
	window_.addListener(&common_ctrl_);

	window_.setSize(Vec2i(1600, 900));

	//renderer.envimap = Texture::import("assets/a.png");
	Vec2i asd = renderer.envimap.getImage()->getSize();
	//Vec4f asd = envimap.getImage()->getVec4f(Vec2i(100, 100));
	cout << asd.x << ", " << asd.y << endl;
}

bool App::handleEvent(const Window::Event& ev) {

	if (model_changed_)	{
		model_changed_ = false;
		bool changed = false;
		renderer.process_png = false;
		auto filename = window_.showFileLoadDialog("Load new scene");

		if (filename.endsWith(".txt")) //if scene file
		{
			loadscene(filename.getPtr(), renderer.rt, renderer.mesh);
			changed = true;
		}
		else if (filename.endsWith(".obj") || filename.endsWith(".OBJ")){ //if mesh file
			renderer.mesh[0].load(filename.getPtr());
			renderer.rt.clear();
			renderer.rt.push_back(RayTracer());
			changed = true;
		}
		if (filename.endsWith(".png")) //if picture
		{
			renderer.envimap = Texture::import(filename);
			renderer.process_png = true;
			auto size = renderer.envimap.getSize();
			auto img = renderer.envimap.getImage();
			renderer.image_data.resize(size.x * size.y * sizeof(Vec4f));
			for (int x = 0; x < size.x; ++x)
			{
				for (int y = 0; y < size.y; ++y)
				{
					auto vec = img->getVec4f(Vec2i(x, y));
					//for (int i = 0; i < 4; ++i)
					//	vec[i] = pow(vec[i], 1.0f / 2.2f);
					/*for (int i = 0; i < 4; ++i)
						vec[i] = pow(vec[i], 4.2f) * 5.0f;*/
					vec.w = 1.0f;
					renderer.image_data[y * size.x + x] = vec;
				}
			}
		}
		
		// Load the vertex buffer to GPU.
		if (changed)
		{
			renderer.clear_screen = true;
			cout << "loading BVH" << endl;

			//build raytracers for each mesh
			int i = 0;
			for (auto& rt : renderer.rt){
				rt.constructHierarchy(renderer.mesh[i].triangles);
				++i;
			}

			cout << "BVH loaded" << endl;
		}
	}

	if (shading_mode_changed_) {
		common_ctrl_.message(shading_toggle_ ?
			"Particles as lines and points" :
			"Mesh generated from particles");
		shading_mode_changed_ = false;
	}



	if (ev.type == Window::EventType_KeyDown) {
		if (ev.key == FW_KEY_CONTROL)
		{
			float ang = tan(.5f * renderer.fov * FW_PI / 180.0f);
			Vec2f screen = window_.getSize();
			Vec3f start = cameraCtrl.getPosition();
			Vec3f dir = (cameraCtrl.getCameraToWorld() * Vec4f(mousepos * ang * Vec2f(screen.x / screen.y, 1.0f), -1.0f, .0f)).getXYZ();

			/*RaycastResult r1 = renderer.rt.raycast(start, dir * 99.0f);
			RaycastResult r2 = renderer.cloth_rt.raycast(start, dir * 99.0f);
			if (r2.t < r1.t)
				r1 = r2;*/
			/*RaycastResult r1 = castRay(start, start + dir, renderer.mesh, renderer.rt);
			if (r1.t < 1)
			{
				renderer.focal_dist = (r1.point - start).length();
				renderer.hitposition = r1.point;
			}*/
		}
		if (ev.key == FW_KEY_SPACE)
		{
			renderer.primarylights[0].position = cameraCtrl.getPosition();
			renderer.primarylights[0].camera.setForward(cameraCtrl.getForward());
			cameraCtrl.getPosition().print();
		}
	}

	if (ev.type == Window::EventType_KeyUp) {
		if (ev.key == FW_KEY_F5)
		{
			renderer.reload_shaders = true;
		}
		if (ev.key == FW_KEY_ALT)
		{
			grabbing = false;
		}
	}
	if (ev.type == Window::EventType_Mouse) {
		if (Vec2f(ev.mouseDelta).length())
		{
			Vec2f lastpos = mousepos;
			mousepos = (Vec2f(ev.mousePos) - Vec2f(window_.getSize()) * .5f) * 2.0f / Vec2f(window_.getSize());
			mousepos.y *= -1;
		}
	}


	if (ev.type == Window::EventType_Close) {
		window_.showModalMessage("Exiting...");
		delete this;
		return true;
	}

	window_.setVisible(true);
	if (ev.type == Window::EventType_Paint)
	{

		/*if (framecount > shutter_duration)
		{
		renderer.clear_screen = true;
		framecount = 0;
		}*/
		if (enable_animation)
		{
			common_ctrl_.m_screenshot = true;
			common_ctrl_.screenshotname_animation = true;
		}
		else
		{
			common_ctrl_.screenshotname_animation = false;
		}
		renderer.clear_screen = true;
		renderer.motionblur_shutter_time = shutter_duration;
		for (int j = 0; j < shutter_duration; j++)
		{
			float t = rnd.getF32(-1,1);
			//renderer.model_rotation = rot_velocity * t;
			
			/*Mat3f rot = Mat3f::rotation(rot_axis, rot_velocity * t);
			renderer.current_transformation_.setCol(0, Vec4f(rot.getCol(0), .0f));
			renderer.current_transformation_.setCol(1, Vec4f(rot.getCol(1), .0f));
			renderer.current_transformation_.setCol(2, Vec4f(rot.getCol(2), .0f));*/
			renderer.model_rotation = rot_velocity;
			renderer.model_velocity = model_velocity;
			renderer.render(gl_, window_, cameraCtrl, j + 1 == shutter_duration, shutter_duration, step_ * steps_per_update_);
			if (!renderer.renderer_accumulate_indefinitely)
			for (int i = 0; i < steps_per_update_; ++i) 
			{
				/*ps_->wind_randomness = wind_randomness;
				ps2_->wind_randomness = wind_randomness;
				ps_->wind_strength = wind_strength;
				ps2_->wind_strength = wind_strength;
				switch (integrator_) {
				case EULER_INTEGRATOR:
					integrator::eulerStep(*ps_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, true);
					if (ps_type_ == SPRINKLER_AND_CLOTH)
						integrator::eulerStep(*ps2_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, false);
					break;
				case TRAPEZOID_INTEGRATOR:
					integrator::trapezoidStep(*ps_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, true);
					if (ps_type_ == SPRINKLER_AND_CLOTH)
						integrator::trapezoidStep(*ps2_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, false);
					break;
				case MIDPOINT_INTEGRATOR:
					integrator::midpointStep(*ps_, *ps_, step_, enable_collisions, &renderer.rt, enable_particle_collision_vectors, enable_particle_vectors, true);
					if (ps_type_ == SPRINKLER_AND_CLOTH)
						integrator::midpointStep(*ps2_, *ps_, step_, enable_collisions, &renderer.rt, enable_particle_collision_vectors, enable_particle_vectors, false);
					break;
				case RK4_INTEGRATOR:
					integrator::rk4Step(*ps_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, true);
					if (ps_type_ == SPRINKLER_AND_CLOTH)
						integrator::rk4Step(*ps2_, *ps_, step_, enable_collisions, &renderer.rt, &renderer.cloth_rt, enable_particle_collision_vectors, enable_particle_vectors, false);
					break;
				default:
					assert(false && " invalid integrator type");
				}*/
				if ((renderer.shading_mode || enable_particle_cloth_collisions) && (ps_type_ == SPRINKLER_AND_CLOTH || ps_type_ == CLOTH_SYSTEM))
				{
					glBindBuffer(GL_ARRAY_BUFFER, renderer.mesh[1].VBO);
					glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* renderer.mesh[1].vertices.size(), renderer.mesh[1].vertices.data(), GL_DYNAMIC_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					/*if (enable_particle_cloth_collisions && (ps_type_ == SPRINKLER_AND_CLOTH || ps_type_ == CLOTH_SYSTEM))
					{
						if (framecount % 100 == 0)
						{
							renderer.cloth_rt.constructHierarchy(renderer.mesh[1].triangles);
							framecount = 0;
						}
						else
							renderer.cloth_rt.updateAABBs(renderer.mesh[1].triangles);
					}*/
				}
				framecount++;
			}
		}
		common_ctrl_.message(sprintf("Use Home/End to rotate camera."), "instructions");
		common_ctrl_.message(sprintf("%.0f supersamples", renderer.framenum), "camerainfo");
	}

	window_.repaint();

	return false;
}



void FW::init(void) {
	new App;
}
