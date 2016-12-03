/*
 Name:		WSC_RemoteUI.ino
 Created:	11/12/2016 6:55:52 PM
 Author:	Travis
*/

#include "WSC_RemoteUI.h"

void setup() {
	Serial.begin(115200);
	printf_begin();
	
	dht.begin();
	setup_watchdog(wdt_8s);

	pinMode(radioIRQ, INPUT_PULLUP);

	// Setup and configure rf radio
	radio.begin();
	//radio.setAutoAck(1);                    // Ensure autoACK is enabled
	//radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 15);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	//radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	readDHT();
	
	myNextion.sendCommand("sleep=0");
	delay(50);
	myNextion.sendCommand("page 0");
	delay(50);
	myNextion.sendCommand("thsp=0");	//set screen sleep on no touch
	//delay(50);
	//myNextion.sendCommand("thup=1");	//screen autowake on touch	
	//delay(50);
}

void loop() {
	//if (wdCount > tempReadCounter) {
	//	readDHT();
	//	//sendData();
	//	wdCount = 0;		
	//}

	GUIInput();	
	
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

void ISR_ScreenWake() {
	Serial.println("Woke from sleep");
}

void do_sleep(void) {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
	sleep_enable();
	attachInterrupt(digitalPinToInterrupt(nextionTx), ISR_ScreenWake, CHANGE);
	WDTCSR |= _BV(WDIE);
	sleep_mode();                        // System sleeps here
										 // The WDT_vect interrupt wakes the MCU from here
	sleep_disable();                     // System continues execution here when watchdog timed out  
	detachInterrupt(digitalPinToInterrupt(nextionTx));
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

void pageUpdate() {
	switch (page) {
	case 1: {
		String update = "set_temp.val=" + String(data.setTemp);
		myNextion.sendCommand(update.c_str());
		//update room temp and humidity
		//update state
		//update date and time
		break;
	}
	case 2: {
		//update state,fire temp, smoke, burn time, fans, air box temp, CO, and # cycles
		break;
	}
	case 3: {
		break;
	}
	case 4: {
		break;
	}
	default:
		break;
	}
}

void GUIInput() {
	String message = myNextion.listen();	
	if (message != "") {
		Serial.println(myNextion.parseString(message));
	}
		
	switch (message[0]) {
	case 0x00:
		if (message[1] == 0xff && message[2] == 0xff && message[3] == 0xff) {
			Serial.println("Invalid Command!");
		}		
		break;
	case 0x01:	
		break;
	case 0x65:
		if (message[1] == 1 && message[2] == 6) {
			data.setTemp++;
			String update = "set_temp.val=" + String(data.setTemp);
			myNextion.sendCommand(update.c_str());				
		} else if (message[1] == 1 && message[2] == 7) {
			data.setTemp--;
			String update = "set_temp.val=" + String(data.setTemp);
			myNextion.sendCommand(update);
		}
		break;
	case 0x66:
		page = message[1];
		Serial.print("page ");
		Serial.println(page);
		pageUpdate();
		break;
	case 0x86:
		Serial.println("Screen went to sleep");
		Serial.println("Going to Sleep");
		delay(50);
		do_sleep();
		break;
	case 0x87:
		Serial.println("Screen woke up");
		break;
	default:
		if (message != "") {
			//Serial.println(message);
			Serial.println(myNextion.parseString(message));
			Serial.println("something else");
		}
		break;
	}
}
