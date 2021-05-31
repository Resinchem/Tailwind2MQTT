/*
 * Tailwind MQTT Control and Status
 * Alpha v0.45 - Added additional Home Assistant MQTT Discovery (switches/door control)
 * Pleaes review setup and install info at https://github.com/Resinchem/Tailwind2MQTT
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>          //NEW for v0.43+
#include <FS.h>
#include "Credentials.h"          //File must exist in same folder as .ino.  
#include "Settings.h"             //File must exist in same folder as .ino.  

//GLOBAL VARIABLES
bool mqttConnected = false;       //Will be enabled if defined and successful connnection made. 
long lastReconnectAttempt = 0;    //If MQTT connected lost, attempt reconnenct
unsigned long nextPollTime = 0;
unsigned long nextSwitchPollTime = 0;
uint16_t ota_time_elapsed = 0;           // Counter when OTA active
uint16_t ota_time = ota_boot_time_window;
const char* APssid = AP_SSID;        
const char* APpassword = AP_PWD;  
const char *ssid = SID;
const char *password = PW;
const char *mqttUser = MQTTUSERNAME;
const char *mqttPW = MQTTPWD;
const char *mqttClient = MQTTCLIENT;
const char *doorStatusURL = "http://"TAILWIND_IP"/status";
const char *doorCommandURL = "http://"TAILWIND_IP"/cmd";

int statusCodes[8][3] = {
  {-1, -2, -4},   //0
  {1, -2, -4},    //1
  {-1, 2, -4},    //2
  {1, 2, -4},     //3
  {-1, -2, 4},    //4
  {1, -2, 4},     //5
  {-1, 2, 4},     //6
  {1, 2, 4}       //7
};

int curDoorStatus = 99;
WiFiClient espClient;
PubSubClient client(espClient);    
ESP8266WebServer server;
HTTPClient http;

//------------------------------
// Setup WIFI
//-------------------------------
void setup_wifi() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);  //Disable WiFi Sleep
  delay(200);
  // WiFi - AP Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);    // IP is usually 192.168.4.1
#endif
  // WiFi - Local network Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2) 
  byte count = 0;
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // Stop if cannot connect
    if (count >= 60) {
      // Could not connect to local WiFi 
      Serial.println();
      Serial.println("Could not connect to WiFi.");     
      return;
    }
    delay(500);
    count++;
  }
  Serial.println();
  Serial.println("Successfully connected to Wifi");
  IPAddress ip = WiFi.localIP();
  Serial.println(WiFi.localIP());
#endif   
};

//------------------------------
// Setup MQTT
//-------------------------------
void setup_mqtt() {
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  byte mcount = 0;
  //char topicPub[32];
  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(callback);
  Serial.print("Connecting to MQTT broker.");
  while (!client.connected( )) {
    Serial.print(".");
    client.connect(mqttClient, mqttUser, mqttPW);
    if (mcount >= 60) {
      Serial.println();
      Serial.println("Could not connect to MQTT broker. MQTT disabled.");
      // Could not connect to MQTT broker
      return;
    }
    delay(500);
    mcount++;
  }
  Serial.println();
  Serial.println("Successfully connected to MQTT broker.");
  client.subscribe(MQTT_TOPIC_SUB"/#");
  client.publish(MQTT_TOPIC_PUB"/mqtt", "connected", true);
  mqttConnected = true;
#endif
}

void reconnect() 
{
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqttClient, mqttUser, mqttPW)) 
      {
        Serial.println("connected");
        // ... and resubscribe
        client.subscribe(MQTT_TOPIC_SUB"/#");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 149)
    {
    ESP.restart();
    }
  }
}

// =============================================================
// *************** MQTT Message Processing *********************
// =============================================================
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;      // should contain door number (1, 2, 3) or "ON"/"OFF" for discovered switches
  String tailwindCommand;               //string value required for POST
  int validReturnCode = 0;
  bool validCommand = true;
  if (strcmp(topic, MQTT_TOPIC_SUB"/opendoor") == 0) {
    int tailwindParm = message.toInt();   //use integer value for validation/comparison

    //validate payload and convert to API value (door 3)
    if ((tailwindParm == 1) || (tailwindParm == 2)) {
      tailwindCommand = message;
    } else if (tailwindParm == 3) {
      tailwindCommand = "4";
    } else {
      //Invalid command
      validCommand = false;
    }
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/closedoor") == 0) {
    int tailwindParm = message.toInt();
    //validate payload and convert to API value
     if (tailwindParm == 1) {
        tailwindCommand = "-1";
     } else if (tailwindParm == 2) {
        tailwindCommand = "-2";
     } else if (tailwindParm == 3) {
        tailwindCommand = "-4";
     } else {
       //Invalid command
       validCommand = false;
     }
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/toggledoor") == 0) {
    int tailwindParm = message.toInt();
    int curDoorPos = getSingleDoorStatus(tailwindParm);
    tailwindCommand = String(curDoorPos * -1); 
  /* These topics have to be added for Home Assistant MQTT switch discovery  
   *   Home Assistant will always pass "OFF" or "ON" as the payload in the command topic
   *   so it is not possible to pass the door number as the payload.  Therefore, each door
   *   "switch" must have it's own topic. A delay must be added to allow the operation
   *   to complete and a status change occurs to prevent switch "bouncing" in HA
   */
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/door1/setstate") == 0) {
    if (message == "ON") {
      tailwindCommand = "1";
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "ON", true);
      nextSwitchPollTime = millis() + switch_delay_open;
    } else {
      tailwindCommand = "-1";
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "OFF", true);
      nextSwitchPollTime = millis() + switch_delay_close;
    }
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/door2/setstate") == 0) {
    if (message == "ON") {
      tailwindCommand = "2";
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "ON", true);
      nextSwitchPollTime = millis() + switch_delay_open;
    } else {
      tailwindCommand = "-2";
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "OFF", true);
      nextSwitchPollTime = millis() + switch_delay_close;
    }
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/door2/setstate") == 0) {
    if (message == "ON") {
      tailwindCommand = "4";
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "ON", true);
      nextSwitchPollTime = millis() + switch_delay_open;
    } else {
      tailwindCommand = "-4";
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "OFF", true);
      nextSwitchPollTime = millis() + switch_delay_close;
    }
  }
  
  
  if (validCommand) {
    validReturnCode = tailwindCommand.toInt();
    //issue curl command
    http.begin(doorCommandURL);
    int returnCode = http.POST(tailwindCommand);
    String payloadS = http.getString(); //Get the response payload
    if (returnCode != 200) {
      //debug info
      Serial.print("Command failed. Returned code: ");
      Serial.println(returnCode);
      Serial.print("URL: ");
      Serial.println(doorCommandURL);
      Serial.print("Command: ");
      Serial.println(tailwindCommand);
      //Update last command status
      client.publish(MQTT_TOPIC_PUB"/lastresult", "FAILED", true);
    } else if ((payloadS.toInt()) != (validReturnCode)) {
      //Tailwind did not return proper response string (should match payload)
      //debug info
      Serial.print("Command: ");
      Serial.println(tailwindCommand);
      Serial.print("Tailwind returned: ");
      Serial.println(payloadS);
      client.publish(MQTT_TOPIC_PUB"/lastresult", "FAILED", true);
    } else {
      client.publish(MQTT_TOPIC_PUB"/lastresult", "OK", true);
    }
    http.end(); //Close connection
  } else {
    client.publish(MQTT_TOPIC_PUB"/lastresult", "INVALID", true);
  }
  return;
};
//-------------------------------
// Home Assistant MQTT Discovery
//-------------------------------
void setup_ha_discovery() {
  if (ha_discovery) {
    char buffer[256];
    char buffer2[256];
    char buffer3[256];
    char bufferm[256];
    char bufferl[256];
    char buffers[256];
    char bufferd1[256];
    char bufferd2[256];
    char bufferd3[256];
    StaticJsonDocument<200> doc;
    // Door binary sensor states
    doc["name"] = "tailwind_door1";
    doc["device_class"] = "garage_door";
    doc["state_topic"] = MQTT_TOPIC_PUB"/door1/state";
    serializeJson(doc, buffer);
    client.publish("homeassistant/binary_sensor/tailwind_door1/config", buffer, true);

    doc.clear();
    doc["name"] = "tailwind_door2";
    doc["device_class"] = "garage_door";
    doc["state_topic"] = MQTT_TOPIC_PUB"/door2/state";
    serializeJson(doc, buffer2);
    client.publish("homeassistant/binary_sensor/tailwind_door2/config", buffer2, true);

    doc.clear();
    doc["name"] = "tailwind_door3";
    doc["device_class"] = "garage_door";
    doc["state_topic"] = MQTT_TOPIC_PUB"/door3/state";
    serializeJson(doc, buffer3);
    client.publish("homeassistant/binary_sensor/tailwind_door3/config", buffer3, true);

    //Other sensors
    //mqtt connection status
    doc.clear();
    doc["name"] = "tailwind_mqtt_status";
    doc["state_topic"] = MQTT_TOPIC_PUB"/mqtt";
    serializeJson(doc, bufferm);
    client.publish("homeassistant/sensor/tailwind_mqtt/config", bufferm, true);

    //lastcommand result
    doc.clear();
    doc["name"] = "tailwind_last_result";
    doc["state_topic"] = MQTT_TOPIC_PUB"/lastresult";
    serializeJson(doc, bufferl);
    client.publish("homeassistant/sensor/tailwind_lastresult/config", bufferl, true);

    //status code result
    doc.clear();
    doc["name"] = "tailwind_status_code";
    doc["state_topic"] = MQTT_TOPIC_PUB"/statuscode";
    serializeJson(doc, buffers);
    client.publish("homeassistant/sensor/tailwind_status_code/config", buffers, true);
    
   // switches
   doc.clear();
   doc["name"] = "tailwind_door1";
   doc["stat_t"] = MQTT_TOPIC_PUB"/door1/switchstate";
   doc["cmd_t"] = MQTT_TOPIC_SUB"/door1/setstate";
   serializeJson(doc, bufferd1);
   client.publish("homeassistant/switch/tailwind_door1/config", bufferd1, true);   

   doc.clear();
   doc["name"] = "tailwind_door2";
   doc["stat_t"] = MQTT_TOPIC_PUB"/door2/switchstate";
   doc["cmd_t"] = MQTT_TOPIC_SUB"/door2/setstate";
   serializeJson(doc, bufferd2);
   client.publish("homeassistant/switch/tailwind_door2/config", bufferd2, true);   

   doc.clear();
   doc["name"] = "tailwind_door3";
   doc["stat_t"] = MQTT_TOPIC_PUB"/door3/switchstate";
   doc["cmd_t"] = MQTT_TOPIC_SUB"/door3/setstate";
   serializeJson(doc, bufferd3);
   client.publish("homeassistant/switch/tailwind_door3/config", bufferd3, true);   
  
  } else {
    // publish with empty payload, which will remove HA entities if previously created
    client.publish("homeassistant/binary_sensor/tailwind_door1/config", "");
    client.publish("homeassistant/binary_sensor/tailwind_door2/config", "");
    client.publish("homeassistant/binary_sensor/tailwind_door3/config", "");
    client.publish("homeassistant/sensor/tailwind_mqtt/config", "");
    client.publish("homeassistant/sensor/tailwind_lastresult/config", "");
    client.publish("homeassistant/sensor/tailwind_status_code/config","");
    client.publish("homeassistant/switch/tailwind_door1/config", "");
    client.publish("homeassistant/switch/tailwind_door2/config", "");
    client.publish("homeassistant/switch/tailwind_door3/config", "");
  }
  return;
};
// ============================================
//   SETUP
// ============================================
void setup() {
  // Serial monitor
  Serial.begin(115200);
  Serial.println("Booting...");
  setup_wifi();
  setup_mqtt();
  setup_ha_discovery();
  //-----------------------------
  // Setup OTA Updates
  //-----------------------------
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  });
  ArduinoOTA.begin();
  // Setup handlers for web calls for OTAUpdate and Restart
  server.on("/restart",[](){
    server.send(200, "text/html", "<h1>Restarting...</h1>");
    delay(1000);
    ESP.restart();
  });
  server.on("/otaupdate",[]() {
    server.send(200, "text/html", "<h1>Ready for upload...<h1><h3>Start upload from IDE now</h3>");
    ota_flag = true;
    ota_time = ota_time_window;
    ota_time_elapsed = 0;
  });
  server.begin();
  delay(20);
  //Assure tailwind is not polled more often than once per second, regardless of user settings
  if (tailwind_poll < 1000) {
    tailwind_poll = 1000;
  }
}

