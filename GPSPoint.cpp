#include "GPSPoint.h"
#include "avr/dtostrf.h"
#include <cstdio>

#define pi 3.141592653

GPSPoint::GPSPoint(float latitude, float longitude) {
	this->lat = latitude;
	this->lon = longitude;
}

GPSPoint::GPSPoint() {
	this->lat = 0;
	this->lon = 0;
}

void GPSPoint::latString(char* out) {
	double deg;
	double min = modf(this->lat, &deg) * 60;
	
	char dir = deg < 0 ? 'S' : 'N';
	
	int degi = (int)deg;
	
	char mins[100];
	dtostrf(fabs(min), 7, 4, mins);
	
	sprintf(out, "%d %s %c", abs(degi), mins, dir);
}

void GPSPoint::lonString(char* out) {
	double deg;
	double min = modf(this->lon, &deg) * 60;
	
	char dir = deg < 0 ? 'W' : 'E';
	
	int degi = (int)deg;
	
	char mins[100];
	dtostrf(fabs(min), 7, 4, mins);
	
	sprintf(out, "%d %s %c", abs(degi), mins, dir);
}

double GPSPoint::distance(GPSPoint& p) {
	double R = 6371000; // metres
	double radLat1 = this->lat * pi/180;
	double radLat2 = p.lat * pi/180;
	double deltaPhi = (p.lat-this->lat) * pi/180;
	double deltaLambda = (p.lon-this->lon) * pi/180;
	
	double a = sin(deltaPhi/2) * sin(deltaPhi/2) +
          cos(radLat1) * cos(radLat2) *
          sin(deltaLambda/2) * sin(deltaLambda/2);
	double c = 2 * atan2(sqrt(a), sqrt(1-a));

	return R * c;
}

float GPSPoint::headingTo(GPSPoint& p) {
	float y = sin(p.lon - this->lon) * cos(p.lat);
	float x = cos(this->lat) * sin(p.lat) - sin(p.lat)*cos(p.lat)*cos(p.lon-this->lon);
  float theta = atan2(y, x);
	float bearing = (theta*180/pi + 360);
	
	if(bearing > 360) { bearing -= 360; }
	
	return bearing;
}
