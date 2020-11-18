// GPSHelper
// Jonathon Simister

#include <Arduino.h>

#include "GPSHelper.h"
#include "GPS.h"

GPSHelper::GPSHelper() {
	this->softSerial = new SoftwareSerial(GPS_RXPin, GPS_TXPin);
	this->rawReceiver = NULL;

  this->splitBuffer = new char*[15];

  for(int i = 0;i < 15;i++) {
    this->splitBuffer[i] = new char[100];
  }
}

void GPSHelper::splitString(const char* s, char** a) {
  const char* p = s;
  int x = 0;
  int y = 0;

  while (*p != 0) {
    while (*p != ',' && *p != 0) {
      a[x][y++] = *p++;
    }

    a[x][y] = 0;

    if (*p == 0) {
      return;
    }

    x++;

    if(x > 14) { return; }
    
    p++;
    y = 0;
  }
}

void GPSHelper::setup() {
	softSerial->begin(GPSBaud);
	
	gpsInitPins();
	delay(100);
	gpsOn();
	delay(200);
	
	//Enable and set interval or disable, per NMEA sentence type
	softSerial->print(gpsConfig(NMEA_GGA_SENTENCE, 1));
	softSerial->print(gpsConfig(NMEA_GLL_SENTENCE, 0));
	softSerial->print(gpsConfig(NMEA_GSA_SENTENCE, 0));
	softSerial->print(gpsConfig(NMEA_GSV_SENTENCE, 0));
	softSerial->print(gpsConfig(NMEA_RMC_SENTENCE, 1));
	softSerial->print(gpsConfig(NMEA_VTG_SENTENCE, 0));
	softSerial->print(gpsConfig(NMEA_GNS_SENTENCE, 0));
}

void GPSHelper::registerRawReceiver(void (*receive)(const char*)) {
  this->rawReceiver = receive;
}

int substrcmp(const char* str1, const char* str2, int n) {
  for(int i = 0;i < n;i++) {
    if(str1[i] != str2[i]) {
      return str1[i] - str2[i];
    }
  }

  return 0;
}

int GPSHelper::poll(GPSPoint* position, float* speed, float* heading) {
	char buf[100];
	
	int n = handleGPS(buf);
	
	if(n > 0 && this->rawReceiver != NULL) {
		this->rawReceiver(buf);
	}
	
	if(substrcmp(buf, "$GPRMC", 6) != 0) {
		return POLL_NONEWDATA;
	}

  splitString(buf, this->splitBuffer);
	
	const char* s1 = this->splitBuffer[3];
	const char* latDir = this->splitBuffer[4];
	const char* s3 = this->splitBuffer[5];
	const char* lonDir = this->splitBuffer[6];
	const char* s4 = this->splitBuffer[7];
	const char* s5 = this->splitBuffer[8];

	char latD[5];
	latD[0] = s1[0];
	latD[1] = s1[1];
	latD[2] = 0;

	char lonD[5];
	lonD[0] = s3[0];
	lonD[1] = s3[1];
	lonD[2] = s3[2];
	lonD[3] = 0;

	char latM[100];
	char lonM[100];

	strcpy(latM, s1+2);
	strcpy(lonM, s3+3);
		
	if(latDir[0] != 'N' && latDir[0] != 'S') {
		return POLL_GPSNOTREADY;
	}
	
	int latDeg = atoi(latD);
	int lonDeg = atoi(lonD);
	
	float latMin = atof(latM);
	float lonMin = atof(lonM);
	
	float lat = latDeg + (latMin / 60);
	float lon = lonDeg + (lonMin / 60);
	
	if(latDir[0] == 'S') { lat = 0 - lat; }
	if(lonDir[0] == 'W') { lon = 0 - lon; }
	
	if(position != NULL) {
		*position = GPSPoint(lat, lon);
	}
	
	if(speed != NULL) {
		*speed = atof(s4);
	}
	
	if(heading != NULL) {
		*heading = atof(s5);
	}
	
	return POLL_DATARETURNED;
}

char GPSHelper::waitForCharacter() {
  while (!softSerial->available());
  return softSerial->read();
}

int GPSHelper::handleGPS(char* buffer) {
  while (softSerial->read() != '$') {
    if(!softSerial->available()){
      return 0;
    }
  }
  int counter = 1;
  char c = 0;
  buffer[0] = '$';
  c = waitForCharacter();
  while (c != '*') {
    buffer[counter++] = c;
    if(c=='$'){//new start
      counter=1;
    }
    c = waitForCharacter();
  }
  buffer[counter++] = c;
  buffer[counter++] = waitForCharacter();
  buffer[counter++] = waitForCharacter();
  buffer[counter++] = '\r';
  buffer[counter++] = '\n';
  buffer[counter] = '\0';

  buffer[1] = 'G';
  buffer[2] = 'P';

  gpsDoChecksum(buffer);

  return counter;
}
