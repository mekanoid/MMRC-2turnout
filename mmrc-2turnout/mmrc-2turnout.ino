//
//  Modulstyrning MMRC 2turnout
//  2019 Peter Kindström
//  This code is in the public domain
//
//  Last modified: 2019-12-26
//
//  Uses the following libraries
//   PubSubClient by Nick O'Leary - https://pubsubclient.knolleary.net/
//   IotWebConf by Balazs Kelemen - https://github.com/prampec/IotWebConf
//   EasyButton by Evert Arias    - https://github.com/evert-arias/EasyButton
//

// Include all libraries
#include <PubSubClient.h>
#include <IotWebConf.h>       // Take care of wifi connection & client settings
#include <EasyButton.h>       // Handle button presses
#include "mmrcServo.h"        // Handle turnout servos
#include "mmrcStatus.h"       // Handle status LEDs

WiFiClient wifiClient;            // Wifi initialisation
PubSubClient mqttClient(wifiClient);

// --------------------------------------------------------------------------------------------------
// IotWebConf variables

// Default string and number lenght
#define STRING_LEN 32
#define NUMBER_LEN 8

// Access point configuration
const char thingName[] = "MMRC-client";               // Initial AP name, used as SSID of the own Access Point
const char wifiInitialApPassword[] = "MmrcIsBest";    // Initial password, used when it creates an own Access Point

// Device configuration
char cfgMqttServer[STRING_LEN];
char cfgMqttPort[NUMBER_LEN];
char cfgDeviceId[STRING_LEN];
char cfgDeviceName[STRING_LEN];

// Node configuration
char cfgServo1Min[NUMBER_LEN];
char cfgServo1Max[NUMBER_LEN];
char cfgServo1Move[NUMBER_LEN];
char cfgServo1Back[NUMBER_LEN];
char cfgServo2Min[NUMBER_LEN];
char cfgServo2Max[NUMBER_LEN];
char cfgServo2Move[NUMBER_LEN];
char cfgServo2Back[NUMBER_LEN];

// Indicate if it is time to reset the client or connect to MQTT
boolean needMqttConnect = false;
// boolean needReset = false;

// When CONFIG_PIN is pulled to ground on startup, the client will use the initial
// password to buld an AP. (E.g. in case of lost password)
// #define CONFIG_PIN D8

// Status indicator pin.
// First it will light up (kept LOW), on Wifi connection it will blink
// and when connected to the Wifi it will turn off (kept HIGH).
// #define STATUS_PIN LED_BUILTIN


// -------------------------------------------------------------------------------------------------
// IotWebConf configuration

// Callback method declarations
void wifiConnected();
void configSaved();

// Start DNS, Webserver & IotWebConf
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

// Define settings to show up on configuration web page - Device
IotWebConfParameter webDeviceId = IotWebConfParameter("Enhetens unika id", "deviceId", cfgDeviceId, STRING_LEN);
IotWebConfParameter webDeviceName = IotWebConfParameter("Enhetens namn", "deviceName", cfgDeviceName, STRING_LEN);

// Add your own configuration - MQTT
IotWebConfParameter webMqttServer = IotWebConfParameter("MQTT IP-adress", "mqttServer", cfgMqttServer, STRING_LEN);
IotWebConfParameter webMqttPort = IotWebConfParameter("MQTT-port", "mqttPort", cfgMqttPort, NUMBER_LEN);

// Separator with text
IotWebConfSeparator separator1 = IotWebConfSeparator("Inst&auml;llningar f&ouml;r Servo 1");

// Define settings to show up on configuration web page - Servo 1
IotWebConfParameter webServo1Min = IotWebConfParameter("Servo 1 min", "servo1min", cfgServo1Min, NUMBER_LEN);
IotWebConfParameter webServo1Max = IotWebConfParameter("Servo 1 max", "servo1max", cfgServo1Max, NUMBER_LEN);
IotWebConfParameter webServo1Move = IotWebConfParameter("Servo 1 hastighet", "servo1move", cfgServo1Move, NUMBER_LEN);
IotWebConfParameter webServo1Back = IotWebConfParameter("Servo 1 back", "servo1back", cfgServo1Back, NUMBER_LEN);

// Separator with text
IotWebConfSeparator separator2 = IotWebConfSeparator("Inst&auml;llningar f&ouml;r Servo 2");

