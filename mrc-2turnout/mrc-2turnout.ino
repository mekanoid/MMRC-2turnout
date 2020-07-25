// ==================================================================================================
//
//  Modulstyrning MRC 2turnout
//  Copyright (C) 2020  Peter Kindström
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//    
//  Last modified: 2020-07-05
//
//  Uses the following libraries
//   PubSubClient by Nick O'Leary - https://pubsubclient.knolleary.net/
//   IotWebConf by Balazs Kelemen - https://github.com/prampec/IotWebConf
//   EasyButton by Evert Arias    - https://github.com/evert-arias/EasyButton
//
// ==================================================================================================

// --------------------------------------------------------------------------------------------------
//  Includes
// --------------------------------------------------------------------------------------------------

// Include all libraries
#include <PubSubClient.h>     // Handle MQTT subcription & publish
#include <IotWebConf.h>       // Handle wifi connection & client settings
#include <EasyButton.h>       // Handle button presses
#include "mrcServo.h"         // Handle turnout servos
#include "mrcStatus.h"        // Handle status LEDs


// Include settings and functions sketch files
#include "mrcSettings.h"
#include "mrcWifi.h"
#include "mrcMqtt.h"


// --------------------------------------------------------------------------------------------------
//  Create objects
// --------------------------------------------------------------------------------------------------

// Create Servo object and assign pin
mrcServo servoVx1(pinVx1Servo);
mrcServo servoVx2(pinVx2Servo);


// Create Status LED object and assign pin
mrcStatus ledVx1Rakt(pinVx1Led1);
mrcStatus ledVx1Turn(pinVx1Led2);


// Create Button object and set initial values
EasyButton vx1button(pinVx1Button, 100, false, true);


// --------------------------------------------------------------------------------------------------
//  Setting things up
// --------------------------------------------------------------------------------------------------
void setup() { 
  if (debug == 1) {Serial.begin(9600);Serial.println("");}

  // ------------------------------------------------------------------------------------------------
  // Button setup (in the EasyButton.h file)

  // Initialize the button
  vx1button.begin();

  // Define callback function to be called when a button is pressed
  vx1button.onPressed(btn1Pressed);


  // ------------------------------------------------------------------------------------------------
  // IotWebConfig setup

  // Initial setup (in the mrcWifi.h file)
  wifiSetup();

  // Set up required URL handlers for the config web pages
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });


  // -------------------------------------------------------------------------------------------------
  // MQTT setup

  // Define topics (in the mrcMqtt.h file)
  mqttSetup();

  // Wait for IotWebServer to start network connection
  delay(2000);

  // Connect to MQTT broker
  int mqttPort = atoi(cfgMqttPort);
  mqttClient.setServer(cfgMqttServer, mqttPort);

  // Define function to handle callbacks
  mqttClient.setCallback(mqttCallback);

  
  // -----------------------------------------------------------------------------------------------
  // Servo setup

  // Set limits of the servo: "xxx.limit(min degree, max degree, time interval in ms)"
  servoVx1.limits(servo1min,servo1max,servo1move,servo1back);
  servoVx2.limits(servo2min,servo2max,servo2move,servo2back);

  // Define which funktion to call when a servo has stopped moving
  servoVx1.onFinished(servo1Finished);
  servoVx2.onFinished(servo2Finished);

}

// --------------------------------------------------------------------------------------------------
//  Run forever... 
// --------------------------------------------------------------------------------------------------
void loop() { 

  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (needMqttConnect) {
    if (mqttConnect()) {
      needMqttConnect = false;
    }
  } else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected())) {
    Serial.println("MQTT reconnect");
    Serial.print("Client connected = ");
    Serial.println(mqttClient.connected());
    mqttConnect();
  }

  // Run repetitive jobs
  mqttClient.loop();            // Wait for incoming MQTT messages
  iotWebConf.doLoop();          // Check for IotWebConfig actions
  vx1button.read();             // Check for button pressed
  servoVx1.loop();              // Check for/perform servo action
  ledVx1Rakt.loop();            // Check for/perform led action
  ledVx1Turn.loop();            // Check for/perform led action
  servoVx2.loop();              // Check for/perform servo action

  // Blink both status leds if we have just started after a power off
  if (hasStarted == 1) {
      if (debug == 1) {Serial.println(dbText+"Start");}
      ledVx1Rakt.blink(1500);     // Set blink rate 1500 ms for status led 1
      ledVx1Turn.blink(1500);     // Set blink rate 1500 ms for status led 2
      hasStarted = 0;             // Don't run this routine again (until next power off/on)
  }
    
} 

