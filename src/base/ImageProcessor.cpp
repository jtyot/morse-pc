#include "ImageProcessor.hpp"
#include "Renderer.hpp"

using namespace FW;
using namespace std;

int ImageProcessor::expandPixel(int i, float threshold, std::vector<float>& histogram, std::vector<bool>& visited, int orig_i, int maxdist)
{
	int count = 1;
	visited[i] = true;

	i++;
	if (i - orig_i < maxdist && histogram[i] > threshold)
		count += expandPixel(i, threshold, histogram, visited, orig_i, maxdist);

	return count;
}

//gets the sum of data in square range (-outer, outer) around point (x,y)
float get_sum_square(const int x, const int y, const Vec4f* data, const Vec2i size, const int outer)
{
	float avebrightness_outer = 0;
	for (int dx = -outer; dx < outer; ++dx)
	{
		for (int dy = -outer; dy < outer; ++dy)
		{
			int x2 = x + dx;
			int y2 = y + dy;
			if (x2 < 0 || x2 >= size.x || y2 < 0 || y2 >= size.y)
				continue;
			avebrightness_outer += data[y2 * size.x + x2].getXYZ().dot(Vec3f(1));
		}
	}
	return avebrightness_outer;
}

//gets the average value of data in square range (-outer, outer) around point (x,y)
float get_average_square(const int x, const int y, const Vec4f* data, const Vec2i size, const int outer)
{
	float avebrightness = get_sum_square(x, y, data, size, outer);
	int count = 4 * outer * outer;
	return avebrightness / count;
}

//gets the average value of data in square range (-outer, outer) / (-inner, inner) around point (x,y)
float get_average_square(const int x, const int y, const Vec4f* data, const Vec2i size, const int outer, const int inner)
{
	float avebrightness_outer = get_sum_square(x, y, data, size, outer);
	int count_outer = 4 * outer * outer;

	float avebrightness_inner = get_sum_square(x, y, data, size, inner);
	int count_inner = 4 * inner * inner;

	return (avebrightness_outer - avebrightness_inner) / (count_outer - count_inner);
}

