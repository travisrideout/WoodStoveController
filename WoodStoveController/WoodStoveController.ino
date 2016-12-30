/*
 Name:		WoodStoveController.ino
 Created:	11/5/2016 8:20:11 PM
 Author:	Travis Rideout
*/

//TODO: Status LED
//TODO: Reset button
//TODO: Running out of fuel recognization

#include "WoodStoveController.h"

void setup() {
	Serial.begin(115200);
	Serial2.begin(115200);

	dht.begin();
	WiFi.init(&Serial2);

	pinMode(fanRelay_Pin, OUTPUT);
	pinMode(buzzer_Pin, OUTPUT);
	pinMode(damperDir_Pin, OUTPUT);
	pinMode(damperDir_Pin, OUTPUT);
	pinMode(encoder0PinA, INPUT);
	pinMode(encoder0PinB, INPUT);

	digitalWrite(fanRelay_Pin, HIGH);

	Serial.println(F("Wood Stove Controller"));

	// Setup and configure rf radio
	radio.begin();
	radio.setPALevel(RF24_PA_MAX);
	//radio.setAutoAck(1);                    // Ensure autoACK is enabled
	//radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 15);                 // Smallest time between retries, max no. of retries
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	// Connect to WiFi access point.
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(WLAN_SSID);

	WiFi.begin(WLAN_SSID, WLAN_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		WiFi.disconnect();		
		delay(500);
		Serial.print(".");
		WiFi.begin(WLAN_SSID, WLAN_PASS);
	}
	Serial.println();

	Serial.println("WiFi connected");
	Serial.print("IP address: "); 
	Serial.println(WiFi.localIP());

	InitializeStateVariables();

	attachInterrupt(0, doEncoderA, CHANGE);
	attachInterrupt(1, doEncoderB, CHANGE);

	tempPID.SetOutputLimits(0, 1440);
	damperPID.SetOutputLimits(-255, 255);
	
	tempPID.SetMode(AUTOMATIC);
	damperPID.SetMode(AUTOMATIC);
}

void loop() {
	MQTT_connect();
		
	if (millis() - readTime > readingFrequency) {
		readSensors();
		tempPID.Compute();		
		readTime = millis();
		if (safetyCheck() != 0) {
			state = alarm_state;
			Serial.println("Alarm set");
		}
	}

	//Check each changes in position
	if (tmp != encoder0Pos) {
		damperValvePosition_meas = encoder0Pos * 360 / (cpr * ratio);
		tmp = encoder0Pos;
	}
	
	damperServoLoop();
	
	if (millis() - stateTime > systemFrequency) {		
		FSM();
		setOutputs();
		stateTime = millis();
	}
	
	if (millis() - publishTime > publishFrequency) {
		print_debug();
		publish();
		publishTime = millis();
	}

	if (readRF()) {
		sendRF();
	}
	
	mqttclient.loop();
}

//TODO: setup callback to allow changes from MQTT
void callback(char* topic, byte* payload, unsigned int length) {
	Serial.println(topic);
	Serial.write(payload, length);
	Serial.println("");
}

void InitializeStateVariables() {
	off = {
		LOW,	//fans
		60		//chimneyTemp_cmd
	};

	idle = {
		LOW,	//fans
		300		//chimneyTemp_cmd
	};	

	heat = {
		HIGH,	//fans
		450		//chimneyTemp_cmd
	};

	alarm = {
		HIGH,	//fans
		60		//chimneyTemp_cmd
	};
}

 //Function to connect and reconnect as necessary to the MQTT server.
 //Should be called in the loop function and it will take care of connecting.
void MQTT_connect() {		
	if (mqttclient.connected()) {	// Stop if already connected.
		return;
	}
	
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Lost WiFi, trying to reconnect");
		WiFi.disconnect();
		delay(500);
		WiFi.begin(WLAN_SSID, WLAN_PASS);
	}

	Serial.print("Connecting to MQTT... ");
	if (mqttclient.connect(AIO_SERVER, AIO_USERNAME, AIO_KEY)) {
		Serial.println(F("MQTT Connected"));
	} else {
		Serial.println("Failed to connect to MQTT Server");
		int8_t ret = mqttclient.state();
		Serial.print(F("Connection error: "));
		Serial.println(ret);
		mqttclient.disconnect();
	}		
}

void print_debug() {
	Serial.print(F("UST="));
	Serial.print(upStairsTemp);
	Serial.print(F(",USH="));
	Serial.print(upStairsHumidity);
	Serial.print(F(",DST="));
	Serial.print(downStairsTemp);
	Serial.print(F(",DSH="));
	Serial.print(downStairsHumidity);
	Serial.print(F("FAN="));
	Serial.print(fans);
	Serial.print(F(",HET="));
	Serial.print(heatExchangerTemp);
	Serial.print(F(",CT="));
	Serial.print(chimneyTemp);	
	Serial.print(F(",SMK="));
	Serial.print(smoke);
	Serial.print(F(",CO="));
	Serial.print(carbonMonoxide);
	Serial.print(F(",ALM="));
	Serial.print(alarm_num);
	Serial.print(F(",ST="));
	Serial.print(state);
	Serial.print(F(",DVP="));
	Serial.print(damperValvePosition_meas);
	Serial.print(F(",SFS="));
	Serial.println(stokeFanPWM);
}

