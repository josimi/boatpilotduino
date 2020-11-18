// GPSHelper
// Jonathon Simister

#ifndef GPSHelper_h
#define GPSHelper_h

#include "GPSPoint.h"

#if defined (ARDUINO_ARCH_AVR)
#include <SoftwareSerial.h>
#elif defined(ARDUINO_ARCH_SAMD)
#include "SoftwareSerialZero.h"
#endif

#define POLL_GPSNOTREADY -1
#define POLL_NONEWDATA 0
#define POLL_DATARETURNED 1

class GPSHelper {
public:
	GPSHelper();

	void setup();
	
	void registerRawReceiver(void (*receive)(const char*));
	
	int poll(GPSPoint* position, float* speed, float* heading);
	
private:
	void (*rawReceiver)(const char*);
	SoftwareSerial* softSerial;
  char** splitBuffer;
	
	char waitForCharacter();
	int handleGPS(char* buffer);
  void splitString(const char* s, char** a);
};

#endif
