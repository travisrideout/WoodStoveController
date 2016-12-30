#pragma once

#include <DHT.h>
#include <RF24_config.h>
#include <RF24.h>
#include <printf.h>
#include <nRF24L01.h>
#include "NextionDisplay.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define DHTTYPE DHT22

//pin declarations
const byte radioIRQ = 3;
const byte nextionRx = 4;
const byte nextionTx = 5;
const byte nrf24Rx = 6;
const byte nrf24Tx = 7;
const byte DHTPIN = 12;
const byte buzzerPin = 13;

//constants
const uint64_t pipes[2] = { 0xffcadbdacfLL, 0x5632867136LL };   
const int tempReadInterval = 60000;
const int screenTimeout = 30000;
const int screenRefreshRate = 1000;

//Objects
SoftwareSerial nextion(nextionRx, nextionTx);
NextionDisplay myNextion(nextion, 9600);
RF24 radio(nrf24Rx, nrf24Tx);
DHT dht(DHTPIN, DHTTYPE);

//Variables
volatile bool wake = false;

byte page = 0;

struct dataPacket {
	float temperature;
	float humidity;
	byte setTemp = 70;
}data;

//Variables - Coms
struct COMS_IN {
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
}WSC_Sub;	//25bytes

struct COMS_OUT {
	float US_T;
	float US_H;
	byte SET_T;
}WSC_Pub;

//timers & counters
byte tempReadCounter = 2;
volatile byte wdCount = 0;
unsigned long refreshTime = 0;

// Sleep declarations
typedef enum { 
	wdt_16ms = 0, 
	wdt_32ms, 
	wdt_64ms, 
	wdt_128ms, 
	wdt_250ms, 
	wdt_500ms, 
	wdt_1s, 
	wdt_2s, 
	wdt_4s, 
	wdt_8s 
} wdt_prescalar_e;

void setup_watchdog(uint8_t prescalar);

//method declarations
void readDHT();
void do_sleep(void);