// Define settings to show up on configuration web page - Servo 2
IotWebConfParameter webServo2Min = IotWebConfParameter("Servo 2 min", "servo2min", cfgServo2Min, NUMBER_LEN);
IotWebConfParameter webServo2Max = IotWebConfParameter("Servo 2 max", "servo2max", cfgServo2Max, NUMBER_LEN);
IotWebConfParameter webServo2Move = IotWebConfParameter("Servo 2 hastighet", "servo2move", cfgServo2Move, NUMBER_LEN);
IotWebConfParameter webServo2Back = IotWebConfParameter("Servo 2 back", "servo2back", cfgServo2Back, NUMBER_LEN);

// -------------------------------------------------------------------------------------------------
// Define topic variables

// Variable for topics to subscribe to
const int nbrSubTopics = 1;
String subTopic[nbrSubTopics];

// Variable for topics to publish to
const int nbrPubTopics = 7;
String pubTopic[nbrPubTopics];
String pubTopicContent[nbrPubTopics];

// Often used topics
String pubTurnoutState;
String pubDeviceStateTopic;

const byte NORETAIN = 0;      // Used to publish topics as NOT retained
const byte RETAIN = 1;        // Used to publish topics as retained


// -------------------------------------------------------------------------------------------------
// Variable declaration

// Pin
int pinVx1Led1 = RX;          // Which pin the first status led is attached to
int pinVx1Led2 = D1;          // Which pin the second status led is attached to
int pinVx1Button = D2;        // Which pin the first button is attached to
int pinVx1Servo = D3;         // Which pin the first servo is attached to
int pinVx2Servo = D4;         // Which pin the second servo is attached to

//int pinVx2Led1 = D5;        // Which pin the third status led is attached to
//int pinVx2Led2 = D6;        // Which pin the forth status led is attached to
//int pinVx2Button = D7;      // Which pin the second button is attached to

// Button
int hasStarted = 1;           // Indicates if we have just started from a power down
int button1State = 1;         // Indicates if the first button has been pressed
//int button2State = 1;         // Indicates if the second button has been pressed

// Debug
byte debug = 1;               // Set to "1" for debug messages in Serial monitor (9600 baud)
String dbText = "Main   : ";  // Debug text

// Device
String deviceID;
String deviceName;

// Servo 1
int servo1min;
int servo1max;
int servo1move;
int servo1back;

// Servo 2
int servo2min;
int servo2max;
int servo2move;
int servo2back;


// -------------------------------------------------------------------------------------------------
// Declare all the objects to be used in the program
mmrcServo servoVx1(pinVx1Servo);
mmrcServo servoVx2(pinVx2Servo);
mmrcStatus ledVx1Rakt(pinVx1Led1);
mmrcStatus ledVx1Turn(pinVx1Led2);
EasyButton vx1button(pinVx1Button, 100, false, true);


