#pragma once
#include <PID_v1.h>

enum state {
	off_state,
	idle_state,
	heat_state,
};

enum alarm {
	fireBox_Over_Temp,
	heatExchanger_Over_Temp,
	smoke,
	carbonMonoxide
};

struct state_params {
	bool fan1;		//low = off, high = on
	bool fan2;		//low = off, high = on
	byte stokeFanSpeed_cmd;		//speed in % 0-100
	byte damperValvePosition_cmd; //position in % 0-100	
	double fireBoxTemp_cmd;			//temp
	double heatExchangerTemp_cmd;
}off, idle, heat;

double fireBoxTemp;
double heatExchangerTemp;
byte stokeFanSpeed_meas;
byte damperValvePosition_meas;
double downStairsTemp;
double upStairsTemp;
byte temp_slack = 5;
double Setpoint, Input, Output;
double Kp = 4, Ki = 1, Kd = 2;

int timer_FireBox_Over_Temp = 30000;
int timer_heatExchanger_Over_Temp = 30000;
int timer_smoke = 30000;
int timer_carbonMonoxide = 30000;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

