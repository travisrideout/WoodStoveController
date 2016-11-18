#pragma once

#include <DHT.h>
#include <RF24_config.h>
#include <RF24.h>
#include <printf.h>
#include <nRF24L01.h>
#include <Nextion.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define DHTTYPE DHT22

//pin declarations
const byte buttonPin = 2;
const byte radioIRQ = 3;
const byte nextionRx = 4;
const byte nextionTx = 5;
const byte nrf24Rx = 6;
const byte nrf24Tx = 7;
const byte displayPowerPin = 8;
const byte DHTPIN = 12;
const byte buzzerPin = 13;

//constants
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };   // Radio pipe addresses for the 2 nodes to communicate.
const int tempReadInterval = 60000;
const int screenTimeout = 30000;

//Objects
SoftwareSerial nextion(nextionRx, nextionTx);
Nextion myNextion(nextion, 9600);
RF24 radio(nrf24Rx, nrf24Tx);
DHT dht(DHTPIN, DHTTYPE);

//Variables
volatile bool wake = false;
volatile byte wdCount = 0;
bool screenOn = false;
byte page = 0;
struct dataPacket {
	float temperature;
	float humidity;
	byte setTemp = 70;
}data;

struct feedback {
	byte state = 0;
}info;

//timers & counters
byte tempReadCounter = 2;
unsigned long screenTimeoutTimer;

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


