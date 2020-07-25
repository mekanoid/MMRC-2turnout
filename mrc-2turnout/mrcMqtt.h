/*
  MRC MQTT settings & functions
*/

#include <Arduino.h>

WiFiClient wifiClient;            // Wifi initialisation
PubSubClient mqttClient(wifiClient);

// -------------------------------------------------------------------------------------------------
// Define topic variables

// Variable for topics to subscribe to
const int nbrSubTopics = 1;
String subTopic[nbrSubTopics];

// Variable for topics to publish to
const int nbrPubTopics = 8;
String pubTopic[nbrPubTopics];
String pubTopicContent[nbrPubTopics];

// Often used topics
String pubTurnoutState;
String pubDeviceStateTopic;

// MQTT command to execute when a MQTT message arrives (see mqttCallback() )
String mqttMessage;

const byte NORETAIN = 0;      // Used to publish topics as NOT retained
const byte RETAIN = 1;        // Used to publish topics as retained


// -------------------------------------------------------------------------------------------------
// Function to setup MQTT. Called from setup() function
// -------------------------------------------------------------------------------------------------
void mqttSetup() {

  // Subscribe
  subTopic[0] = "mrc/"+deviceID+"/turnout/direction/set";
//  subTopic[1] = "mrc/"+deviceID+"/turnout/direction/set";
//  subTopic[2] = signalOneSlaveListen;
//  subTopic[3] = signalTwoSlaveListen;

  // Publish - device
  pubTopic[0] = "mrc/"+deviceID+"/$name";
  pubTopicContent[0] = deviceName;

  pubTopic[1] = "mrc/"+deviceID+"/$deviceid";
  pubTopicContent[1] = deviceID;

  pubTopic[2] = "mrc/"+deviceID+"/$nodes";
  pubTopicContent[2] = "turnout";

  // Publish - node 01
  pubTopic[3] = "mrc/"+deviceID+"/turnout/$name";
  pubTopicContent[3] = "Växel 1";
  pubTopic[4] = "mrc/"+deviceID+"/turnout/$type";
  pubTopicContent[4] = "2turnout";
  pubTopic[5] = "mrc/"+deviceID+"/turnout/$properties";
  pubTopicContent[5] = "direction";
  
  // Publish - node 01 - property 01
  pubTopic[6] = "mrc/"+deviceID+"/turnout/direction/$name";
  pubTopicContent[6] = "Riktning";
  pubTopic[7] = "mrc/"+deviceID+"/turnout/direction/$datatype";
  pubTopicContent[7] = "string";

  // Device status
  pubTurnoutState = "mrc/"+deviceID+"/turnout/direction";
  pubDeviceStateTopic = "mrc/"+deviceID+"/$state";

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

  // Set device status to "ready" (convert String topic to Char)
  char tpc[pubTurnoutState.length()+1];
  pubTurnoutState.toCharArray(tpc, pubTurnoutState.length()+1);
  mqttClient.publish(tpc, "unknown", RETAIN);

  tpc[pubDeviceStateTopic.length()+1];
  pubDeviceStateTopic.toCharArray(tpc, pubDeviceStateTopic.length()+1);
  mqttClient.publish(tpc, "ready", RETAIN);

  return true;

}