// --------------------------------------------------------------------------------------------------
//  Setting things up
// --------------------------------------------------------------------------------------------------
void setup() { 
  if (debug == 1) {Serial.begin(9600);Serial.println("");}

  // ------------------------------------------------------------------------------------------------
  // Button start

  // Initialize the button
  vx1button.begin();

  // Add the callback function to be called when the button is pressed
  vx1button.onPressed(btn1Pressed);

  // ------------------------------------------------------------------------------------------------
  // IotWebConfig start
  if (debug == 1) {Serial.println(dbText+"IotWebConf start");}

  // Adding up items to show on config web page
  iotWebConf.addParameter(&webDeviceId);
  iotWebConf.addParameter(&webDeviceName);
  iotWebConf.addParameter(&webMqttServer);
  iotWebConf.addParameter(&webMqttPort);
  iotWebConf.addParameter(&separator1);
  iotWebConf.addParameter(&webServo1Min);
  iotWebConf.addParameter(&webServo1Max);
  iotWebConf.addParameter(&webServo1Move);
  iotWebConf.addParameter(&webServo1Back);
  iotWebConf.addParameter(&separator2);
  iotWebConf.addParameter(&webServo2Min);
  iotWebConf.addParameter(&webServo2Max);
  iotWebConf.addParameter(&webServo2Move);
  iotWebConf.addParameter(&webServo2Back);
  iotWebConf.getApTimeoutParameter()->visible = true; // Show & set AP timeout at start

//  iotWebConf.setStatusPin(STATUS_PIN);
//  iotWebConf.setConfigPin(CONFIG_PIN);

  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  
  // -- Get all the configuration/settings from EEPROM memory
  // TODO Hantera/konvertera MQTT-parametrar
  boolean validConfig = iotWebConf.init();
  if (validConfig == true) {
    if (debug == 1) {Serial.println(dbText+"Config from EEPROM");}
    deviceID = String(cfgDeviceId);
    deviceName = String(cfgDeviceName);

    servo1min = atoi(cfgServo1Min);
    servo1max = atoi(cfgServo1Max);
    servo1move = atoi(cfgServo1Move);
    servo1back = atoi(cfgServo1Back);
    servo2min = atoi(cfgServo2Min);
    servo2max = atoi(cfgServo2Max);
    servo2move = atoi(cfgServo2Move);
    servo2back = atoi(cfgServo2Back);
  } else {
    if (debug == 1) {Serial.println(dbText+"Default config");}
    deviceName = String(thingName);
    String tmpNo = String(random(2147483647));
    deviceID = deviceName+"-"+tmpNo;

    servo1min = 80;
    servo1max = 94;
    servo1move = 100;
    servo1back = 3;
    servo2min = 87;
    servo2max = 99;
    servo2move = 100;
    servo2back = 3;
  }

  // Set up required URL handlers for the config web pages
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  if (debug == 1) {Serial.println(dbText+"IotWebConf start...done");}

  // ------------------------------------------------------------------------------------------------
  // Set up topics to subscribe and publish to

  // Subscribe
  subTopic[0] = "mmrc/"+deviceID+"/turnout01/turn/set";
//  subTopic[1] = "mmrc/"+deviceID+"/turnout01/turn/set";
//  subTopic[2] = signalOneSlaveListen;
//  subTopic[3] = signalTwoSlaveListen;

  // Publish - device
  pubTopic[0] = "mmrc/"+deviceID+"/$name";
  pubTopicContent[0] = deviceName;
  pubTopic[1] = "mmrc/"+deviceID+"/$nodes";
  pubTopicContent[1] = "turnout01";

  // Publish - node 01
  pubTopic[2] = "mmrc/"+deviceID+"/turnout01/$name";
  pubTopicContent[2] = "Växel 1";
  pubTopic[3] = "mmrc/"+deviceID+"/turnout01/$type";
  pubTopicContent[3] = "double";
  pubTopic[4] = "mmrc/"+deviceID+"/turnout01/$properties";
  pubTopicContent[4] = "turn";
  
  // Publish - node 01 - property 01
  pubTopic[5] = "mmrc/"+deviceID+"/turnout01/turn/$name";
  pubTopicContent[5] = "Omläggning";
  pubTopic[6] = "mmrc/"+deviceID+"/turnout01/turn/$datatype";
  pubTopicContent[6] = "string";

  // Device status
  pubTurnoutState = "mmrc/"+deviceID+"/turnout01/turn";
  pubDeviceStateTopic = "mmrc/"+deviceID+"/$state";

  // -----------------------------------------------------------------------------------------------
  // Connect to MQTT broker and define function to handle callbacks
  delay(2000);    // Wait for IotWebServer to start network connection
  int mqttPort = atoi(cfgMqttPort);
  mqttClient.setServer(cfgMqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  // TODO Hantera/konvertera MQTT-parametrar

  
  // -----------------------------------------------------------------------------------------------
  // Servo start

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
//  Function to show AP (access point) web start page
// --------------------------------------------------------------------------------------------------
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String page = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" ";
  page += "content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  page += "<title>MMRC-inst&auml;llningar</title></head><body>";
  page += "<h1>Inst&auml;llningar</h1>";
  page += "<p>V&auml;lkommen till MMRC-enheten med namn: '";
  page += cfgDeviceId;
  page += "'</p>";
  page += "P&aring; sidan <a href='config'>Inst&auml;llningar</a> ";
  page += "kan du best&auml;mma hur just denna MMRC-klient ska fungera.";

  page += "<p>M&ouml;jligheten att &auml;ndra dessa inst&auml;llningar &auml;r ";
  page += "alltid tillg&auml;nglig de f&ouml;rsta ";
  page += "30";
  page += " sekunderna efter start av enheten.";
  page += "</body></html>\n";

  server.send(200, "text/html", page);
}


// --------------------------------------------------------------------------------------------------
//  Function that gets called when web page config has been saved
// --------------------------------------------------------------------------------------------------
void configSaved()
{
  if (debug == 1) {Serial.println(dbText+"IotWebConf config saved");}
  deviceID = String(cfgDeviceId);
  deviceName = String(cfgDeviceName);
  // TODO Hantera/konvertera MQTT-parametrar
  
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


// --------------------------------------------------------------------------------------------------
//  Function beeing called when wifi connection is up and running
// --------------------------------------------------------------------------------------------------
void wifiConnected() {
  if (debug == 1) {Serial.println(dbText+"Wifi connected, start MQTT");}
  // We are ready to start the MQTT connection
  needMqttConnect = true;
}


// --------------------------------------------------------------------------------------------------
// (Re)connects to MQTT broker and subscribes to one or more topics
// --------------------------------------------------------------------------------------------------
boolean mqttConnect() {
  char tmpTopic[254];
  char tmpContent[254];
  char tmpID[deviceID.length()];    // For converting deviceID
  char* tmpMessage = "lost";        // Status message in Last Will
  
  // Convert String to char* for last will message
  deviceID.toCharArray(tmpID, deviceID.length()+1);
  pubDeviceStateTopic.toCharArray(tmpTopic, pubDeviceStateTopic.length()+1);
  
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
  if (debug == 1) {Serial.print(dbText+"MQTT connection...");}

    // Attempt to connect
    // boolean connect (tmpID, pubDeviceStateTopic, willQoS, willRetain, willMessage)
    if (mqttClient.connect(tmpID,tmpTopic,0,true,tmpMessage)) {
      if (debug == 1) {Serial.println("connected");}
      if (debug == 1) {Serial.print(dbText+"MQTT client id = ");}
      if (debug == 1) {Serial.println(cfgDeviceId);}

      // Subscribe to all topics
      if (debug == 1) {Serial.println(dbText+"Subscribing to:");}
      for (int i=0; i < nbrSubTopics; i++){
        // Convert String to char* for the mqttClient.subribe() function to work
        subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);
  
        // ... print topic
        if (debug == 1) {Serial.println(dbText+" - "+tmpTopic);}

        //   ... and subscribe to topic
        mqttClient.subscribe(tmpTopic);
      }

      // Publish to all topics
      if (debug == 1) {Serial.println(dbText+"Publishing to:");}
      for (int i=0; i < nbrPubTopics; i++){
        // Convert String to char* for the mqttClient.publish() function to work
        pubTopic[i].toCharArray(tmpTopic, pubTopic[i].length()+1);
        pubTopicContent[i].toCharArray(tmpContent, pubTopicContent[i].length()+1);

        // ... print topic
        if (debug == 1) {Serial.print(dbText+" - "+tmpTopic);}
        if (debug == 1) {Serial.print(" = ");}
        if (debug == 1) {Serial.println(tmpContent);}

        // ... and subscribe to topic
        mqttClient.publish(tmpTopic, tmpContent, true);
      
      }
     
    } else {
      // Show why the connection failed
      if (debug == 1) {Serial.print(dbText+"failed, rc=");}
      if (debug == 1) {Serial.print(mqttClient.state());}
      if (debug == 1) {Serial.println(", try again in 5 seconds");}

      // Wait 5 seconds before retrying
      delay(5000);
     
    }
  }

  // Set device status to "ready"
  mqttPublish(pubTurnoutState, "unknown", RETAIN);
  mqttPublish(pubDeviceStateTopic, "ready", RETAIN);
  return true;

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

  // Check for "turnout01/turn/set" command
  if (tpc == subTopic[0]) {
    if (msg == "toggle") {
      btn1Pressed();
    }
  }
  
}

// --------------------------------------------------------------------------------------------------
//  Publish message to MQTT broker
// --------------------------------------------------------------------------------------------------
void mqttPublish(String pbTopic, String pbPayload, byte retain) {

  // Convert String to char* for the mqttClient.publish() function to work
  char msg[pbPayload.length()+1];
  pbPayload.toCharArray(msg, pbPayload.length()+1);
  char tpc[pbTopic.length()+1];
  pbTopic.toCharArray(tpc, pbTopic.length()+1);

  // Report back to pubTopic[]
  int check = mqttClient.publish(tpc, msg, retain);

  // TODO check "check" integer to see if all went ok

  // Print information
  if (debug == 1) {Serial.println(dbText+"Sending: "+pbTopic+" = "+pbPayload);}

}