//=============================
// Main Loop
//=============================
void loop() {
  //Handle OTA updates when OTA flag set via HTML call to http://ip_address/otaupdate
  if (ota_flag) {
    uint16_t ota_time_start = millis();
    while (ota_time_elapsed < ota_time) {
      ArduinoOTA.handle();  
      ota_time_elapsed = millis()-ota_time_start;   
      delay(10); 
    }
    ota_flag = false;
  }
  //Handle any web calls
  server.handleClient();
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  delay(1000);
  if (millis() > nextPollTime) {
    int newDoorStatus;
    newDoorStatus = getDoorStatus();
    // Only update MQTT if status has changed and updated code is returned   
    if ((newDoorStatus != curDoorStatus) && (newDoorStatus >= 0) && (newDoorStatus <= 7)) {
      curDoorStatus = newDoorStatus;
      updateMQTTStatus();
      // Needed to prevent switch "bounce" in HA until operation completes and state updates
      if (millis() > nextSwitchPollTime) {
        updateMQTTSwitchStatus();
        nextSwitchPollTime = 0;
      }
    }
    nextPollTime = millis() + tailwind_poll;
  }
}

int getDoorStatus() {
  int retVal = 99;
  http.begin(doorStatusURL);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    retVal = payload.toInt();
  }
  http.end();
  return retVal;
}

