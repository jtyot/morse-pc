#pragma once

#include <base/Math.hpp>
#include <vector>
#include <chrono>

namespace FW
{
	struct Renderer;

	struct blinker //class that describes a detected light, whether it's a sender we're following or just a streetlight
	{
		Vec2f pos; //position of light on screen
		int size;
		float brightness;
		float mass = 1;
		bool isDead = false;
		std::chrono::high_resolution_clock::time_point lastseen; //time point when we last saw this light

		blinker(const Vec2f pos, const std::chrono::high_resolution_clock& timer, int size, float brightness) : pos(pos), lastseen(timer.now()), size(size), brightness(brightness) {}

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
			return pos;
		}

		Vec2f getCurPos(const std::chrono::high_resolution_clock& timer) const //gets 2nd-order approximation of current position
		{
			float t = time(timer); //time elapsed since last sighting, in seconds
			return getCurPos(t);
		}

		void updatePos(const Vec2f new_pos, const std::chrono::high_resolution_clock& timer, int new_size, float new_brightness)
		{
			brightness = new_brightness;
			size = new_size;
			pos = (new_pos + pos * mass) / (mass+1.0f);
			mass++;
			lastseen = timer.now();
		}

		bool tryUpdate(const Vec2f new_pos, const std::chrono::high_resolution_clock& timer, int new_size, float new_brightness)
		{
			if ((new_pos - getCurPos(timer)).length() < .15f)
			{
				updatePos(new_pos, timer, new_size, new_brightness);
				return true;
			}
			return false;
		}
	};

	class ImageProcessor
	{

	public:
		void				processImage(const std::vector<Vec4f>& imageData, const Vec2i& size);
		int					expandPixel(int i, float threshold, std::vector<float>& histogram, std::vector<bool>& visited, int orig_i, int maxdist);

		bool				blit_texture_back = true;
		bool				stabilize_image = true;
		bool				show_searchpoints = false;
		bool				show_histograms = true;
		bool				show_unseenblinkers = true;

		int					smoothing_kernel_size = 1;
		int					downscalefactor_slider = 5;
		int					pointsearch_min = 2;
		int					pointsearch_max = 5;

		float				lightsearch_threshold = .1f;
		float				pointsearch_threshold = .1f;
		float				light_min_distance = .15f;
		std::vector<float>  sumImage;

		ImageProcessor(Renderer* renderer) : renderer(renderer){}
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