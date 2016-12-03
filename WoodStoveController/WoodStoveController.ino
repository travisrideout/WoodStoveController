/*
 Name:		WoodStoveController.ino
 Created:	11/5/2016 8:20:11 PM
 Author:	Travis Rideout
*/

//TODO: Status LED
//TODO: Reset button

#include "WoodStoveController.h"

void setup() {
	Serial.begin(115200);
	Serial2.begin(115200);

	dht.begin();
	WiFi.init(&Serial2);

	pinMode(fanRelay, OUTPUT);
	pinMode(buzzer_pin, OUTPUT);

	digitalWrite(fanRelay, HIGH);

	Serial.println(F("Wood Stove Controller"));

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
}

void loop() {
	//if change in set temp then change state
	//get room temps
	//if any alarms then off state and send alarm notification
	//if trying to heat and firebox temp not going up then out of fuel

	MQTT_connect();
	
	if (millis() - readTime > readingFrequency) {
		readSensors();
		readTime = millis();
		if (safetyCheck() != 0) {
			state = alarm_state;
			Serial.println("Alarm set");
		}
	}
	
	if (millis() - stateTime > systemFrequency) {		
		FSM();
		setOutputs();
		stateTime = millis();

		Serial.print("state: ");
		Serial.println(state);
	}
	
	if (millis() - publishTime > publishFrequency) {
		print_debug();
		//publish();
		publishTime = millis();
	}
	
	mqttclient.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.println(topic);
	Serial.write(payload, length);
	Serial.println("");
}

void InitializeStateVariables() {
	off = {
		LOW,	//fans
		30		//heatExchangerTemp_cmd
	};

	idle = {
		LOW,	//fans
		80		//heatExchangerTemp_cmd
	};	

	heat = {
		HIGH,	//fans
		120		//heatExchangerTemp_cmd
	};

	alarm = {
		LOW,	//fans
		30		//heatExchangerTemp_cmd
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
	Serial.print(F("US_T="));
	Serial.print(upStairsTemp);
	Serial.print(F(",US_H="));
	Serial.print(upStairsHumidity);
	Serial.print(F(",DS_T="));
	Serial.print(downStairsTemp);
	Serial.print(F(",DS_H="));
	Serial.print(downStairsHumidity);
	Serial.print(F("FAN="));
	Serial.print(fans);
	Serial.print(F(",HE_T="));
	Serial.print(heatExchangerTemp);
	Serial.print(F(",C_T="));
	Serial.print(chimneyTemp);	
	Serial.print(F(",SMK="));
	Serial.print(smoke);
	Serial.print(F(",CO="));
	Serial.print(carbonMonoxide);
	Serial.print(F(",ALM="));
	Serial.println(alarm_num);
}

bool publish() {
	String string1 = "US_T=" + String(upStairsTemp);
	String string2 = ",US_H=" + String(upStairsHumidity);
	String string3 = ",DS_T=" + String(downStairsTemp);
	String string4 = ",DS_H=" + String(downStairsHumidity);
	String string5 = "FAN=" + String(fans);
	String string6 = ",HE_T=" + String(heatExchangerTemp);
	String string7 = ",C_T=" + String(chimneyTemp);
	String string8 = ",SMK=" + String(smoke);
	String string9 = ",CO=" + String(carbonMonoxide);
	String string10 = ",ALM=" + String(alarm_num);

	String buf = string1 + string2 + string3 + string4 + string5 + string6
		+ string7 + string8;
	char pub[128];
	buf.toCharArray(pub, sizeof(pub));
	Serial.println(pub);

	if (!mqttclient.publish(FEED_PATH, pub)) {
	Serial.println(F("Failed"));
	} else {
	Serial.println(F("OK!"));
	}
}

void readSensors() {	
	downStairsHumidity = dht.readHumidity();
	downStairsTemp = dht.readTemperature(true);	//read as fahrenheit (true)
									
	if (isnan(downStairsHumidity) || isnan(downStairsTemp)) {
		Serial.println("Failed to read from DHT sensor!");
	}

	heatExchangerTemp = heatExchangerThermocouple.readFahrenheit();
	chimneyTemp = chimneyPipeThermocouple.readFahrenheit();	

	smoke = analogRead(smokePin);
	carbonMonoxide = analogRead(CO_pin);
}

//check for issues in order of importance
byte safetyCheck() {
	if (smoke > smoke_Threshold) {
		alarm_num = smoke_present;
		return alarm_num;
	}
	if (carbonMonoxide > co_Threshold) {
		alarm_num = carbonMonoxide_present;
		return alarm_num;
	}
	if (chimneyTemp > chimney_Over_Temp_Threshold) {
		alarm_num = chimney_Over_Temp;
		return alarm_num;
	}
	if (heatExchangerTemp > he_Over_Temp_Threshold) {
		alarm_num = heatExchanger_Over_Temp;
		return alarm_num;
	}	
	
	alarm_num = no_Alarm;
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
		heTempGoal = alarm.heatExchangerTemp_cmd;
	} else if (state == off_state) {
		fans = off.fans_cmd;
		heTempGoal = off.heatExchangerTemp_cmd;
	} else if (state == idle_state) {
		fans = idle.fans_cmd;
		heTempGoal = idle.heatExchangerTemp_cmd;
	} else if (state == heat_state) {
		fans = heat.fans_cmd;
		heTempGoal = heat.heatExchangerTemp_cmd;
	}	
}

void setOutputs() {
	digitalWrite(fanRelay, !fans);	//inverted

	if (state == alarm_state) {
		tone(buzzer_pin, 4000, 2000);
	}

	//TODO: PID loop on damper valve to reach HE temp
	//TODO: stoke fan speed = 0-100% over damper position 30-100%
}