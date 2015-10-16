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
	renderer.initRendering(gl_, window_, cameraCtrl);
	renderer.current_transformation_.setIdentity();
	renderer.mesh.push_back(Mesh());
	renderer.mesh.push_back(Mesh());


	common_ctrl_.showFPS(true);
	common_ctrl_.addButton(&model_changed_, FW_KEY_M, "Load scene (M)");
	common_ctrl_.addToggle(&moviemode, FW_KEY_P, "Toggle image sequence playback (P)");
	common_ctrl_.addSeparator();
	common_ctrl_.addToggle(&renderer.shading_mode, FW_KEY_T, "Toggle shading mode (T)", &shading_mode_changed_);
	common_ctrl_.addToggle(&renderer.imageProcessor.blit_texture_back, FW_KEY_Y, "Toggle point blinking (Y)");
	common_ctrl_.addToggle(&renderer.render_greyscale, FW_KEY_B, "Toggle render only blue channel (B)");
	common_ctrl_.addToggle(&renderer.show_postprocessing, FW_KEY_NONE, "Toggle postprocessing visibility");
	common_ctrl_.addSeparator();
	common_ctrl_.addToggle(&renderer.imageProcessor.stabilize_image, FW_KEY_U, "Toggle image stabilization (U)");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_histograms, FW_KEY_NONE, "Toggle histogram display");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_searchpoints, FW_KEY_NONE, "Toggle search point display");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_unseenblinkers, FW_KEY_NONE, "Toggle unseen blinker visibility");
	common_ctrl_.addSeparator();


	common_ctrl_.showFPS(true);

	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((S32*)&renderer.samples_per_draw, 1, 100, false, FW_KEY_NONE, FW_KEY_NONE, "Render samples: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.downscalefactor_slider, 1, 60, false, FW_KEY_NONE, FW_KEY_NONE, "Downscale factor: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_max, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "point search count: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_min, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "point search scale: %d");
	common_ctrl_.endSliderStack();


	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.shadowCutOff, .0f, .99f, false, FW_KEY_NONE, FW_KEY_NONE, "light search shadow cutoff threshold: %.4f");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.lightsearch_threshold, .01f, 10.f, true, FW_KEY_NONE, FW_KEY_NONE, "light search local max threshold: %.4f");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.light_min_distance, .0f, .1f, false, FW_KEY_NONE, FW_KEY_NONE, "lightpoint min distance: %.4f");
	common_ctrl_.endSliderStack();

	window_.addListener(&cameraCtrl);
	cameraCtrl.setKeepAligned(true);
	cameraCtrl.setPosition(0);
	cameraCtrl.setFar(20.0f);

	window_.setTitle("Assignment 0");

	window_.addListener(this);
	window_.addListener(&common_ctrl_);

	window_.setSize(Vec2i(1600, 900));
}

void App::postProcessImage()
{
	auto size = renderer.envimap.getSize();
	auto img = renderer.envimap.getImage();
	renderer.image_data.resize(size.x * size.y * sizeof(Vec4f));
	for (int x = 0; x < size.x; ++x)
	{
		for (int y = 0; y < size.y; ++y)
		{
			auto vec = img->getVec4f(Vec2i(x, y));
			vec.w = 1.0f;
			renderer.image_data[y * size.x + x] = vec;
		}
	}
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
			if (filename.endsWith("0000.png"))
			{
				image_sequence_name = filename.substring(0, filename.getLength() - FW::String("0000.png").getLength());
				cout << "image sequence name: " << image_sequence_name.getPtr() << "****.png" << endl;
				moviemode = true;
				current_sequence_frame = 0;
			}
			postProcessImage();
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
		renderer.moviemode = moviemode;
		renderer.motionblur_shutter_time = shutter_duration;
		for (int j = 0; j < shutter_duration; j++)
		{
			float t = rnd.getF32(-1,1);
			renderer.model_rotation = rot_velocity;
			renderer.model_velocity = model_velocity;
			renderer.render(gl_, window_, cameraCtrl, j + 1 == shutter_duration, shutter_duration, step_ * steps_per_update_);
			if (!renderer.renderer_accumulate_indefinitely)
			for (int i = 0; i < steps_per_update_; ++i) 
			{
				if ((renderer.shading_mode || enable_particle_cloth_collisions) && (ps_type_ == SPRINKLER_AND_CLOTH || ps_type_ == CLOTH_SYSTEM))
				{
					glBindBuffer(GL_ARRAY_BUFFER, renderer.mesh[1].VBO);
					glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* renderer.mesh[1].vertices.size(), renderer.mesh[1].vertices.data(), GL_DYNAMIC_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				framecount++;
			}
		}

		if (moviemode && renderer.process_png)
		{
			current_sequence_frame++;

			//convert frame number to string
			int temp_number = current_sequence_frame;
			int divisor = 1000;
			char number[5];
			for (int i = 3; i >= 0; --i)
			{
				int temp_remnant = temp_number / divisor;
				number[3 - i] = temp_remnant + '0';

				temp_number -= temp_remnant * divisor;
				divisor /= 10;
			}
			number[4] = '\0';
			auto name = image_sequence_name + number + ".png";
			cout << "opening image frame: " << name.getPtr() << endl;
			renderer.envimap = Texture::import(name);
			clearError();
			if (!renderer.envimap.exists())
			{
				renderer.envimap = Texture::import(image_sequence_name + "0000.png");
				current_sequence_frame = 0;
			}
			postProcessImage();
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
