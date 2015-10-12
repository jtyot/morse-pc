#include "ImageProcessor.hpp"
#include "Renderer.hpp"

using namespace FW;
using namespace std;

bool inBounds(Vec2i pos, Vec2i size)
{
	return !(pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y);
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
		sumImage.resize((lastsize.x / downscalefactor + 1) * (lastsize.y / downscalefactor + 1));
	}

	Vec2i size = lastsize / downscalefactor;

	//initialize histogram vectors
	memset(histoX.data(), 0, histoX.size() * sizeof(float));
	memset(histoY.data(), 0, histoY.size() * sizeof(float));
	memset(sumImage.data(), 0, sumImage.size() * sizeof(float));

	Vec2f maxvalue = .0f; //maximum value of each histogram

	//compute histogram from image rows, loop over all pixels and sum it to the proper vector
	float invsizex = 1.0f / lastsize.x;
	float invsizey = 1.0f / lastsize.y;
	for (int j = 0; j < lastsize.y; j++)
	{
		for (int i = 0; i < lastsize.x; i++)
		{
			auto value = imageData[j * lastsize.x + i].z; //read only blue channel
			int x = i / downscalefactor;
			int y = j / downscalefactor;
			histoX[x] += value;
			histoY[y] += value;

			//sumImage[j / downscalefactor * size.x + i / downscalefactor] += value;
			float cur_sum = value;
			/*if ((j%downscalefactor) != 0 || (i%downscalefactor) != 0)
			{
				sumImage[y * size.x + x] = cur_sum;
				continue;
			}*/
			if (inBounds(Vec2i(x - 1, y), size))
				cur_sum += sumImage[y * size.x + (x - 1)];
			if (inBounds(Vec2i(x, y - 1), size))
				cur_sum += sumImage[(y - 1) * size.x + x];
			if (inBounds(Vec2i(x - 1, y - 1), size))
				cur_sum -= sumImage[(y - 1) * size.x + (x - 1)];

			sumImage[y * size.x + x] = cur_sum;
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
					if (i < 0 || i >= size[axis]) //if out of bounds, skip pixel
					{
						continue;
					}
					if ((*prevHistograms[axis])[i] == 0)
					{
						(*prevHistograms[axis])[i] = (*histograms[axis])[x];
						continue;
					}

					auto cur = (*histograms[axis])[x];
					auto prev = (*prevHistograms[axis])[i];
					auto weight = 1.0f - abs(float(offset)) / size[axis] * maxdiffscale + .5f;
					curError += pow((cur - prev), 2.0f) * weight;
					count += weight;
					(*prevHistograms[axis])[i] = (*histograms[axis])[x];
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

	std::vector<Vec3f> histoPoints;
	histoPoints.reserve(size.x * 2 + size.y * 2);

	int sum_count = pointsearch_max;
	int sum_scale = pointsearch_min;
	std::vector<float> sumSquares(sum_count);
	std::vector<int> sumSizes(sum_count);
	int k = 0;
	for (auto& l : sumSizes)
		l = sum_scale * pow(2.0f, k++);

	//blinkers.clear();
	//loop over all pixels to find lights
	for (int i = 0; i < size.x; ++i)
	{
		for (int j = 0; j < size.y; ++j)
		{
			int x = i + minOffset.x;
			int y = j + minOffset.y;
			Vec2f pos = Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f;

			//first, compute all square sums centered on current pixel
			for (int k = 0; k < sum_count; ++k)
			{
				int x_right = i + sumSizes[k], x_left = i - sumSizes[k], y_top = j + sumSizes[k], y_bot = j - sumSizes[k];
				float cur_sum = .0f;
				if (!inBounds(Vec2i(x_right, y_bot), size) || !inBounds(Vec2i(x_left, y_top), size))
				{
					sumSquares[k] = .0f;
					continue;
				}
				cur_sum += sumImage[y_top * size.x + x_right];
				cur_sum -= sumImage[y_bot * size.x + x_right];
				cur_sum -= sumImage[y_top * size.x + x_left];
				cur_sum += sumImage[y_bot * size.x + x_left];
				sumSquares[k] = cur_sum;
			}

			//iterate over pairs of squares, getting the average brightness ratio of inner vs outer, and selecting the size with highest average brightness difference
			float max_diff = lightsearch_threshold;
			int max_idx = 0;
			bool found = false;
			for (int k = 0; k < sum_count - 1; ++k)
			{
				float inner = sumSquares[k];
				float outer = sumSquares[k + 1];
				if (outer == .0f)
					continue;
				outer -= inner;

				int inner_area = (sumSizes[k] * 2 + 1) * (sumSizes[k] * 2 + 1);
				int outer_area = (sumSizes[k + 1] * 2 + 1) * (sumSizes[k + 1] * 2 + 1) - inner_area;
				float inner_average = inner / inner_area;
				float outer_average = outer / outer_area;
				float cur_diff = inner_average - outer_average;
				if (cur_diff > max_diff)
				{
					found = true;
					max_diff = cur_diff;
					max_idx = k;
				}
			}

			//compute average brightness at point

			//if brightness below threshold, ignore
			if (!found)
				continue;
			//blinkers.push_back(blinker(pos, timer, sumSizes[max_idx], .0f));

			//loop over previously seen blinkers, set position and last seen time if close enough
			found = false;
			bool anyInRange = false;
			int bestIdx = 0;
			int k = 0;
			float mincost = light_min_distance * sumSizes[max_idx];
			for (auto& p : blinkers){
				auto cost = (pos - p.getCurPos(timer)).length();
				if (cost < mincost)
				{
					found = true;
					mincost = cost;
					bestIdx = k;
				}
				++k;
			}

			//if we detected previously unknown blinker, add it
			if (!found)
				blinkers.push_back(blinker(pos, timer, sumSizes[max_idx], max_diff));
			else if (found)
			{
				//else, update the position of found blinker
				blinkers[bestIdx].updatePos(pos, timer, sumSizes[max_idx], max_diff);
			}
		}
	}


	//eliminate dead blinkers
	vector<blinker> new_blinkers;
	for (int i = 0; i < blinkers.size(); ++i)
	{
		auto& p = blinkers[i];
		p.mass = 1;
		if (!p.dead(timer))
			new_blinkers.push_back(p);
	}
	blinkers = new_blinkers;

	//render blinkers as squares
	//histoPoints.clear();
	for (auto& p : blinkers)
	{
		auto pos = p.getCurPos(timer);

		float radius = p.size;// / downscalefactor;
		if (!p.on(timer))
		{
			if (show_unseenblinkers)
				radius *= .5f;
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