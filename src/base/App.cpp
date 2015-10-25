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


App::App(void)
: common_ctrl_(CommonControls::Feature_Default & ~CommonControls::Feature_RepaintOnF5),
model_changed_(true),
shading_mode_changed_(false)
{
	renderer.initRendering(window_);
	renderer.current_transformation_.setIdentity();


	common_ctrl_.showFPS(true);
	common_ctrl_.addButton(&model_changed_, FW_KEY_M, "Load scene (M)");
	common_ctrl_.addToggle(&moviemode, FW_KEY_P, "Toggle image sequence playback (P)");
	common_ctrl_.addToggle(&renderer.render_greyscale, FW_KEY_B, "Toggle render only blue channel (B)");
	common_ctrl_.addToggle(&renderer.show_postprocessing, FW_KEY_NONE, "Toggle postprocessing visibility");
	common_ctrl_.addSeparator();
	common_ctrl_.addToggle(&renderer.imageProcessor.stabilize_image, FW_KEY_U, "Toggle image stabilization (U)");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_histograms, FW_KEY_NONE, "Toggle histogram display");
	common_ctrl_.addToggle(&renderer.imageProcessor.show_unseenblinkers, FW_KEY_NONE, "Toggle unseen blinker visibility");
	common_ctrl_.addSeparator();


	common_ctrl_.showFPS(true);

	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.downscalefactor_slider, 1, 60, false, FW_KEY_NONE, FW_KEY_NONE, "Downscale factor: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_max, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "point search count: %d");
	common_ctrl_.addSlider((S32*)&renderer.imageProcessor.pointsearch_min, 1, 15, false, FW_KEY_NONE, FW_KEY_NONE, "point search scale: %d");
	common_ctrl_.endSliderStack();


	common_ctrl_.beginSliderStack();
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.shadowCutOff, .0f, .99f, false, FW_KEY_NONE, FW_KEY_NONE, "light search shadow cutoff threshold: %.4f");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.lightsearch_threshold, .01f, 10.f, true, FW_KEY_NONE, FW_KEY_NONE, "light search local max threshold: %.4f");
	common_ctrl_.addSlider((F32*)&renderer.imageProcessor.light_min_distance, .0f, .1f, false, FW_KEY_NONE, FW_KEY_NONE, "lightpoint min distance: %.4f");
	common_ctrl_.endSliderStack();


	window_.setTitle("Assignment 0");

	window_.addListener(this);
	window_.addListener(&common_ctrl_);

	window_.setSize(Vec2i(1600, 900));
}

void App::postProcessImage()
{
	auto size = renderer.envimap.getSize();
	auto img = renderer.envimap.getImage();
	renderer.image_data.resize(size.x * size.y);
	auto data = img->getPtr();
	for (int x = 0; x < size.x; ++x)
	{
		for (int y = 0; y < size.y; ++y)
		{
			renderer.image_data[y * size.x + x] = Vec3u8(data[(y * size.x + x) * 3 + 0], data[(y * size.x + x) * 3 + 1], data[(y * size.x + x) * 3 + 2]);
		}
	}
}

bool App::handleEvent(const Window::Event& ev) {

	if (model_changed_)	{
		model_changed_ = false;
		bool changed = false;
		renderer.process_png = false;
		auto filename = window_.showFileLoadDialog("Load new scene");

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

			imageSequence.clear();
			auto number = filename.substring(filename.getLength() - FW::String("0000.png").getLength(), filename.getLength() - FW::String(".png").getLength());
			imageSequence[number] = renderer.image_data;
		}
		
	}

	if (shading_mode_changed_) {
		common_ctrl_.message(shading_toggle_ ?
			"Particles as lines and points" :
			"Mesh generated from particles");
		shading_mode_changed_ = false;
	}



	if (ev.type == Window::EventType_KeyDown) {

	}

	if (ev.type == Window::EventType_KeyUp) {

	}
	if (ev.type == Window::EventType_Mouse) {

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

		renderer.render(window_);

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
			auto numstr = FW::String(number);
			if (imageSequence[numstr].size() == 0)
			{
				renderer.envimap = Texture::import(name);
				clearError();
				if (!renderer.envimap.exists())
				{
					renderer.envimap = Texture::import(image_sequence_name + "0000.png");
					current_sequence_frame = 0;
				}
				postProcessImage();
				imageSequence[numstr] = renderer.image_data;
			}
			else
				renderer.image_data = imageSequence[numstr];
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
