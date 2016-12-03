// NextionDisplay.h

#ifndef _NEXTIONDISPLAY_h
#define _NEXTIONDISPLAY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <SoftwareSerial.h>

class NextionDisplay {
private:
	SoftwareSerial *nextion;

public:
	NextionDisplay(SoftwareSerial &next, uint32_t baud);	//Constructor
	String listen(unsigned long timeout = 100);
	String parseString(String cmd);
	void sendCommand(const char* cmd);
	void sendCommand(String cmd);
};
#endif