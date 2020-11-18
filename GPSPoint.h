#ifndef GPSPoint_h
#define GPSPoint_h

#include <cmath>

class GPSPoint {
	public:
		GPSPoint();
		GPSPoint(float latitude, float longitude);
		
		double distance(GPSPoint& p);
		float headingTo(GPSPoint& p);
		
		void latString(char* out);
		void lonString(char* out);
		
	private:
		float lat, lon;
};

#endif
