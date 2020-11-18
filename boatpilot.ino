// autonomous boat project
// Jonathon Simister

#include <Wire.h>
#include <ServoDriver.h>
#include "GPSHelper.h"
#include <SD.h>
#include <TinyScreen.h>
#include "FileParser.h"
#include "avr/dtostrf.h"
#include "BatteryHelper.h"

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#include <SoftwareSerial.h>
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#include "SoftwareSerialZero.h"
#endif

TinyScreen display = TinyScreen(TinyScreenPlus);
ServoDriver servo(NO_R_REMOVED);
GPSHelper gpsHelper;
BatteryHelper batteryHelper;

#define SD_CARD_PIN 10

GPSPoint startPosition;
GPSPoint* waypoints = new GPSPoint[20];
int nwaypoints = 0;
int waypointi = 0;

const char* keywordGoto = "goto";
const char* keywordSetBatteryLogFrequency = "set batterylogfrequency";
const char* keywordSetRudderRange = "set rudderrange";
const char* keywordSetRudderLog = "set rudderlog";

const int nkeywords = 2;
const char* keywords[] = { keywordGoto, keywordSetBatteryLogFrequency, keywordSetRudderLog, keywordSetRudderRange };

int batteryLogFrequency = -1;
int rudderLog = 1;

int rudderLeft = 2300;
int rudderCenter = 1750;
int rudderRight = 1200;

int rudderPosition;

char logFilename[100];

void dataLogger(const char* s) {
  File logFile = SD.open(logFilename, FILE_WRITE);
  
  if (logFile) {
      logFile.write(s);
      logFile.close();
  }
}

void gpsLogger(const char* s) {
  int callCount = 0;
  callCount++;
  
  dataLogger(s);
  
  if(batteryLogFrequency != -1 && callCount % batteryLogFrequency == 0) {
    char buf[100];
    char buf2[100];

    float volts = batteryHelper.getVoltage();
    dtostrf(volts, 4, 2, buf2);

    sprintf(buf2, "# battery level %s V\b", buf2);
    dataLogger(buf2);
  }
}

void setup() {
  display.begin();
  display.setBrightness(10);

  Wire.begin();

  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);

  servo.useResetPin();         //This tells the library to use the reset line attached to Pin 9
  if(servo.begin(20000)){      //Set the period to 20000us or 20ms, correct for driving most servos
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,0);
    
    display.print("servo begin failed!");
    
    while(1);
  }
  //The failsafe turns off the PWM output if a command is not sent in a certain amount of time.
  //Failsafe is set in milliseconds- comment or set to 0 to disable
  servo.setFailsafe(5000);

  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  
  while (!SD.begin(SD_CARD_PIN)) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,0);
    
    display.print("SD card not present!");
    delay(500);
  }

  if(!SD.exists("route.txt")) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,0);
    
    display.print("route.txt not found!");
    while(true) { }
  }

  display.clearScreen();

  bool parseFailed = false;
  FileParser* parser = new FileParser("route.txt");

  const char* keyword;
  while(parser->readKeyword(keywords, nkeywords, &keyword)) {
	  if(keyword == keywordGoto) {
		  float lat, lon;
    
		  bool a = parser->readFloat(&lat);
		  bool b = parser->expectString(",");
		  bool c = parser->readFloat(&lon);

		  if(!a || !b || !c) { parseFailed = true; break; }

		  waypoints[nwaypoints++] = GPSPoint(lat, lon);
	  } else if(keyword == keywordSetBatteryLogFrequency) {
		  float logFreq;
		
		  parser->readFloat(&logFreq);

      batteryLogFrequency = logFreq;
	  } else if(keyword == keywordSetRudderLog) {
      float f;

      parser->readFloat(&f);

      rudderLog = f;
	  } else if(keyword == keywordSetRudderRange) {
      float left, center, right;

      bool a = parser->readFloat(&left);
      bool b = parser->readFloat(&center);
      bool c = parser->readFloat(&right);

      if(!a || !b || !c) { parseFailed = true; break; }

      rudderLeft = left;
      rudderCenter = center;
      rudderRight = right;
	  }
  }

  rudderPosition = rudderCenter;
  servo.setServo(4, rudderCenter);

  delete parser;

  if(parseFailed) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,0);
    
    display.print("route.txt corrupt!");
    while(true) { }
  }

  if(nwaypoints < 1) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,0);
    
    display.print("no waypoints!");
    while(true) { }
  }
  
  display.fontColor(TS_8b_Yellow,TS_8b_Black);
  display.setCursor(0,0);
  display.print("Starting up...");

  gpsHelper.setup();

  int r;
  int gpsSpinner = 0;

  do {
    display.clearScreen();
    display.setCursor(0,0);

    gpsSpinner = (gpsSpinner + 1) % 4;

    char wait[100];
    strcpy(wait, "Waiting for GPS");

    for(int i = 0;i < gpsSpinner;i++) {
      strcat(wait, ".");
    }
      
    display.print(wait);

    delay(250);
  
    r = gpsHelper.poll(&startPosition, NULL, NULL);
  } while(r != POLL_DATARETURNED);

  // compute log file name
  strcpy(logFilename, "log.txt");
  int logOrdinal = 1;

  while(SD.exists(logFilename)) {
    sprintf(logFilename, "log %d.txt", ++logOrdinal);
  }

  gpsHelper.registerRawReceiver(gpsLogger);
}

