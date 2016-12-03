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

#define DHTTYPE DHT11

#define WLAN_SSID       "Rideout_2.4GHz"
#define WLAN_PASS       "kj6608td"
#define WLAN_SECURITY	WLAN_SEC_WEP
#define AIO_USERNAME    "TravisRideout"
#define AIO_KEY         "c490163b9b074329ba35cbe30bf6152e"
#define FEED_PATH AIO_USERNAME "/feeds/woodstove"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL

//pin declarations
const int nrfIntPin = 2;	//interrupt pins = 2,3,18,19,20,21
const int fanRelay = 4;
const int DHTPIN = 5;
const int thermoCLK = 6;
const int thermoSO = 7;
const int thermo1CS = 8;
const int thermo2CS = 9;
const int buzzer_pin = 10;
const int smokePin = A0;
const int CO_pin = A1;

//Constants - Thresholds
const int chimney_Over_Temp_Threshold = 500;
const int he_Over_Temp_Threshold = 150;
const byte chimney_Min_Temp_Threshold = 200;
const int smoke_Threshold = 512;
const int co_Threshold = 512;

//Constants - Time
const int readingFrequency = 10000;
const int systemFrequency = 3000;
const unsigned int publishFrequency = 3000;

//Variables - Inputs - Sensors
double chimneyTemp = 0;
double heatExchangerTemp = 0;
byte stokeFanSpeed_meas;
byte damperValvePosition_meas;
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
byte stokeFanSpeed = 0;		//speed in % 0-100
byte damperValvePosition = 0; //position in % 0-100
bool buzzer = false;

//Variable - system
byte state = 0;
byte alarm_num = 0;
byte heTempGoal = 30;

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
DHT dht(DHTPIN, DHTTYPE);
MAX6675 heatExchangerThermocouple(thermoCLK, thermo1CS, thermoSO);
MAX6675 chimneyPipeThermocouple(thermoCLK, thermo2CS, thermoSO);
WiFiEspClient client;
PubSubClient mqttclient(AIO_SERVER, AIO_SERVERPORT, callback, client);

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
	carbonMonoxide_present
};

//structs
struct state_params {
	bool fans_cmd;		//low = off, high = on
	double heatExchangerTemp_cmd;
}off, idle, heat, alarm;