#pragma once

#include <base/Math.hpp>
#include <base/Random.hpp>
#include <vector>
#include <chrono>

namespace FW
{
	struct Renderer;

	struct blinker //class that describes a detected light, whether it's a sender we're following or just a streetlight
	{
		Vec2f pos; //position of light on screen
		Vec2f vel, acc;
		Vec2f prev_pos, prev_vel;
		static int blinkerID;
		float size;
		int ID;
		float brightness;
		float dt = 1e9;
		float mass = 1;
		bool isDead = false;
		bool firstUpdate = true;
		bool remove = false;
		std::chrono::high_resolution_clock::time_point lastseen; //time point when we last saw this light

		blinker(const Vec2f pos, const std::chrono::high_resolution_clock& timer, int size, float brightness) : pos(pos), lastseen(timer.now()), size(size), brightness(brightness), ID(blinkerID++) {}

		long long time(const std::chrono::high_resolution_clock& timer) const //get time elapsed since last sighting, in milliseconds
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - lastseen).count() + 1;
		}

		bool dead(const std::chrono::high_resolution_clock& timer) const //is it too long since we last saw this?
		{
			auto dur = time(timer);
			return dur > 2000; //dead if not seen in last .1 seconds
		}

		bool on(const std::chrono::high_resolution_clock& timer) const //was this seen just recently?
		{
			return time(timer) < 60; //on if last seen under n milliseconds ago
		}

		Vec2f getCurPos(float t) const
		{
			return pos;// +vel * t;// +.5f * acc * t * t;
		}

		Vec2f getCurPos(const std::chrono::high_resolution_clock& timer) const //gets 2nd-order approximation of current position
		{
			float t = time(timer); //time elapsed since last sighting, in seconds
			return getCurPos(t);
		}

		void mergePos(const Vec2f new_pos, int new_size, float new_brightness, float other_mass = 1)
		{
			brightness = (new_brightness * other_mass + brightness * mass) / (mass + other_mass);
			size = (new_size * other_mass + size * mass) / (mass + other_mass);
			pos = (new_pos * other_mass + pos * mass) / (mass + other_mass);
			mass++;
			isDead = false;
		}

		void update(const std::chrono::high_resolution_clock& timer)
		{
			if (isDead)
				return;

			if (firstUpdate)
			{
				prev_pos = pos;
				prev_vel = vel;
			}

			dt = time(timer);
			lastseen = timer.now();
			auto new_vel = pos - prev_pos;
			vel = new_vel / dt;
			auto new_acc = vel - prev_vel;
			acc = new_acc / dt;
			prev_pos = pos;
			prev_vel = vel;

			firstUpdate = false;
		}

		bool operator==(const blinker& other)
		{
			return other.ID == ID;
		}
	};

	class ImageProcessor
	{

	public:
		std::vector<Vec3f> blinkerColors;
		Random				rnd;
		void				processImage(const std::vector<Vec4f>& imageData, const Vec2i& size);
		int					expandPixel(int i, float threshold, std::vector<float>& histogram, std::vector<bool>& visited, int orig_i, int maxdist);

		bool				blit_texture_back = true;
		bool				stabilize_image = true;
		bool				show_searchpoints = false;
		bool				show_histograms = true;
		bool				show_unseenblinkers = true;

		int					smoothing_kernel_size = 1;
		int					downscalefactor_slider = 2;
		int					pointsearch_min = 1;
		int					pointsearch_max = 4;

		float				lightsearch_threshold = .06f;
		float				pointsearch_threshold = .1f;
		float				light_min_distance = .011f;
		float				shadowCutOff = .2f;
		std::vector<float>  sumImage;

		ImageProcessor(Renderer* renderer) : renderer(renderer){ 
			rnd = Random(1234); 
			for (int i = 0; i < 90; ++i)
				blinkerColors.push_back(rnd.getVec3f(0, 1));
		}
		~ImageProcessor(void){}
		
	private:
		ImageProcessor(const ImageProcessor&); //forbid copy
		ImageProcessor operator=(const ImageProcessor&); //forbid assignment

	private:
		std::vector<float>  prevHistoX;
		std::vector<float>  prevHistoY;
		std::vector<float>	histoX;
		std::vector<float>	histoY;
		std::vector<blinker> blinkers;

		Vec2i				lastsize = 0;
		Vec2i				lastoffset = 0;
		int					lastdownscalefactor = 0;

		Renderer*			renderer;
		std::chrono::high_resolution_clock timer;
	};

}