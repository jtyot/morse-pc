#include "ImageProcessor.hpp"
#include "Renderer.hpp"

using namespace FW;
using namespace std;

int blinker::blinkerID = 0;


bool inBounds(Vec2i pos, Vec2i size)
{
	return !(pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y);
}


void ImageProcessor::processImage(const std::vector<Vec3u8>& imageData, const Vec2i& size_in)
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
	float shadowWeight = 1.0f / (1.0f - shadowCutOff);
	for (int j = 0; j < lastsize.y; j++)
	{
		for (int i = 0; i < lastsize.x; i++)
		{
			auto orig_value = imageData[j * lastsize.x + i].getVec3f();
			auto value = dot(orig_value, Vec3f(.333f)); //greyscale for histogram
			value = max(.0f, value - shadowCutOff) * shadowWeight;
			int x = i / downscalefactor;
			int y = j / downscalefactor;
			histoX[x] += value;
			histoY[y] += value;

			value = orig_value.z; //clamped blue channel for light search
			value = max(.0f, value - shadowCutOff) * shadowWeight;

			float cur_sum = value;
			if (inBounds(Vec2i(x - 1, y), size))
				cur_sum += sumImage[y * size.x + (x - 1)];
			if (inBounds(Vec2i(x, y - 1), size))
				cur_sum += sumImage[(y - 1) * size.x + x];
			if (inBounds(Vec2i(x - 1, y - 1), size))
				cur_sum -= sumImage[(y - 1) * size.x + (x - 1)];

			sumImage[y * size.x + x] = cur_sum;
		}
	}

	//normalize histogram by dividing values by number of pixels in sum
	for (int i = 0; i < size.x; ++i)
	{
		float w = 1.0f - abs(i - size.x / 2.0f) / (size.x / 2.0f); //pixels near center of image have higher importance
		histoX[i] *= invsizex * w;
		maxvalue.x = max(histoX[i], maxvalue.x);
	}

	for (int i = 0; i < size.y; ++i)
	{
		float w = 1.0f - abs(i - size.y / 2.0f) / (size.y / 2.0f); //pixels near center of image have higher importance
		histoY[i] *= invsizey * w;
		maxvalue.y = max(histoY[i], maxvalue.y);
	}

	std::vector<float>* histograms[2];
	histograms[0] = &histoX;
	histograms[1] = &histoY;

	std::vector<Vec3f> histoPoints;
	histoPoints.reserve(size.x * 2 + size.y * 2);

	//render histograms
	if (show_histograms)
	{
		for (int axis = 0; axis < 2; axis++){
			for (int i = 1; i < size[axis]; ++i)
			{
				Vec3f pos1, pos2;
				pos1[axis] = (*histograms[axis])[i - 1] / maxvalue[axis] *.2f - .95f;
				pos2[axis] = (*histograms[axis])[i] / maxvalue[axis] * .2f - .95f;
				pos1[(axis + 1) % 2] = float(i - 1.0f) / size[axis] * 2.0f - 1.0f;
				pos2[(axis + 1) % 2] = float(i) / size[axis] * 2.0f - 1.0f;
				histoPoints.push_back(pos1);
				histoPoints.push_back(pos2);
			}
		}
	}

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
					float diff = cur - prev;
					curError += diff * diff;
					count ++;
					(*prevHistograms[axis])[i] = (*histograms[axis])[x];
				}

				if (count == .0f)
					continue;

				curError /= count;
				float moveCost = float(abs(offset - lastoffset[axis])) / downscalefactor / 100.0f;
				curError += moveCost * moveCost * 0.0009f;
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


	int sum_count = pointsearch_max;
	int sum_scale = pointsearch_min;
	std::vector<float> sumSquares(sum_count);
	std::vector<int> sumSizes(sum_count);
	int k = 0;
	for (auto& l : sumSizes)
		l = sum_scale * pow(2.0f, k++);

	for (auto& p : blinkers)
		p.isDead = true;
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
			float mincost = light_min_distance;
			for (auto& p : blinkers){
				auto cost = (pos - p.getCurPos(timer)).length() / (p.size + sumSizes[max_idx]);
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
			{
				blinkers.push_back(blinker(pos, timer, sumSizes[max_idx], max_diff));
				blinkers[blinkers.size() - 1].ID = blinker::blinkerID++;
			}
			else if (found)
			{
				//else, update the position of found blinker
				blinkers[bestIdx].mergePos(pos, sumSizes[max_idx], max_diff);
			}
		}
	}

	//merge blinkers that are too close to each other
	for (auto& p : blinkers)
	{
		for (auto& q : blinkers)
		{
			if (p == q || p.isDead || q.isDead)
				continue;
			if ((p.pos - q.pos).length() < (q.size + p.size) * light_min_distance * .5f)
			{
				auto& smaller = p;
				auto& larger = q;
				if (p.ID < q.ID) //choose the older blinker, the other one gets removed
				{
					std::swap(p, q);
				}

				smaller.remove = true;
				larger.mergePos(smaller.pos, smaller.size, smaller.brightness, smaller.mass);
			}
		}
	}

	for (auto& p : blinkers)
		p.update(timer);

	renderer->renderVector(histoPoints, GL_LINES, Mat4f());
	histoPoints.clear();
	//eliminate dead blinkers
	vector<blinker> new_blinkers;
	for (int i = 0; i < blinkers.size(); ++i) //discard blinkers that have not been seen in a while, and removed blinkers
	{
		auto& p = blinkers[i];
		p.mass = 0;
		if (!p.dead(timer) && !p.remove)
			new_blinkers.push_back(p);
	}
	blinkers = move(new_blinkers);

	//render blinkers as squares
	for (auto& p : blinkers)
	{
		auto pos = p.getCurPos(timer);

		float radius = p.size;
		if (p.isDead)
		{
			if (show_unseenblinkers)
				radius *= .5f;
			else
				continue;
		}

		int x = (pos.x * .5f + .5f) * size.x - minOffset.x;
		int y = (pos.y * .5f + .5f) * size.y - minOffset.y;


		histoPoints.push_back(Vec3f(Vec2f(x - radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		histoPoints.push_back(Vec3f(Vec2f(x + radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);

		histoPoints.push_back(Vec3f(Vec2f(x + radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		histoPoints.push_back(Vec3f(Vec2f(x + radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);

		histoPoints.push_back(Vec3f(Vec2f(x + radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		histoPoints.push_back(Vec3f(Vec2f(x - radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);

		histoPoints.push_back(Vec3f(Vec2f(x - radius, y - radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		histoPoints.push_back(Vec3f(Vec2f(x - radius, y + radius) / Vec2f(size) * 2.0f - 1.0f, 0));
		histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);

		//draw anticipated movement
		//float dt = 30.0f;
		//for (float t = 0; t < 500.0f; t += dt)
		//{
		//	histoPoints.push_back(Vec3f(Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f + p.getCurPos(t + p.time(timer)) - p.pos, 0));
		//	histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		//	float t2 = t + dt;
		//	histoPoints.push_back(Vec3f(Vec2f(x, y) / Vec2f(size) * 2.0f - 1.0f + p.getCurPos(t2 + p.time(timer)) - p.pos, 0));
		//	histoPoints.push_back(blinkerColors[p.ID % blinkerColors.size()]);
		//}
	}

	renderer->renderVectorColored(histoPoints, GL_LINES, Mat4f());
}