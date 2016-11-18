/*
 Name:		WSC_RemoteUI.ino
 Created:	11/12/2016 6:55:52 PM
 Author:	Travis
*/

#include "WSC_RemoteUI.h"

// the setup function runs once when you press reset or power the board

void setup() {
	Serial.begin(115200);
	printf_begin();

	myNextion.init();
	dht.begin();
	setup_watchdog(wdt_8s);

	pinMode(buttonPin, INPUT_PULLUP);
	pinMode(radioIRQ, INPUT_PULLUP);
	pinMode(displayPowerPin, OUTPUT);
	pinMode(buttonPin, OUTPUT);

	// Setup and configure rf radio
	radio.begin();
	//radio.setAutoAck(1);                    // Ensure autoACK is enabled
	//radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 15);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	readDHT();
	screenTimeoutTimer = millis();
	digitalWrite(displayPowerPin, HIGH);
	screenOn = true;
		
	myNextion.setComponentText("t5", "Idle");
}

void loop() {
	if (wake == true) {
		wake = false;
		screenTimeoutTimer = millis();
		digitalWrite(displayPowerPin, HIGH);
		screenOn = true;
		Serial.println("Turning on Screen");
	}

	if (wdCount > tempReadCounter) {
		readDHT();
		//sendData();
		wdCount = 0;		
	}

	if (screenOn) {
		GUIInput();
	}
	
	if (millis() - screenTimeoutTimer > screenTimeout) {
		digitalWrite(displayPowerPin, LOW);
		screenOn = false;
		Serial.println("Going to Sleep");
		delay(50);
		do_sleep();
	}
}

void ISR_PowerButton() {
	wake = true;
	sleep_disable();
}

void setup_watchdog(uint8_t prescalar) {
	uint8_t wdtcsr = prescalar & 7;
	if (prescalar & 8)
		wdtcsr |= _BV(WDP3);
	MCUSR &= ~_BV(WDRF);                      // Clear the WD System Reset Flag
	WDTCSR = _BV(WDCE) | _BV(WDE);            // Write the WD Change enable bit to enable changing the prescaler and enable system reset
	WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);  // Write the prescalar bits (how long to sleep, enable the interrupt to wake the MCU
}

ISR(WDT_vect) {
	wdCount++;	
}

void do_sleep(void) {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
	sleep_enable();
	attachInterrupt(digitalPinToInterrupt(buttonPin), ISR_PowerButton, FALLING);
	WDTCSR |= _BV(WDIE);
	sleep_mode();                        // System sleeps here
										 // The WDT_vect interrupt wakes the MCU from here
	sleep_disable();                     // System continues execution here when watchdog timed out  
	detachInterrupt(digitalPinToInterrupt(buttonPin));
	page = 0;
	WDTCSR &= ~_BV(WDIE);
}

void readDHT() {	
	data.humidity = dht.readHumidity();
	data.temperature = dht.readTemperature(true);

	// Check if any reads failed
	if (isnan(data.humidity) || isnan(data.temperature)) {
		Serial.println("Failed to read from DHT sensor!");
	}
}

bool sendData() {
	radio.powerUp();                                
	radio.stopListening();
	if (!radio.write(&data, sizeof(data))) {
		return false;
	}
	//TODO: Decide whether GUI should provide warnings, if so how to handle them and manage battery
	//radio.startListening();
	radio.powerDown();
	return true;
}

bool establishConnection() {
	Serial.println("Waiting for connection");
	int count = 0;
	while (!sendData()) {
		if (count < 20) {
			Serial.print(".");
			count++;
		} else {
			count = 0;
			Serial.println("!");
		}
		delay(3000);
	}
	Serial.println(" ");
	Serial.println("Connection Established");
	return true;
}

void getPage() {
	byte page = 255;
	while (page == 255) {
		page = myNextion.pageId();
	}
	Serial.print("page ");
	Serial.println(page);
}

void GUIInput() {
	String message = myNextion.listen();	
	if (message != "") {
		Serial.println(message);
		screenTimeoutTimer = millis();
	}	
	if (message == "65 1 6 1 ffff ffff ffff") {
		data.setTemp++;
		myNextion.setComponentValue("set_temp", data.setTemp);		
	}
	if (message == "65 1 7 1 ffff ffff ffff") {
		data.setTemp--;
		myNextion.setComponentValue("set_temp", data.setTemp);
	}
	if (message == "66 0 ffff ffff ffff") {
		page = 0;
	}
	if (message == "66 1 ffff ffff ffff") {
		page = 1;
	}
	if (message == "66 2 ffff ffff ffff") {
		page = 2;
	}
	if (message == "66 3 ffff ffff ffff") {
		page = 3;
	}
	if (message == "66 4 ffff ffff ffff") {
		page = 4;
	}
	
	/*if (myNextion.getComponentText("t5") != 0x70 + (String)"Idle") {
		myNextion.setComponentText("t5", "Idle");
		Serial.println("setting Idle Text");
		delay(100);
	}*/	
}
