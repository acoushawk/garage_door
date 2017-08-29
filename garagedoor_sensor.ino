#include <EEPROM.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <RunningMedian.h>

// defines pins numbers
const int trigPin = D7;
const int echoPin = D8;
const int relayPin = D6;

// Wifi stuff
const char* commandSub="home/inside/garage/door/control";
const char* statusSub="home/inside/garage/door/status";
const char* triggeredSub="home/inside/garage/door/triggered";
const char* mqtt_server = "192.168.100.6";

// defines variables
long duration;
long distance;
bool doorOpen;
unsigned long currentMilis;
RunningMedian samples = RunningMedian(15);
long count = 0;
long medianDistance;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
WiFiManager wifiManager;
wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
wifiManager.autoConnect("GarageDoorAP");
pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
pinMode(echoPin, INPUT); // Sets the echoPin as an Input
pinMode(relayPin, OUTPUT);
digitalWrite(relayPin, LOW);
Serial.begin(9600); // Starts the serial communication

client.setServer(mqtt_server, 1883);
client.setCallback(callback);
currentMilis = millis();
}

void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
 // Attempt to connect
 if (client.connect("GarageDoor Sensor")) {
  // ... and subscribe to topic
  client.subscribe(commandSub);
  client.subscribe(statusSub);
  client.subscribe(triggeredSub);
 } else {
  // Wait 30 seconds before retrying
  delay(30000);
  }
 }
}

void loop() {
if (!client.connected()) 
{
    reconnect();
}
client.loop();
if (millis() - currentMilis >= 2000 ) {
    count++;
    Serial.println(count);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);

    // Calculating the distance
    distance = duration*0.0133/2;
    samples.add(distance);

    currentMilis = millis();
}
else if (millis() - currentMilis < 0) 
{
    currentMilis = millis();
}


if (count >= 15) {
Serial.println("Checking Distance");
medianDistance = samples.getMedian();
Serial.println(medianDistance);
    if (medianDistance > 23 && doorOpen != true) 
    {
        doorstatus(true);
        doorOpen = true;
    }
    else if (medianDistance < 23 && doorOpen != false)
    {
        doorstatus(false);
        doorOpen = false;
    }
    count = 0;
    samples.clear();
}
}

void callback(char* topic, byte* payload, unsigned int length) {
 for (int i=0;i<length;i++) 
  {
    char receivedChar = (char)payload[i];
    if (String(topic) == String(commandSub)) 
    {
        Serial.println("got the command");
        triggerdoor();
    }
  }
}

void triggerdoor (){
    Serial.println("triggered");
    digitalWrite(relayPin, HIGH);
    delay(2000);
    digitalWrite(relayPin, LOW);
    client.publish(triggeredSub, "Started");
    delay(20000);
    client.publish(triggeredSub, "Done");
}

void doorstatus (bool status){
  if (status) {
    client.publish(statusSub, "Closed");
  } 
  else
  {
    client.publish(statusSub, "Open");
  }
}
