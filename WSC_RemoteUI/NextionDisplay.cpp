
#include "NextionDisplay.h"

NextionDisplay::NextionDisplay(SoftwareSerial &next, uint32_t baud) : nextion(&next) {
	nextion->begin(baud);
	Serial.flush();
	nextion->flush();
}

//Get display serial messages. Returns raw HEX data
String NextionDisplay::listen(unsigned long timeout) {	//returns generic
	char _bite;
	char _end = 0xff;	//end of file x3
	String cmd;
	int countEnd = 0;

	while (nextion->available()>0) {
		delay(10);
		if (nextion->available()>0) {
			_bite = nextion->read();
			cmd += _bite;
			if (_bite == _end) {
				countEnd++;
			}
			if (countEnd == 3) {
				break;
			}
		}
	}
	return cmd;
}

//Convert raw HEX data from display to easy to read format
String NextionDisplay::parseString(String cmd) {
	String temp = "";
	char _end = 0xff;
	int countEnd = 0;
	for (uint8_t i = 0; i<cmd.length(); i++) {
		if (cmd[i] == _end) {
			countEnd++;
		}
		temp += String(cmd[i], HEX);	//add hexadecimal value
		if (countEnd == 3) {
			return temp;
		}
		temp += " ";
	}
}

void NextionDisplay::sendCommand(const char* cmd) {
	while (nextion->available()) {
		nextion->read();
	}
	nextion->print(cmd);
	nextion->write(0xFF);
	nextion->write(0xFF);
	nextion->write(0xFF);
}

void NextionDisplay::sendCommand(String cmd) {
	while (nextion->available()) {
		nextion->read();
	}
	nextion->print(cmd.c_str());
	nextion->write(0xFF);
	nextion->write(0xFF);
	nextion->write(0xFF);
}
