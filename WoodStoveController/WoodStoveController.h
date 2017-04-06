#pragma once
//#include <WiFiEspUdp.h>
//#include <WiFiEspServer.h>

#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <max6675.h>
#include <RF24_config.h>
#include <RF24.h>
#include <printf.h>
#include <nRF24L01.h>
#include <PID_v1.h>
#include <Rideout.h>

#define DHTTYPE DHT11   

const uint64_t pipes[2] = { 0xffcadbdacfLL, 0x5632867136LL };

//pin declarations *interrupt pins = 2,3,18,19,20,21
const byte encoder0PinA = 2;
const byte encoder0PinB = 3;
const byte fanRelay_Pin = 4;
const byte DHT_Pin = 5;
const byte thermoCLK = 6;
const byte thermoSO = 7;
const byte thermo1CS = 8;
const byte thermo2CS = 9;
const byte buzzer_Pin = 10;
const byte damperPWM_Pin = 11;	
const byte stokeFan_Pin = 12;	
const byte damperDir_Pin = 13;
const byte nrfInt_Pin = 18;
const byte smoke_Pin = A0;
const byte CO_pin = A1;
const byte nrf24Rx = 48;
const byte nrf24Tx = 49;
const byte miso = 50;
const byte mosi = 51;
const byte sck = 52;

//Constants - Thresholds
const int chimney_Over_Temp_Threshold = 550;
const int he_Over_Temp_Threshold = 150;
const byte chimney_Off_Temp_Threshold = 120;
const byte chimney_On_Temp_Threshold = 160;
const int smoke_Threshold = 512;
const int co_Threshold = 512;
const int stokeFanThreshold = 864;	// damperMaxPosition * 0.6;

//Constants - Time
const int readingFrequency = 10000;
const int systemFrequency = 3000;
const unsigned int publishFrequency = 30000;

//Constants - Motor Tunings
const double tempKp = 10, tempKi = 5, tempKd = 2;
const double damperKp = 4, damperKi = 4, damperKd = 1;
const byte cpr = 64;
const double ratio = 131.25;
const byte deadband = 5;
const int damperMaxPosition = 1440;	//360deg/rev * 4 rev

//Variables - Volatile
volatile long encoder0Pos = 0;

//Variables - Inputs - Sensors
double chimneyTemp = 0;
float heatExchangerTemp = 0;
double damperPosition_meas = 0.0;
float downStairsTemp = 60.0f;
float downStairsHumidity = 30.0f;
float upStairsTemp = 60.0f;
float upStairsHumidity = 30.0f;
int smoke = 0;
int carbonMonoxide = 0;

//Variables - Inputs - Commands
byte set_temp = 75;

//Variables - Outputs
bool fans = false;
byte stokeFanPWM = 0;		//speed in % 0-100
bool buzzer = false;
double damperPWM = 0;

//Variable - system
byte state = 0;
byte alarm_num = 0;
double chimneyTempGoal = 60;
double damperCMD = 0.0;		//4 turns @360deg per turn = 1440
unsigned int tmp = 0;
unsigned int Aold = 0;
unsigned int Bnew = 0;

//Variables - Working
byte temp_slack = 3;

//Variables - Timers
unsigned long readTime = 0;
unsigned long stateTime = 0;
unsigned long publishTime = 0;

//function definitions for functions used in setup()
void MQTT_connect();
void setOutputs();
void callback(char* topic, byte* payload, unsigned int length);

//Objects
DHT dht(DHT_Pin, DHTTYPE);
MAX6675 heatExchangerThermocouple(thermoCLK, thermo1CS, thermoSO);
MAX6675 chimneyPipeThermocouple(thermoCLK, thermo2CS, thermoSO);
WiFiEspClient client;
PubSubClient mqttclient(AIO_SERVER, AIO_SERVERPORT, callback, client);
RF24 radio(nrf24Rx, nrf24Tx);
//regulates damper position to try and meet chimney temp goal
PID tempPID(&chimneyTemp, &damperCMD, &chimneyTempGoal, tempKp, tempKi, tempKd, DIRECT);
//regulates damper servo pwm to try and meet damper position command
PID damperPID(&damperPosition_meas, &damperPWM, &damperCMD, damperKp, damperKi, damperKd, DIRECT);

//enums
enum states {
	off_state,
	idle_state,
	heat_state,
	alarm_state,
};

enum alarm_codes {
	no_Alarm,
	chimney_Over_Temp,
	heatExchanger_Over_Temp,
	smoke_present,
	carbonMonoxide_present,
	out_of_fuel,
};

//structs
struct state_params {
	bool fans_cmd;		//low = off, high = on
	double chimneyTemp_cmd;
}off, idle, heat, alarm;

struct COMS_OUT {
	float DS_T;	//4
	float DS_H;	//4
	bool FAN;	//1
	float HE_T;	//4
	float C_T;	//4
	int SMK;	//2
	int CO;		//2
	byte ALM;	//1
	byte ST;	//1
	byte DVP;	//1
	byte SFS;	//1
}GUI_Pub;	//25bytes

struct COMS_IN {
	float US_T;
	float US_H;
	byte SET_T;
}GUI_Sub;