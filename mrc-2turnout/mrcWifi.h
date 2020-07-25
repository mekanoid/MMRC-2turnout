/*
  MRC Wifi settings
*/

// --------------------------------------------------------------------------------------------------
// IotWebConf variables

// Default string and number lenght
#define STRING_LEN 32
#define NUMBER_LEN 8

// Access point configuration
const char thingName[] = "MRC-client";                  // Initial AP name, used as SSID of the own Access Point
const char wifiInitialApPassword[] = "mrc4president";   // Initial password, used when it creates an own Access Point

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

void wifiSetup() {
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

  if (debug == 1) {Serial.println(dbText+"IotWebConf start...done");}

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
  page += "<title>MRC-inst&auml;llningar</title></head><body>";
  page += "<h1>Inst&auml;llningar</h1>";
  page += "<p>V&auml;lkommen till MRC-enheten med namn: '";
  page += cfgDeviceId;
  page += "'</p>";
  page += "P&aring; sidan <a href='config'>Inst&auml;llningar</a> ";
  page += "kan du best&auml;mma hur just denna MRC-klient ska fungera.";

  page += "<p>M&ouml;jligheten att &auml;ndra dessa inst&auml;llningar &auml;r ";
  page += "alltid tillg&auml;nglig de f&ouml;rsta ";
  page += "30";
  page += " sekunderna efter start av enheten.";
  page += "</body></html>\n";

  server.send(200, "text/html", page);
}


// --------------------------------------------------------------------------------------------------
//  Function beeing called when wifi connection is up and running
// --------------------------------------------------------------------------------------------------
void wifiConnected() {
  if (debug == 1) {Serial.println(dbText+"Wifi connected, start MQTT");}
  // We are ready to start the MQTT connection
  needMqttConnect = true;
}