int getSingleDoorStatus(int whichDoor) {
  int curStatus = getDoorStatus();
  int retVal = 99;
  retVal = statusCodes[curStatus][whichDoor-1];
  return retVal;
}

void updateMQTTStatus() {
  switch (curDoorStatus) {
    case 0:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "0", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "OFF", true);
      break;
    case 1:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "1", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "ON", true);
      break;
    case 2:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "2", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "OFF", true);
      break;
    case 3:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "3", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "ON", true);
      break;
    case 4:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "4", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "OFF", true);
      break;
    case 5:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "5", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "ON", true);
      break;
    case 6:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "6", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "OFF", true);
      break;
    case 7:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "7", true);
      client.publish(MQTT_TOPIC_PUB"/door3/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/state", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/state", "ON", true);
      break;
    default:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "99", true);
      break;     
  }
}

void updateMQTTSwitchStatus() {
  switch (curDoorStatus) {
    case 0:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "OFF", true);
      break;
    case 1:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "ON", true);
      break;
    case 2:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "OFF", true);
      break;
    case 3:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "ON", true);
      break;
    case 4:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "OFF", true);
      break;
    case 5:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "OFF", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "ON", true);
      break;
    case 6:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "OFF", true);
      break;
    case 7:
      client.publish(MQTT_TOPIC_PUB"/door3/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door2/switchstate", "ON", true);
      client.publish(MQTT_TOPIC_PUB"/door1/switchstate", "ON", true);
      break;
    default:
      client.publish(MQTT_TOPIC_PUB"/statuscode", "99", true);
      break;     
  }
}
