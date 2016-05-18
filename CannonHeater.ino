/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
 * http://www.mysensors.org/build/temp
 */


// Enable debug prints to serial monitor
#define MY_DEBUG 


#define MY_NODE_ID 3

#define MY_DEFAULT_TX_LED_PIN A0
#define MY_DEFAULT_RX_LED_PIN A1
#define MY_DEFAULT_ERR_LED_PIN A2
#define MY_RF24_CE_PIN 8
#define MY_RF24_CS_PIN 9



// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define CHILD_ID 1   // Id of the sensor child

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
#define ONE_WIRE_BUS A4 // Pin where dallas sensor is connected 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempSensor;

float receivedvalue;
float setPoint = 27;
bool heaterStatus = 0;
int gasStatus = 0;
int fanStatus = 0;


// Initialize temperature message
//MyMessage msg(0,V_TEMP);

MyMessage tempSensorMsg(CHILD_ID, V_TEMP);
MyMessage heaterStatusMsg(CHILD_ID, V_HVAC_FLOW_STATE);


//Relay Pins
#define PWR_RLY 4
#define GAS_RLY 5
#define MID_RLY 6
#define HIGH_RLY 7

#define RELAY_ON 1
#define RELAY_OFF 0

int statuscode; // a number for each of the above 0-9
char* strStatus[] = {
	"Off","Low","Med","High", "VHigh" };


long counter;


void setup()  
{ 
	//Intiallise the Dallas sensors
	sensors.begin();
	sensors.getAddress(tempSensor, 0);  //first sensor on bus
	sensors.setResolution(tempSensor, 10); //0.25 degC resolution.


  //setup pins
  digitalWrite(PWR_RLY, RELAY_OFF);
  pinMode(PWR_RLY, OUTPUT);
  digitalWrite(GAS_RLY, RELAY_OFF);
  pinMode(GAS_RLY, OUTPUT);
  digitalWrite(MID_RLY, RELAY_OFF);
  pinMode(MID_RLY, OUTPUT);
  digitalWrite(HIGH_RLY, RELAY_OFF);
  pinMode(HIGH_RLY, OUTPUT);

}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Canon Heater", "1.7");
  
  //Describe the device type...probably want this to be HVAC??
  present(CHILD_ID, S_HVAC);
   


}

void loop()     
{     
	  counter++ ;
	  if (counter >100000) {
		  Serial.println("Reading Temp");
		  getTemp();
		  send(heaterStatusMsg.set(strStatus[statuscode])); //this sends the MODE back to the controller. Probably need to send this when the states change too. This currently isnt processed by Openhab

		  
		  counter = 0;
	  };




}