bool publish() {
	String string1 = "UST=" + String(upStairsTemp);
	String string2 = ",USH=" + String(upStairsHumidity);
	String string3 = ",DST=" + String(downStairsTemp);
	String string4 = ",DSH=" + String(downStairsHumidity);
	String string5 = ",FAN=" + String(fans);
	String string6 = ",HE_T=" + String(heatExchangerTemp);
	String string7 = ",CT=" + String(chimneyTemp);
	String string8 = ",SMK=" + String(smoke);
	String string9 = ",CO=" + String(carbonMonoxide);
	String string10 = ",ALM=" + String(alarm_num);
	String string11 = ",ST=" + String(state);
	String string12 = ",DVP=" + String(damperValvePosition_meas);
	String string13 = ",SFS=" + String(stokeFanPWM);

	String buf = string1 + string2 + string3 + string4 + string5 + string6
		+ string7 + string8 + string9 + string10 + string11 + string12 + string13;
	char pub[128];
	buf.toCharArray(pub, sizeof(pub));
	Serial.println(pub);

	if (!mqttclient.publish(FEED_PATH, pub)) {
	Serial.println(F("Failed"));
	} else {
	Serial.println(F("OK!"));
	}
}

bool readRF() {
	if (!radio.available()) {
		return false;
	}

	while (radio.available()) {
		radio.read(&GUI_Sub, sizeof(GUI_Sub));
	}

	upStairsTemp = GUI_Sub.US_T;
	upStairsHumidity = GUI_Sub.US_H;
	set_temp = GUI_Sub.SET_T;

	return true;
}

bool sendRF() {
	GUI_Pub.DS_T = downStairsTemp;
	GUI_Pub.DS_H = downStairsHumidity;
	GUI_Pub.HE_T = heatExchangerTemp;
	GUI_Pub.C_T = chimneyTemp;
	GUI_Pub.SMK = smoke;
	GUI_Pub.CO = carbonMonoxide;
	GUI_Pub.ALM = alarm_num;
	GUI_Pub.ST = state;

	radio.stopListening();
	if (!radio.write(&GUI_Pub, sizeof(GUI_Pub))) {
		Serial.println("Failed to send message");
		return false;
	}
	radio.startListening();
	return true;
}

void readSensors() {	
	downStairsHumidity = dht.readHumidity();
	downStairsTemp = dht.readTemperature(true);	//read as fahrenheit (true)
									
	if (isnan(downStairsHumidity) || isnan(downStairsTemp)) {
		Serial.println("Failed to read from DHT sensor!");
	}

	heatExchangerTemp = heatExchangerThermocouple.readFahrenheit();
	chimneyTemp = chimneyPipeThermocouple.readFahrenheit();	

	smoke = analogRead(smoke_Pin);
	carbonMonoxide = analogRead(CO_pin);
}

//check for issues in order of importance
byte safetyCheck() {
	if (smoke > smoke_Threshold) {
		alarm_num = smoke_present;
	}else if (carbonMonoxide > co_Threshold) {
		alarm_num = carbonMonoxide_present;
	}else if (chimneyTemp > chimney_Over_Temp_Threshold) {
		alarm_num = chimney_Over_Temp;
	}else if (heatExchangerTemp > he_Over_Temp_Threshold) {
		alarm_num = heatExchanger_Over_Temp;		
	} else {
		alarm_num = no_Alarm;
	}
	
	return alarm_num;
}

void FSM() {
	if (state != alarm_state) {
		if (chimneyTemp < chimney_Min_Temp_Threshold) {
			state = off_state;
		} else if (upStairsTemp < set_temp) {
			state = heat_state;
		} else if (upStairsTemp > set_temp + temp_slack) {
			state = idle_state;
		}
	}
	
	if (state == alarm_state) {
		fans = alarm.fans_cmd;
		chimneyTempGoal = alarm.chimneyTemp_cmd;
	} else if (state == off_state) {
		fans = off.fans_cmd;
		chimneyTempGoal = off.chimneyTemp_cmd;
	} else if (state == idle_state) {
		fans = idle.fans_cmd;
		chimneyTempGoal = idle.chimneyTemp_cmd;
	} else if (state == heat_state) {
		fans = heat.fans_cmd;
		chimneyTempGoal = heat.chimneyTemp_cmd;
	}	
}

void setOutputs() {
	digitalWrite(fanRelay_Pin, !fans);	//inverted
	analogWrite(damperPWM_Pin, damperPWM);
	stokeFanPWM = map((damperPWM - 123), 0, 122, 0, 255);	//may need to map this differently
	analogWrite(stokeFan_Pin, stokeFanPWM);

	if (state == alarm_state) {
		tone(buzzer_Pin, 4000, 2000);
	}
}

void damperServoLoop() {
	if (abs(damperValvePosition_meas - damperCMD) > deadband) {
		damperPID.Compute();
		if (damperPWM > 0) {
			digitalWrite(damperDir_Pin, HIGH);
		} else {
			digitalWrite(damperDir_Pin, LOW);
		}
		analogWrite(damperPWM_Pin, abs(damperPWM));
	} else {
		analogWrite(damperPWM_Pin, 0);
	}
}

// Interrupt on A changing state
void doEncoderA() {
	Bnew^Aold ? encoder0Pos++ : encoder0Pos--;
	Aold = digitalRead(encoder0PinA);
}

// Interrupt on B changing state
void doEncoderB() {
	Bnew = digitalRead(encoder0PinB);
	Bnew^Aold ? encoder0Pos++ : encoder0Pos--;
}