void loop() {
  GPSPoint destination;
  
  if(waypointi < nwaypoints) {
    destination = waypoints[waypointi];
  } else {
    destination = startPosition;
  }

  GPSPoint position;
  float speed;
  float heading;
  int r = gpsHelper.poll(&position, &speed, &heading);

  if(r != POLL_DATARETURNED) {
    return;
  }
  
  float distance = position.distance(destination);
  float bearing = position.headingTo(destination);

  if(distance < 5 && waypointi < nwaypoints) {
    waypointi++;
    return;
  }

  float anglediff = abs(heading -  bearing);
  float leftTurn = heading - bearing;
  float rightTurn = bearing - heading;

  if(leftTurn < 0) { leftTurn += 360; }
  if(rightTurn < 0) { rightTurn += 360; }
  
  int newRudder;
  if(anglediff < 20) {
    newRudder = rudderCenter;
  } else {
    newRudder = leftTurn < rightTurn ? rudderLeft : rudderRight;
  }

  if(rudderLog == 1 && rudderPosition != newRudder) {
    const char* rudderS = newRudder == rudderCenter ? "center" :
      newRudder == rudderLeft ? "left" : "right";

    char headingS[100];
    char bearingS[100];
    dtostrf(heading, 5, 2, headingS);
    dtostrf(bearing, 5, 2, bearingS);
    char buf[100];
    int n = sprintf(buf, "# moving rudder to %s (heading %s, bearing %s)\n", rudderS, headingS, bearingS);

    dataLogger(buf);
  }

  rudderPosition = newRudder;
  
  servo.setServo(4, rudderPosition);
  

  char line1[100];
  char line2[100];
  char line3[100];
  char line4[100];
  char line5[100];
  char line6[100];
     
  position.latString(line1);
  position.lonString(line2);
  dtostrf(heading, 5, 2, line3);
  dtostrf(bearing, 5, 2, line4);
  dtostrf(distance, 5, 1, line5);
  
  sprintf(line6, "(%d/%d)", waypointi, nwaypoints);

  display.clearScreen();

  display.setFont(thinPixel7_10ptFontInfo);
  int width6 = display.getPrintWidth(line6);

  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.setCursor(0,0);
  display.print(line1);
  display.setCursor(0,12);
  display.print(line2);
    
  display.fontColor(TS_8b_Gray,TS_8b_Black);
  display.setCursor(0,24);
  display.print(line3);
      
  display.fontColor(TS_8b_White,TS_8b_Black);
  display.setCursor(0,36);
  display.print(line4);

  display.fontColor(TS_8b_Blue,TS_8b_Black);
  display.setCursor(0,48);
  display.print(line5);

  display.fontColor(TS_8b_Yellow,TS_8b_Black);
  display.setCursor(95 - width6,48);
  display.print(line6);

  delay(500);
}
