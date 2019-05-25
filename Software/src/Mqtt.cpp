#include "Mqtt.h"

#include "JsonConfiguration.h"
#include "ATM90E32.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

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
    // reconnect();
  }
  clientMqtt.loop();
}

void Mqtt::publishMonitoringData()
{
  metering line;

  /* Update data from monitoring IC */
  Monitoring.handle();

  if (Configuration._mode == MODE_MONO) {
    /* Send Line A */
    line = Monitoring.getLineA();
    clientMqtt.publish(String(Configuration._hostname + "/lineA/voltage").c_str(), String(line.voltage).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/current").c_str(), String(line.current).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/power").c_str(), String(line.power).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/conso").c_str(), String(line.conso).c_str());

    /* Send Line B */
    line = Monitoring.getLineB();
    clientMqtt.publish(String(Configuration._hostname + "/lineB/voltage").c_str(), String(line.voltage).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineB/current").c_str(), String(line.current).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineB/power").c_str(), String(line.power).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineB/conso").c_str(), String(line.conso).c_str());

    /* Send Line C */
    line = Monitoring.getLineC();
    clientMqtt.publish(String(Configuration._hostname + "/lineC/voltage").c_str(), String(line.voltage).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineC/current").c_str(), String(line.current).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineC/power").c_str(), String(line.power).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineC/conso").c_str(), String(line.conso).c_str());
  }
  else if (Configuration._mode == MODE_TRI_1) {
    line = Monitoring.getLineA();

    /* Send Total Line */
    clientMqtt.publish(String(Configuration._hostname + "/lineA/voltage").c_str(), String(line.voltage).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/current").c_str(), String(line.current*3).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/power").c_str(), String(line.power*3).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/conso").c_str(), String(line.conso*3).c_str());
  }
  else if (Configuration._mode == MODE_TRI_2) {
    line = Monitoring.getLineA();
    metering lineC = Monitoring.getLineC();

    /* Send Total Line */
    clientMqtt.publish(String(Configuration._hostname + "/lineA/voltage").c_str(), String(line.voltage).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/current").c_str(), String(line.current*2 + lineC.current).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/power").c_str(), String(line.power*2 + lineC.power).c_str());
    clientMqtt.publish(String(Configuration._hostname + "/lineA/conso").c_str(), String(line.conso*2 + lineC.conso).c_str());
  }

  /* Send Frequency */
  clientMqtt.publish(String(Configuration._hostname + "/frequency").c_str(), String(Monitoring.getFrequency()).c_str());
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!clientMqtt.connected()) {
    if ((millis() - tick) >= 5000) {
      Serial.print("Attempting MQTT connection...");
      // Create a random clientMqtt ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (clientMqtt.connect(clientId.c_str())) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        clientMqtt.publish(String(Configuration._hostname + "/status").c_str(), "online");
        clientMqtt.publish(String(Configuration._hostname + "/relay").c_str(), String(0).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/mode").c_str(), String(Configuration._mode).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/version").c_str(), String(VERSION).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/ip").c_str(), WiFi.localIP().toString().c_str());
        // ... and resubscribe
        clientMqtt.subscribe(String(Configuration._hostname + "/set/#").c_str());
      } else {
        Serial.print("failed, rc=");
        Serial.print(clientMqtt.state());
        Serial.println(" try again in 5 seconds");
        tick = millis();
      }
    }
  }
}

void Mqtt::callback(char* topic, uint8_t* payload, unsigned int length)
{
  String data;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0 ; i < length ; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println();

  String topicStr(topic);
  topicStr.remove(0, topicStr.lastIndexOf('/')+1);

  if (topicStr == String("relay")) {
    int status = data.toInt();
    digitalWrite(RELAY_PIN, status);
    Serial.println(String("set relay status to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay").c_str(), String(status).c_str());
  }
  else if (topicStr == String("timeIntervalUpdate")) {
    int time = data.toInt();
    Serial.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
  }
  else if (topicStr == String("mode")) {
    int mode = data.toInt();
    Serial.println(String("set mode to ") + String(mode));
    Configuration._mode = mode;
    Configuration.saveConfig();
    clientMqtt.publish(String(Configuration._hostname + "/mode").c_str(), String(Configuration._mode).c_str());
  }
  else if (topicStr == String("hostname")) {
    Serial.println("Change hostname to " + data);
    Configuration._hostname = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("restart")) {
    Serial.println("Restart ESP !!!");
    ESP.restart();
  }
  else if (topicStr == String("reset")) {
    Serial.println("Reset ESP and restart !!!");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }
  else if (topicStr == String("resetAllConso")) {
    Serial.println("Reset All conso");
    Monitoring.resetAllConso();
    publishMonitoringData();
  }
  else {
    Serial.println("Unknow command");
  }
}


#if !defined(NO_GLOBAL_INSTANCES) 
Mqtt MqttClient;
#endif
