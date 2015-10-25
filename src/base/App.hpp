#pragma once

#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "3d/Texture.hpp"
#include "base/Random.hpp"
#include "Renderer.hpp"
#include "3d/CameraControls.hpp"
//#include "particle_systems.hpp"
//#include "integrators.hpp"


#include <string>
#include <vector>


namespace FW {



class App : public Window::Listener
{

public:
						App();		// constructor
	virtual				~App() {}	// destructor

	virtual bool		handleEvent(const Window::Event& ev);

private:
						App(const App&);		// forbid copy
	App&				operator=(const App&);	// forbid assignment

	void				postProcessImage();

	Window				window_;
	CommonControls		common_ctrl_;

	bool				model_changed_;
	bool				shading_toggle_;
	bool				shading_mode_changed_;
	bool				firstrender = true;
	bool				moviemode = false;
	int					current_sequence_frame = 0;
	FW::String				image_sequence_name = "";



	Renderer			renderer;
	Timer				timer_;


	int				framecount = 0;
	bool			enable_animation = false;

	std::map<FW::String, std::vector<Vec3u8>> imageSequence;


};

}