void ImageProcessor::processImage(const std::vector<Vec4f>& imageData, const Vec2i& size_in)
{
	int downscalefactor = downscalefactor_slider;
	if (size_in != lastsize || lastdownscalefactor != downscalefactor)
	{
		lastsize = size_in;
		lastdownscalefactor = downscalefactor;
		histoX.resize(lastsize.x / downscalefactor + 1);
		histoY.resize(lastsize.y / downscalefactor + 1);
		prevHistoX.resize(lastsize.x / downscalefactor + 1, 0);
		prevHistoY.resize(lastsize.y / downscalefactor + 1, 0);
	}

	Vec2i size = lastsize / downscalefactor;

	//initialize histogram vectors
	memset(histoX.data(), 0, histoX.size() * sizeof(float));
	memset(histoY.data(), 0, histoY.size() * sizeof(float));

	Vec2f maxvalue = .0f; //maximum value of each histogram

	//compute histogram from image rows, loop over all pixels and sum it to the proper vector
	float invsizex = 1.0f / lastsize.x;
	float invsizey = 1.0f / lastsize.y;
	for (int j = 0; j < lastsize.y; j++)
	{
		for (int i = 0; i < lastsize.x; i++)
		{
			auto value = imageData[j * lastsize.x + i].getXYZ().dot(Vec3f(1)); //read only blue channel
			histoX[i / downscalefactor] += value;
			histoY[j / downscalefactor] += value;
		}
	}

	for (int i = 0; i < size.x; ++i)
	{
		histoX[i] *= invsizex;
		maxvalue.x = max(histoX[i], maxvalue.x);
	}

	for (int i = 0; i < size.y; ++i)
	{
		histoY[i] *= invsizey;
		maxvalue.y = max(histoY[i], maxvalue.y);
	}

	std::vector<float>* histograms[2];
	histograms[0] = &histoX;
	histograms[1] = &histoY;

	std::vector<float>* prevHistograms[2];
	prevHistograms[0] = &prevHistoX;
	prevHistograms[1] = &prevHistoY;

	int maxdiffscale = 8;
	Vec2f minError = 1e9;
	Vec2i minOffset = 0;

	if (stabilize_image)
	{
		//compute offset with best histogram fit
		for (int axis = 0; axis < 2; axis++)
		{
			for (int j = -size[axis] / maxdiffscale; j < size[axis] / maxdiffscale; j++)
			{
				auto offset = j + lastoffset[axis];
				auto curError = .0f; //error metric of current offset value
				float count = 0; //number of valid pixels

				for (int x = 0; x < size[axis]; ++x)
				{
					auto i = x + offset;
					if (i < 0 || i >= size[axis] || (*prevHistograms[axis])[i] == 0) //if out of bounds, skip pixel
						continue;

					auto cur = (*histograms[axis])[x];
					auto prev = (*prevHistograms[axis])[i];
					auto weight = 1.0f - abs(float(offset)) / size[axis] * maxdiffscale + .5f;
					curError += pow((cur - prev), 2.0f) * weight;
					count += weight;
				}

				if (count == .0f)
					continue;

				curError /= count;
				curError += pow(float(abs(offset - lastoffset[axis])) / size[axis] * maxdiffscale, 2.0f) * .14f;
				if (curError < minError[axis])
				{
					minError[axis] = curError;
					minOffset[axis] = offset;
				}
			}
		}
	}

	lastoffset = minOffset;
	auto offset = -1.0f * Vec2f(minOffset) / Vec2f(size);
	renderer->projectionoffset = offset;

	std::vector<int> pointsX;
	std::vector<int> pointsY;
	pointsX.reserve(1000);
	pointsY.reserve(1000);

	std::vector<int> points[2];
	points[0] = pointsX;
	points[1] = pointsY;

	for (int axis = 0; axis < 2; axis++)
	{
		const int kernel_start = 1;
		const int kernel_stop = 2;

		for (int j = 0; j < size[axis]; ++j)
		{
			//set prevHisto = histo
			auto i = j - minOffset[axis];
			if (i < 0 || i >= size[axis])
				(*prevHistograms[axis])[j] = 0;
			else
				(*prevHistograms[axis])[j] = (*histograms[axis])[i];

			if (j < kernel_stop || j >= size[axis] - kernel_stop)
				continue;

			float left_max = .0f;
			float right_max = .0f;

			/*for (int offset_j = j - kernel_stop; offset_j < j - kernel_start; ++offset_j)
				left_max = 1.01f * max(histograms[axis][offset_j], left_max);

			for (int offset_j = j + kernel_start; offset_j < j + kernel_stop; ++offset_j)
				right_max = 1.01f * max(histograms[axis][offset_j], right_max);*/
			left_max = pointsearch_threshold + (*histograms[axis])[j - 1];
			right_max = pointsearch_threshold + (*histograms[axis])[j + 1];

			auto val = (*histograms[axis])[j];
			if (val > left_max && val > right_max)
				points[axis].push_back(j);

		}
	}

	////push histogram values to array for rendering. height = .1 * log(value)
	std::vector<Vec3f> histoPoints;
	histoPoints.reserve(size.x * 2 + size.y * 2);

	if (show_histograms){
		for (int x = 0; x < size.x - 1; ++x)
		{
			histoPoints.push_back(Vec3f(-1.0f + float(x) / size.x * 2.0f, -.95f + .3f * histoX[x] / maxvalue.x, .0f));
			histoPoints.push_back(Vec3f(-1.0f + float(x + 1) / size.x * 2.0f, -.95f + .3f * histoX[x + 1] / maxvalue.x, .0f));
		}

		for (int y = 0; y < size.y - 1; ++y)
		{
			histoPoints.push_back(Vec3f(-.95f + .3f * histoY[y] / maxvalue.y, -1.0f + float(y) / size.y * 2.0f, .0f));
			histoPoints.push_back(Vec3f(-.95f + .3f * histoY[y + 1] / maxvalue.y, -1.0f + float(y + 1) / size.y * 2.0f, .0f));
		}
	}

	int inner_size = pointsearch_min;
	int outer_size = pointsearch_max;

	for (int i = 0; i < points[0].size(); ++i)
	{
		for (int j = 0; j < points[1].size(); ++j)
		{
			int x = points[0][i] + minOffset.x;
			int y = points[1][j] + minOffset.y;
			Vec2f pos = Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f;

			if (show_searchpoints){
				int x1 = (pos.x * .5f + .5f) * size.x - minOffset.x;
				int y1 = (pos.y * .5f + .5f) * size.y - minOffset.y;

				float radius = (float)inner_size;

				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				radius = outer_size;

				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 + radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
				histoPoints.push_back(Vec3f(Vec2f(x1 - radius, y1 + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
			}

			//compute average brightness at point
			float average_inner = get_average_square(points[0][i] * downscalefactor, points[1][j] * downscalefactor, imageData.data(), lastsize, inner_size);
			float average_outer = get_average_square(points[0][i] * downscalefactor, points[1][j] * downscalefactor, imageData.data(), lastsize, outer_size, inner_size);

			//if brightness below threshold, ignore
			if (average_outer + lightsearch_threshold >= average_inner)
				continue;


			//loop over previously seen blinkers, set position and last seen time if close enough
			bool found = false;
			int bestIdx = 0;
			int k = 0;
			float minlength = light_min_distance;
			for (auto& p : blinkers){
				auto len = (pos - p.getCurPos(timer)).length();
				if (len < minlength)
				{
					found = true;
					minlength = len;
					bestIdx = k;
				}
				++k;
			}

			//if we detected previously unknown blinker, add it
			if (!found)
				blinkers.push_back(blinker(pos, timer));
			else
			{
				//else, update the position of found blinker
				blinkers[bestIdx].updatePos(pos, timer);
			}
		}
	}


	//eliminate dead blinkers
	vector<blinker> new_blinkers;
	for (int i = 0; i < blinkers.size(); ++i)
	{
		auto& p = blinkers[i];
		if (!p.dead(timer))
			new_blinkers.push_back(p);
	}
	blinkers = new_blinkers;

	//render blinkers as squares
	//histoPoints.clear();
	for (auto& p : blinkers)
	{
		auto pos = p.getCurPos(timer);

		float radius = 8.0f / downscalefactor;
		if (!p.on(timer))
		{
			if (show_unseenblinkers)
				radius = 4.0f / downscalefactor;
			else
				continue;
		}

		int x = (pos.x * .5f + .5f) * size.x - minOffset.x;
		int y = (pos.y * .5f + .5f) * size.y - minOffset.y;


		histoPoints.push_back(Vec3f(Vec2f(x - radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(Vec3f(Vec2f(x + radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));

		histoPoints.push_back(Vec3f(Vec2f(x + radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(Vec3f(Vec2f(x + radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

		histoPoints.push_back(Vec3f(Vec2f(x + radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(Vec3f(Vec2f(x - radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));

		histoPoints.push_back(Vec3f(Vec2f(x - radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(Vec3f(Vec2f(x - radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));

		//draw anticipated movement
		float dt = 30.0f;
		for (float t = 0; t < 500.0f; t += dt)
		{
			histoPoints.push_back(Vec3f(Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f + p.getCurPos(t + p.time(timer)) - p.pos, 0));
			float t2 = t + dt;
			histoPoints.push_back(Vec3f(Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f + p.getCurPos(t2 + p.time(timer)) - p.pos, 0));
		}
	}

	renderer->renderVector(histoPoints, GL_LINES, Mat4f());
}