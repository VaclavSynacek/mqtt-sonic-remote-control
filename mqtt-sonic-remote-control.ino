/*
 MQTT Sonic Remote Control

   NodeMCU and Ultrasonic sensor connected to MQTT broker sending
   distance measurements every 50ms. Eventually will evolve into
   kind of a poor man's gesture based remote control.

   Idea inspired by https://www.youtube.com/watch?v=VdtUOSBrZIo
   but removing the PIR, adding MQTT and chnging arduino to NodeMCU.
 
 Code structue originally based on:
   Basic ESP8266 MQTT example
   created by Nick O'Leary - https://github.com/knolleary
   taken from https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
#include "secrets.h"

#define TRIGGER 5
#define ECHO    4
// NodeMCU Pin D1 = GPIO5 > TRIGGER | Pin D2 = GPIO4 > ECHO

#define TOPIC "command"

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.print("Connectig to WiFi ");

  delay(10);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT ");
    String clientId = "ESP8266-sonic-remote-control";
    if (!client.connect(clientId.c_str())) {
      Serial.print(".");
      delay(500);
    }
  }
}

void setup() {
  Serial.println("");
  Serial.begin(115200);
  Serial.println("");
  Serial.println("MQTT-Sonic-Remote-Control");
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

long getDistance() {
  
  long duration, distance;
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(TRIGGER, LOW);
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;
  
  return distance;
}


const int OUT = 10;
const int UP = 5;
const int DOWN = 3;
const int ENTER = 0;

int toState(int distance) {
  if (distance < 10) return ENTER;
  if (distance < 30) return DOWN;
  if (distance < 100) return UP;
  return OUT;
}

long previousStateSince = 0;
long now = 0;
int previousState = OUT;
int currentState = OUT;

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  currentState = toState(getDistance());
  now = millis();

  if (currentState == previousState) {
    if (now - previousStateSince > 300) {
      previousStateSince = now;
      switch(currentState) { //when I write section like this I do miss Lisp
        case ENTER:
          client.publish(TOPIC, "enter");
          break;
        case UP:
          client.publish(TOPIC, "up");
          break;
        case DOWN:
          client.publish(TOPIC, "down");
          break;
        case OUT:
          client.publish(TOPIC, "out");
          break;
      }
    }
  } else {
    previousStateSince = now;
    previousState = currentState;
  }
}
