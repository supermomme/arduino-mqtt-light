/*
 * Author: Momme Jürgensen <momme@juergensen.me>
 * Description: MQTT-Light with legacy switch capability
 * PINS:
 * 10 <-> ESP-TX
 * 11 <-> ESP-RX
 * 2 <-> Relay Trigger
 * 3 <-> switch
*/

#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include "SoftwareSerial.h"
#include <PubSubClient.h>
#include <Arduino_JSON.h>

IPAddress server(3, 123, 171, 49); // MQTT-Broker
char ssid[] = "SSID"; // WIFI-SSID
char pass[] = "PASS"; // WIFI-PASSWORD
int status = WL_IDLE_STATUS;

boolean mainLight = false;
#define mainLightPin 2
#define mainLightSwitchPin 3

WiFiEspClient espClient;

PubSubClient client(espClient);

SoftwareSerial soft(10,11);
void setup() {
  Serial.begin(9600);
  pinMode(mainLightPin, OUTPUT);
  setMainLight(false);
  pinMode(mainLightSwitchPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(mainLightSwitchPin), toggleMainLight, CHANGE);
    
  soft.begin(9600);
  WiFi.init(&soft);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present :(");
    while (true);
  }

  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network" + WiFi.localIP());

  client.setServer(server, 1883);
  client.setCallback(callback);
}

void toggleMainLight() {
  mainLight = !mainLight;
  digitalWrite(mainLightPin, !mainLight);
  publishMainLightStatus();
}

void setMainLight(boolean state) {
  mainLight = state;
  digitalWrite(mainLightPin, !mainLight);
  publishMainLightStatus();
}

void publishMainLightStatus() {
  JSONVar data;
  data["val"] = mainLight;
  data["connected"] = true;
  String jsonString = JSON.stringify(data);
  Serial.println(jsonString);
  
  char jsonCharArray[jsonString.length()];
  jsonString.toCharArray(jsonCharArray, jsonString.length());
  client.publish("home/room/momme/light/main", jsonCharArray, true);
}


//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char* pl;
  for (int i=0;i<length;i++) {
    pl[i] = (char)payload[i];
  }
  Serial.print(pl);
  Serial.println();
  JSONVar data = JSON.parse(pl);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
  boolean state = data["val"];
  setMainLight(state);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("arduino-client-momme", "home/room/momme/light/main", 0, true, "{\"val\":false, \"connected\": false}")) {
      Serial.println("connected");
      publishMainLightStatus();
      client.subscribe("home/room/momme/light/main");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
