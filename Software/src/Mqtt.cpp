#include "Mqtt.h"

#include "JsonConfiguration.h"

#include <ESP8266WiFi.h>

WiFiClient espClient;

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

Mqtt::Mqtt()
{
  clientMqtt.setClient(espClient);
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup()
{
  clientMqtt.setServer(Configuration._mqttIpServer.c_str(), Configuration._mqttPortServer);
  clientMqtt.setCallback([this] (char* topic, uint8_t* payload, unsigned int length) { this->callback(topic, payload, length); });
}

void Mqtt::handle()
{
  if (!clientMqtt.connected()) {
    reconnect();
  }
  clientMqtt.loop();
}

bool Mqtt::publish(const char* topic, const char* payload)
{
  return clientMqtt.publish(topic, payload);
}

void Mqtt::reconnect()
{
  // Loop until we're reconnected
  while (!clientMqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random clientMqtt ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (clientMqtt.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      clientMqtt.publish("outTopic", "hello world");
      // ... and resubscribe
      clientMqtt.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Mqtt::callback(char* topic, uint8_t* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0 ; i < length ; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES) 
Mqtt MqttClient;
#endif