// --------------------------------------------------------------------------------------------------
//  Function that gets called when the button is pressed
// --------------------------------------------------------------------------------------------------
void btn1Pressed () {
    if (debug == 1) {Serial.println(dbText+"Button 1 pressed");}

     // Indicate moving turout
     mqttPublish(pubTurnoutState, "moving", RETAIN);

    // Toggle button function
    if (button1State == 0) {
      if (debug == 1) {Serial.println(dbText+"Set to Diverge");}
      servoVx1.through();
      servoVx2.through();
      ledVx1Rakt.blink(500);
      ledVx1Turn.off();
      button1State = 1;
    } else {
      if (debug == 1) {Serial.println(dbText+"Set to Trough");}
      servoVx1.diverge();
      servoVx2.diverge();
      ledVx1Rakt.off();
      ledVx1Turn.blink(500);
      button1State = 0;
    }

}

// --------------------------------------------------------------------------------------------------
//  Function that gets called when servo 1 stops moving
// --------------------------------------------------------------------------------------------------
void servo1Finished () {
  if (debug == 1) {Serial.println(dbText+"Servo 1 finished");}

  // Set LEDs only if both servos are at end position
  if (servoVx2.status() == 0){

    // Check in which position servo is and set status LEDs accordingly
    if (servoVx1.position() == 0) {
      ledVx1Rakt.off();
      ledVx1Turn.on();
      mqttPublish(pubTurnoutState, "diverge", RETAIN);
    } else {
      ledVx1Rakt.on();
      ledVx1Turn.off();
      mqttPublish(pubTurnoutState, "through", RETAIN);
    }

  }
}

// --------------------------------------------------------------------------------------------------
//  Function that gets called when servo 2 stops moving
// --------------------------------------------------------------------------------------------------
void servo2Finished () {
  if (debug == 1) {Serial.println(dbText+"Servo 2 finished");}

  // Set LEDs only if both servos are at end position
  if (servoVx1.status() == 0){
    
    // Check in which position servo is and set status LEDs accordingly
    if (servoVx2.position() == 0) {
      ledVx1Rakt.off();
      ledVx1Turn.on();
      mqttPublish(pubTurnoutState, "diverge", RETAIN);
    } else {
      ledVx1Rakt.on();
      ledVx1Turn.off();
      mqttPublish(pubTurnoutState, "through", RETAIN);
    }
  }
}

// --------------------------------------------------------------------------------------------------
// Function to handle MQTT messages sent to this device
// --------------------------------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Don't know why this have to be done :-(
  payload[length] = '\0';

  // Make strings
  String msg = String((char*)payload);
  String tpc = String((char*)topic);

  // Print the topic and payload
  if (debug == 1) {Serial.println(dbText+"Recieved: "+tpc+" = "+msg);}

  // Check for "turnout/direction/set" command
  if (tpc == subTopic[0]) {
    if (msg == "toggle") {
      btn1Pressed();
    }
  }
  
}

// --------------------------------------------------------------------------------------------------
//  Function that gets called when IotWebConf web page config has been saved
// --------------------------------------------------------------------------------------------------
void configSaved()
{
  if (debug == 1) {Serial.println(dbText+"IotWebConf config saved");}
  deviceID = String(cfgDeviceId);
  deviceName = String(cfgDeviceName);
  
  servo1min = atoi(cfgServo1Min);
  servo1max = atoi(cfgServo1Max);
  servo1move = atoi(cfgServo1Move);
  servo1back = atoi(cfgServo1Back);
  servoVx1.limits(servo1min,servo1max,servo1move,servo1back);

  servo2min = atoi(cfgServo2Min);
  servo2max = atoi(cfgServo2Max);
  servo2move = atoi(cfgServo2Move);
  servo2back = atoi(cfgServo2Back);
  servoVx2.limits(servo2min,servo2max,servo2move,servo2back);

}
