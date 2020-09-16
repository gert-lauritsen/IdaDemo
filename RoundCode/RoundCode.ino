#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <String.h>
#include <PubSubClient.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include "password.h"

const int relay     = 15;
const int OptoInput = 5;

char ID[8];
char mqttTopicStatus[25];
char ONOFF_FEED[25];

bool CmdOnOff = false; //input state om den er og eller off (optokobler)
bool LastCmdOnOff = false; // for at kunne toggle på ændringer
long Cmdonofftimeout = 0; //
char IPadr[20];

WiFiClient wificlient;
PubSubClient client(wificlient);

int updatestatus = 0;
//flag for saving data
bool shouldSaveConfig = false;
char msg[25];
bool AcOnOff;
long lasttest;
//---------------------------------------------------------------------------------------------------------

void switchPressed ()
{
  static unsigned long LastKeypress;
  if ((LastKeypress == 0) | ((millis() - LastKeypress) > 200)) {
    CmdOnOff=!CmdOnOff;
    digitalWrite(relay,CmdOnOff);
    updatestatus = 1;
    LastKeypress = millis();
  }
}

// Message received through MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(" Message arrived ");
  if (payload[0] == '1') CmdOnOff=1;
  if (payload[0] == '0') CmdOnOff=0;
  if (payload[0] == 'T') CmdOnOff=!CmdOnOff;  
   digitalWrite(relay,CmdOnOff);
   updatestatus = 1;
}





void setup() {
  sprintf(&ID[0], "%08X", ESP.getChipId());
  Serial.begin(115200);
  Serial.println("");
  //clean FS, for testing
  //SPIFFS.format();

  wifiSetup();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  digitalWrite (OptoInput, HIGH); // internal pull-up resistor  
  Serial.print(ID);
  Serial.println(" Booting");
  Serial.println("Setup Done");
  updatestatus = 1; //Start med at sende status til senver
}

void SendStatus() {
  char msg[80];
  char temp[25];
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  if (CmdOnOff) json["state"]="ON"; else json["state"]="OFF";
  json["RSSI"]=WiFi.RSSI();
  json["IP"]=IPadr;
  json.printTo(Serial);
  Serial.println("");
  json.printTo(msg);
  client.publish(mqttTopicStatus, msg);
}

void loop() {
  // put your main code here, to run repeatedly:
  wifiHandle();
  if (millis()>lasttest) {
     AcOnOff=digitalRead(OptoInput); //læser state
     lasttest=millis()+500;
  }   
  if (AcOnOff != LastCmdOnOff) {
    //Toggle output
    LastCmdOnOff = AcOnOff;
    CmdOnOff=!CmdOnOff;
    digitalWrite(relay,CmdOnOff);
    updatestatus = 1;
  }
  if (updatestatus) {
    updatestatus = 0;
    SendStatus();
  }
}
