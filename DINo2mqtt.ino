/*
 DINO to MQTT bridge
 Inspired by https://github.com/fr00ller/DINo_MQTT
 
  - Connect (and reconnect if necessary) to an MQTT broker
  - Subscribe to the topics controlling the 4 relays
  - Publish data read by the 4 digital inputs
  - Publish the 4 relay states
 
*/

#include <SPI.h>
//#include <UIPEthernet.h>
#include <EthernetENC.h>
#include <PubSubClient.h>


// Topic definitions

const char* _Topic1 = "DINo/Out1/set";
const char* _Topic2 = "DINo/Out2/set";
const char* _Topic3 = "DINo/Out3/set";
const char* _Topic4 = "DINo/Out4/set";

const char* _SendTopic1 = "DINo/In1/state";
const char* _SendTopic2 = "DINo/In2/state";
const char* _SendTopic3 = "DINo/In3/state";
const char* _SendTopic4 = "DINo/In4/state";

const char* _SendTopicRel1 = "DINo/Out1/state";
const char* _SendTopicRel2 = "DINo/Out2/state";
const char* _SendTopicRel3 = "DINo/Out3/state";
const char* _SendTopicRel4 = "DINo/Out4/state";

const char* State0 = "0";
const char* State1 = "1";

// Input/output status

int A2current = -1, A3current = -1, A4current = -1, A5current = -1;
int R5current = -1, R6current = -1, R7current = -1, R8current = -1;


// Network definition

byte mac[]    = {  0x54, 0x55, 0x58, 0x10, 0x00, 0x11 };
IPAddress ip(192, 168, 4, 133); // Used only if DHCP fails

const char* mqtt_client_name = "DINo";
//byte mqtt_broker[] = { 192, 168, 4, 4 }; 
IPAddress mqtt_broker(192, 168, 4, 4);

// Used to schedule sending I/O states

unsigned long lastUpdate = 0;


// Callback function header

void callback(char* topic, byte* payload, unsigned int length);


// Ethernet and broker

EthernetClient ethClient;
PubSubClient mqtt_client(mqtt_broker, 1883, callback, ethClient);


// MQTT reconnection

unsigned long lastReconnectAttempt = 0;

boolean reconnect() {
  if (mqtt_client.connect(mqtt_client_name)) {
	  
    // Once connected, (re)subscribe to relay topics
    mqtt_client.subscribe(_Topic1);
    mqtt_client.subscribe(_Topic2);
    mqtt_client.subscribe(_Topic3);
    mqtt_client.subscribe(_Topic4);
  }
  return mqtt_client.connected();
}


// Callback function for message return from subscription
void callback(char* topic, byte* payload, unsigned int length) {

  /// Check if received 0
  boolean high = payload[0] != 48;
  
  /// Switch by Topic and setting relay state 
  if(String(topic) == String(_Topic1)){
    high ? digitalWrite(5, 1) : digitalWrite(5, 0);
  } 
  if(String(topic) == String(_Topic2)){
    high ? digitalWrite(6, 1) : digitalWrite(6, 0);
  } 
  if(String(topic) == String(_Topic3)){
    high ? digitalWrite(7, 1) : digitalWrite(7, 0);
  }   
  if(String(topic) == String(_Topic4)){
    high ? digitalWrite(8, 1) : digitalWrite(8, 0);
  }
}

void Setup_Pins() {
	pinMode(5, OUTPUT); digitalWrite(5, 0);
	pinMode(6, OUTPUT); digitalWrite(6, 0);
	pinMode(7, OUTPUT); digitalWrite(7, 0);
	pinMode(8, OUTPUT); digitalWrite(8, 0);
}

char* getPinValue(int pin, int *current)
{
  *current = digitalRead(pin);
  
  if(*current == HIGH){
    return (char *) State1;
  }else{
    return (char *) State0;
  }
}

void setup()
{
  // I/O initialization
  Setup_Pins();

  // Ethernet initialization
  if (Ethernet.begin(mac) == 0) {
    // Failed to configure Ethernet using DHCP
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  delay(1500); // Leave some time for Ethernet shield to initialize

  // Assigning the gateway address to the MQTT broker
  mqtt_broker = Ethernet.gatewayIP();
}

void loop()
{
  unsigned long now = millis();
  
  if (!mqtt_client.connected()) {
    if ((unsigned long)(now - lastReconnectAttempt) > 5000) {
      lastReconnectAttempt = now;
      
      // Attempt to reconnect
      if (reconnect()) {
        // lastReconnectAttempt = 0; /// Not sure this is needed
        lastReconnectAttempt -= 5000;
      }
    }
  } else {

    // Sending data on every change or every 60s ...

    if ((unsigned long)(now - lastUpdate) > 60000) {

      lastUpdate = now;

      mqtt_client.publish(_SendTopic1, getPinValue(A2, &A2current));
      mqtt_client.publish(_SendTopic2, getPinValue(A3, &A3current));
      mqtt_client.publish(_SendTopic3, getPinValue(A4, &A4current));
      mqtt_client.publish(_SendTopic4, getPinValue(A5, &A5current));  

      mqtt_client.publish(_SendTopicRel1, getPinValue(5, &R5current));
      mqtt_client.publish(_SendTopicRel2, getPinValue(6, &R6current));
      mqtt_client.publish(_SendTopicRel3, getPinValue(7, &R7current));
      mqtt_client.publish(_SendTopicRel4, getPinValue(8, &R8current));

    } else {
      int reading;

      if ((reading = digitalRead(A2)) != A2current)
        mqtt_client.publish(_SendTopic1, getPinValue(A2, &A2current));
      if ((reading = digitalRead(A3)) != A3current)
        mqtt_client.publish(_SendTopic2, getPinValue(A3, &A3current));
      if ((reading = digitalRead(A4)) != A4current)
        mqtt_client.publish(_SendTopic3, getPinValue(A4, &A4current));
      if ((reading = digitalRead(A5)) != A5current)
        mqtt_client.publish(_SendTopic4, getPinValue(A5, &A5current));

      if ((reading = digitalRead(5)) != R5current)
        mqtt_client.publish(_SendTopicRel1, getPinValue(5, &R5current));
      if ((reading = digitalRead(6)) != R6current)
        mqtt_client.publish(_SendTopicRel2, getPinValue(6, &R6current));
      if ((reading = digitalRead(7)) != R7current)
        mqtt_client.publish(_SendTopicRel3, getPinValue(7, &R7current));
      if ((reading = digitalRead(8)) != R8current)
        mqtt_client.publish(_SendTopicRel4, getPinValue(8, &R8current));
    }
    
    mqtt_client.loop();
  }
}
