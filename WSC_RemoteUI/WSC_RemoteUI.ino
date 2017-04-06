/*
 Name:		WSC_RemoteUI.ino
 Created:	11/12/2016 6:55:52 PM
 Author:	Travis
*/

#include "WSC_RemoteUI.h"

void setup() {
	Serial.begin(115200);
		
	dht.begin();
	setup_watchdog(wdt_8s);

	pinMode(radioIRQ, INPUT_PULLUP);
	pinMode(buzzerPin, OUTPUT);

	// Setup and configure rf radio
	printf_begin();
	radio.begin();
	radio.setPALevel(RF24_PA_MAX);
	//radio.setAutoAck(1);                    // Ensure autoACK is enabled
	//radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 15);
	radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[1]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging
		
	myNextion.sendCommand("sleep=0");
	delay(50);
	myNextion.sendCommand("page 0");
	delay(50);
	myNextion.sendCommand("thsp=30");	//set screen sleep on no touch after x sec
	delay(50);
	myNextion.sendCommand("thup=1");	//screen autowake on touch	
	delay(50);

	readDHT();
}

void loop() {
	if (wdCount > tempReadCounter) {
		readDHT();		
		sendRF();
		wdCount = 0;	
		if (awake) {
			pageUpdate();
		}
	}

	if (awake) {
		if (millis() - refreshTime > screenRefreshRate) {
			refreshTime = millis();
			sendRF();
		}

		if (readRF()) {
			pageUpdate();
		}
	} else {
		delay(50);
		do_sleep();
	}

	GUIInput();
	readRF();
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
	Serial.println("Going to sleep");
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
	sleep_enable();
	attachInterrupt(digitalPinToInterrupt(nextionTx), ISR_ScreenWake, CHANGE);
	WDTCSR |= _BV(WDIE);
	sleep_mode();                        // System sleeps here
										 // The WDT_vect interrupt wakes the MCU from here
	sleep_disable();                     // System continues execution here when watchdog timed out  
	detachInterrupt(digitalPinToInterrupt(nextionTx));	
	WDTCSR &= ~_BV(WDIE);
}

void readDHT() {	
	data.humidity = dht.readHumidity();
	data.temperature = dht.readTemperature(true);

	if (isnan(data.humidity) || isnan(data.temperature)) {
		Serial.println("Failed to read from DHT sensor!");
	} else {
		Serial.print("Read from DHT sensor: ");
		Serial.print(data.temperature);
		Serial.print(", ");
		Serial.println(data.humidity);
	}	
}

bool sendRF() {
	//radio.powerUp(); 
	//delay(5);
	WSC_Pub.SET_T = data.setTemp;
	WSC_Pub.US_T = data.temperature;
	WSC_Pub.US_H = data.humidity;

	radio.stopListening();
	if (!radio.write(&WSC_Pub, sizeof(WSC_Pub))) {
		Serial.println("Failed to send message");
		radio.startListening();
		return false;
	}
	radio.startListening();
	//TODO: Decide whether GUI should provide warnings, if so how to handle them and manage battery
	//radio.powerDown();

	Serial.println("Sent Radio Message");	
	return true;
}

bool readRF() {
	if (!radio.available()) {
		return false;
	}

	Serial.println("Got Radio message");
	while (radio.available()) {
		radio.read(&WSC_Sub, sizeof(WSC_Sub));
	}
	
	return true;
}

bool establishConnection() {
	Serial.println("Waiting for connection");
	int count = 0;
	while (!sendRF()) {
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
		update = "temp.txt=\"" + String(data.temperature) + "\"";
		myNextion.sendCommand(update.c_str());
		update = "humidity.val=" + String((byte)data.humidity);
		myNextion.sendCommand(update.c_str());
		if (WSC_Sub.ST == 0) {
			update = String("state.txt=\"OFF\"");
		} else if (WSC_Sub.ST == 1) {
			update = String("state.txt=\"IDLE\"");
		} else if (WSC_Sub.ST == 2) {
			update = String("state.txt=\"HEATING\"");
		} else if (WSC_Sub.ST == 3) {
			update = String("state.txt=\"ALARM\"");
		}
		break;
	}
	case 2: {
		String update;
		if (WSC_Sub.ST == 0) {
			update = String("state.txt=\"OFF\"");
		} else if (WSC_Sub.ST == 1) {
			update = String("state.txt=\"IDLE\"");
		} else if (WSC_Sub.ST == 2) {
			update = String("state.txt=\"HEATING\"");
		} else if (WSC_Sub.ST == 3) {
			update = String("state.txt=\"ALARM\"");
		}
		myNextion.sendCommand(update.c_str()); 
		if (WSC_Sub.FAN == 0) {
			update = "fans.txt=\"OFF\"";
		} else {
			update = "fans.txt=\"ON\"";
		}
		myNextion.sendCommand(update.c_str());
		update = "chimney_temp.txt=\"" + String(WSC_Sub.C_T) +"\"";
		myNextion.sendCommand(update.c_str());
		update = "he_temp.txt=\"" + String(WSC_Sub.HE_T) + "\"";
		myNextion.sendCommand(update.c_str());
		update = "smoke.txt=\"" + String(WSC_Sub.SMK) + "\"";
		myNextion.sendCommand(update.c_str());
		update = "co.txt=\"" + String(WSC_Sub.CO) + "\"";
		myNextion.sendCommand(update.c_str());
		update = "stoke.txt=\"" + String(WSC_Sub.SFS) + "\"";
		myNextion.sendCommand(update.c_str());
		update = "damper.txt=\"" + String(WSC_Sub.DVP) + "\"";
		myNextion.sendCommand(update.c_str());
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

bool GUIInput() {
	String message = myNextion.listen();	
	if (message == "") {
		return false;
	} else {
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
		} else if (message[1] == 1 && message[2] == 7) {
			data.setTemp--;
		}
		pageUpdate();
		break;
	case 0x66:
		if (page != message[1]) {
			page = message[1];
			Serial.print("page ");
			Serial.println(page);
			pageUpdate();
		}		
		break;
	case 0xff86:		
		Serial.println("Going to Sleep");
		awake = false;
		delay(50);
		do_sleep();
		break;
	case 0xff87:
		Serial.println("Screen woke up");
		awake = true;
		delay(50);
		myNextion.sendCommand("page 0");
		break;
	default:
		if (message != "") {
			Serial.println(myNextion.parseString(message));
			Serial.println("something else");
		}
		break;
	}	
	return true;
}
