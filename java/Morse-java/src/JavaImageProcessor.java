import java.util.ArrayList;

public class JavaImageProcessor {

	public ArrayList<Blinker> blinkers;
	public double light_min_distance;
	public double light_merge_threshold;
	public int lastID = 0;
	public int frameNumber = 0;
	
	public class Blinker // class that contains information about a blinker object
	{
		double x, y, brightness, size, mass;
		int ID, lastSeenFrame;
		boolean remove = false;
		ArrayList<Boolean> history = new ArrayList<Boolean>();
		
		public Blinker(DetectedLight light, int ID, int frame)
		{
			this.x = light.x;
			this.y = light.y;
			this.brightness = light.brightness;
			this.size = light.size;
			this.ID = ID;
			this.lastSeenFrame = frame;
		}
		
		public double mergeValue(double val1, double val2, double mass1, double mass2) // get weighted average of two values
		{
			return (val1*mass1 + val2 * mass2) / (mass1+mass2);
		}
		
		public void merge(DetectedLight light, int frame) // merge newly detected light and previously seen blinker
		{
			x = mergeValue(light.x, x, 1.0, mass);
			y = mergeValue(light.y, y, 1.0, mass);
			brightness = mergeValue(light.brightness, brightness, 1.0, mass);
			mass += 1.0;
			lastSeenFrame = frame;
		}
		
		public void merge(Blinker other, int frame) // merge two blinkers
		{
			x = mergeValue(other.x, x, other.mass, mass);
			y = mergeValue(other.y, y, other.mass, mass);
			brightness = mergeValue(other.brightness, brightness, other.mass, mass);
			mass += other.mass;
			lastSeenFrame = frame;
		}
		
		public double getDistance(Blinker other)
		{
			double xdist = x-other.x;
			double ydist = y-other.y;
			return Math.sqrt(xdist*xdist + ydist*ydist);
		}
	}
	
	public class DetectedLight // class that describes one pixel on the image that contains a light
	{
		int x, y, brightness, size;
		public DetectedLight(int x, int y, int brightness, int size)
		{
			this.x = x;
			this.y = y;
			this.brightness = brightness;
			this.size = size;
		}
		
		public double getDistance(Blinker other)
		{
			double xdist = x-other.x;
			double ydist = y-other.y;
			return Math.sqrt(xdist*xdist + ydist*ydist);
		}
	}

	
	public void ProcessLights(int[] lightsList)
	{
		frameNumber++;
		
		//first, loop over incoming new lights. Either merge them to existing blinkers or add them as new blinkers if none are close enough
		for(int i = 0; i < lightsList.length / 4; ++i)
		{
			DetectedLight curDetectedLight = new DetectedLight(lightsList[i*4+0], lightsList[i*4+1], lightsList[i*4+2], lightsList[i*4+3]);

			boolean found = false;
			boolean anyInRange = false;
			int bestIdx = 0;
			int k = 0;
			double mincost = light_min_distance;
			
			//loop over previously seen blinkers, set position and last seen time if close enough
			for (Blinker p : blinkers)
			{
				double length = curDetectedLight.getDistance(p);
				double cost = length / (p.size + curDetectedLight.size);
				if (cost < mincost && Math.abs(curDetectedLight.brightness - p.brightness) < light_merge_threshold)
				{
					found = true;
					mincost = cost;
					bestIdx = k;
				}
				++k;
			}

			//if we detected previously unknown blinker, add new light to blinkers list
			if (!found)
			{
				blinkers.add(new Blinker(curDetectedLight, lastID++, frameNumber));
			}
			else if (found)
			{
				//else, update the position of found best blinker match
				blinkers.get(bestIdx).merge(curDetectedLight, frameNumber);
			}
		}
		

		//merge blinkers that are too close to each other
		for (Blinker p : blinkers)
		{
			for (Blinker q : blinkers)
			{
				if (p.ID == q.ID)
					continue;
				double length = q.getDistance(p);
				double cost = length / (p.size + q.size);
				if (cost < light_min_distance && Math.abs(q.brightness - p.brightness) < light_merge_threshold)
				{
					Blinker smaller = p;
					Blinker larger = q;
					if (p.ID < q.ID) //choose the older blinker, the other one gets removed
					{
						larger = p;
						smaller = q;
					}

					smaller.remove = true;
					larger.merge(smaller, frameNumber);
				}
			}
		}
		
		// discard blinkers that have not been seen in a while, and removed blinkers
		ArrayList<Blinker> new_blinkers = new ArrayList<Blinker>();
		for (int i = 0; i < blinkers.size(); ++i)
		{
			Blinker p = blinkers.get(i);
			p.mass = 0;
			if (p.lastSeenFrame - frameNumber < 30 && !p.remove)
				new_blinkers.add(p);
		}
		blinkers = new_blinkers;
		
		//loop over blinkers and interpret their history to strings
		for (Blinker p : blinkers)
		{
			if(p.lastSeenFrame == frameNumber)
				p.history.add(Boolean.TRUE);
			else
				p.history.add(Boolean.FALSE);
			String s = "";
			for (int i = 0; i < p.history.size(); ++i){
				boolean sign = p.history.get(i);
				int len = 1;
				while (true){
					if (i >= p.history.size())
						break;

					if (p.history.get(i) == sign)
						len++;
					else
						break;
					i++;
				}
				if (!sign)
					s += " ";
				else if (len < 6)
					s += ".";
				else if (len < 15)
					s += "-";
			}
		}
	}
}