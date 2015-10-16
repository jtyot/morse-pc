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
private:
	enum ParticleSystemType {
		NONE,
		SIMPLE_SYSTEM,
		SPRING_SYSTEM,
		PENDULUM_SYSTEM,
		CLOTH_SYSTEM,
		SPRINKLER_SYSTEM,
		SPRINKLER_AND_CLOTH,
		GPU_SPRINKLER
	};
	enum IntegratorType {
		EULER_INTEGRATOR,
		TRAPEZOID_INTEGRATOR,
		MIDPOINT_INTEGRATOR,
		RK4_INTEGRATOR
	};
	enum CurrentModel
	{
		MODEL_NONE,
		MODEL_FROM_FILE,
		SCENE_FILE
	};

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
	CameraControls		cameraCtrl;

	CurrentModel		current_model_;
	bool				model_changed_;
	bool				shading_toggle_;
	bool				shading_mode_changed_;
	bool				system_changed_;
	bool				mousedown = false;
	bool				mousepressed = false;
	bool				firstrender = true;
	bool				moviemode = false;
	int					current_sequence_frame = 0;
	FW::String				image_sequence_name = "";

	bool				enable_collisions = true;
	bool				enable_particle_vectors = false;
	bool				enable_particle_collision_vectors = false;
	bool				enable_particle_cloth_collisions = true;
	bool				sprinkler_emits_light = true;

	Vec3i				grabbed_particles = -1;
	bool				grabbing = false;

	Renderer			renderer;
	Vec2f				mousepos;
	Vec2f				mousepos_last;
	Timer				timer_;

	float				wind_strength;
	float				wind_randomness;

	ParticleSystemType	ps_type_;
	IntegratorType		integrator_;

	float			step_;
	int				steps_per_update_;
	int				shutter_duration = 1;
	int				framecount = 0;
	bool			enable_animation = false;

	glGeneratedIndices	gl_;


	// YOUR CODE HERE (R1)
	// Add a class member to store the current translation.

	// EXTRA:
	// For animation extra credit you can use the framework's Timer class.
	// The .start() and .unstart() methods start and stop the timer; when it's
	// running, .end() gives you seconds passed after last call to .end().
	// Timer timer_;
};

}