void receive(const MyMessage &message) {
	// We only expect one type of message from controller. But we better check anyway.
	if (message.isAck()) {
		Serial.println("This is an ack from gateway");
	}

	//Not using this currently but this reads the desired temp to set the thermostat to. Would need to build the logic.
	// i guess the logic would be to turn down the heater when the desired temp level is reached.
	// as temp sensor is a bit close i d want to use a separate sensor....
		if (message.type == V_HVAC_SETPOINT_HEAT){
			receivedvalue = String(message.data).toFloat();

			if (receivedvalue > 0)	{
			Serial.println(String(message.data));
			setPoint = receivedvalue;
			Serial.println("New SetPoint Temp recieved: " + String(setPoint));
		//	gw.send(msgSetpoint.set(setPoint, 1));
			//Store state in eeprom
			//gw.saveState(CHILD_ID_HEATER, setPoint);
			//updateSetTemp();
		}
	}
	
	if ((message.sensor == CHILD_ID) && (message.type == V_HVAC_FLOW_STATE)) { //if the Vera switch is ON then the pumps must be running as the pumps will update the status so any message here would mean the over-ride is wanted

		String heatLvl = (message.data);

		if (heatLvl == "Off") {
			Serial.print("Set heater to OFF !!!!!! :");
			setHeaterOff();
			statuscode = 0;
		}


		if (heatLvl == "Low") {
			Serial.print("Set heater LOW/LOW :");
			setHeaterOn();
			setFanLow();
			setGasLow();
			statuscode = 1;
		}
		if (heatLvl == "Med") {
			Serial.print("Set heater MID/LOW :");
			setHeaterOn();
			setFanMid();
			setGasLow();
			statuscode = 2;

		}
		if (heatLvl == "High") {
			Serial.print("Set heater MID/HIGH :");
			setHeaterOn();
			setFanMid();
			setGasHigh();
			statuscode = 3;
		}
		if (heatLvl == "VHigh") {
			Serial.print("Set heater HIGH/HIGH :");
			setHeaterOn();
			setFanHigh();
			setGasHigh();
			statuscode = 4;
		}


	}

/*
	// Fan Speed and Gas level control MQTT mygateway1-in/5/1/1/0/22 - data 1 to 4 
	if ((message.sensor == CHILD_ID) && (message.type == V_HVAC_SPEED)) { //if the Vera switch is ON then the pumps must be running as the pumps will update the status so any message here would mean the over-ride is wanted
																	  //if (message.header.messageType=M_SET_VARIABLE &&
																	  //      message.header.type==V_HEATER) {
		Serial.println("some v_heater data came");
		Serial.println(String(message.data)); //Print out the data of the msg	

		int heatLvl = atoi(message.data);
		//String heatLvl = (message.data);
			
			
			if (heatLvl == 0) {
				Serial.print("Set heater to OFF !!!!!! :");
				setHeaterOff();
				statuscode = 0;
			}


			if (heatLvl == 1) {
				Serial.print("Set heater LOW/LOW :");
				setHeaterOn();
				setFanLow();
				setGasLow();
				statuscode = 1;
			}
		if (heatLvl == 2) {
			Serial.print("Set heater MID/LOW :");
			setHeaterOn();
			setFanMid();
			setGasLow();
			statuscode = 2;
			
		}
		if (heatLvl == 3) {
			Serial.print("Set heater MID/HIGH :");
			setHeaterOn();
			setFanMid();
			setGasHigh();
			statuscode = 3;
		}
		if (heatLvl == 4) {
			Serial.print("Set heater HIGH/HIGH :");
			setHeaterOn();
			setFanHigh();
			setGasHigh();
			statuscode = 4;
		}
	}

	*/

}


void setHeaterOff() {
	Serial.println("Setting Heater OFF");
	digitalWrite(MID_RLY, RELAY_OFF); //also need to turn off the Mid speed relay
	digitalWrite(PWR_RLY, RELAY_OFF);
	//   gw.sendVariable(CHILD_ID, V_HEATER, "Off");  //report back to Vera
	//send(heaterStatusMsg.set((const char*) "Off"));
	heaterStatus = 0;
}

void setHeaterOn() {
	Serial.println("Setting Heater ON");
	digitalWrite(PWR_RLY, RELAY_ON);
	//   gw.sendVariable(CHILD_ID, V_HEATER, "HeatOn");  //report back to Vera
	//send(heaterStatusMsg.set((const char*) "HeatOn"));
	//gw.send(tempSensorMsg.set(averageTemp, 1));
	heaterStatus = 1;
}

void setGasLow() {
	digitalWrite(GAS_RLY, RELAY_OFF);
	Serial.println("Setting Gas LOW");
	gasStatus = 0;
}

void setGasHigh() {
	Serial.println("Setting Gas HIGH");
	digitalWrite(GAS_RLY, RELAY_ON);
	gasStatus = 1;
}

void setFanLow() {
	digitalWrite(MID_RLY, RELAY_OFF);
	digitalWrite(HIGH_RLY, RELAY_OFF);
	Serial.println("Setting Fan LOW");
	fanStatus = 0;
}

void setFanMid() {
	digitalWrite(MID_RLY, RELAY_ON);
	digitalWrite(HIGH_RLY, RELAY_OFF);
	Serial.println("Setting Fan MID");
	fanStatus = 1;
}

void setFanHigh() {
	digitalWrite(MID_RLY, RELAY_ON);
	digitalWrite(HIGH_RLY, RELAY_ON);
	Serial.println("Setting Fan HIGH");
	fanStatus = 2;
}




void getTemp() {
	//get temp and send to Heater device
	sensors.requestTemperatures(); // Fetch temperatures from Dallas
	float heaterTemp = (sensors.getTempC(tempSensor) * 10.)/10.;
	send(tempSensorMsg.set(heaterTemp, 1));
	Serial.println(heaterTemp);
	
}