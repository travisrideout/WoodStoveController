/*
 Name:		WoodStoveController.ino
 Created:	11/5/2016 8:20:11 PM
 Author:	Travis Rideout
*/

#include "WoodStoveController.h"

void setup() {
	Serial.begin(115200);

	InitializeStateVariables();
}

void loop() {
	//if change in set temp then change state
	//get room temps
	//if temp below setpoint - temp_slack then heat
	//if temp above setpoint then idle
	//if any alarms then off state and send alarm notification
	//if trying to heat and firebox temp not going up then out of fuel
	
}

void InitializeStateVariables() {
	off = {
		LOW,	//fan 1
		LOW,	//fan2
		0,		//stokeFanSpeed_cmd
		0,		//damperValvePosition_cmd
		30,		//fireBoxTemp_cmd
		30,		//heatExchangerTemp_cmd
	};

	idle = {
		LOW,	//fan 1
		LOW,	//fan2
		0,		//stokeFanSpeed_cmd
		10,		//damperValvePosition_cmd
		300,	//fireBoxTemp_cmd
		100,	//heatExchangerTemp_cmd
	};	

	heat = {
		HIGH,	//fan 1
		HIGH,	//fan2
		100,	//stokeFanSpeed_cmd
		100,	//damperValvePosition_cmd
		30,		//fireBoxTemp_cmd
		30,		//heatExchangerTemp_cmd
	};
}