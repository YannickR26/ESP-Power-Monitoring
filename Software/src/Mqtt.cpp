#include "Mqtt.h"

#include "JsonConfiguration.h"
#include "ATM90E32.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "Logger.h"

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

void Mqtt::publishMonitoringData()
{
  metering line;

  /* Send Status */
  clientMqtt.publish(String(Configuration._hostname + "/relay").c_str(), String(digitalRead(RELAY_PIN)).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/mode").c_str(), String(Configuration._mode).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/version").c_str(), String(VERSION).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/build").c_str(), String(String(__DATE__) + " " + String(__TIME__)).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/ip").c_str(), WiFi.localIP().toString().c_str());

  /* Send Line A */
  line = Monitoring.getLineA();
  clientMqtt.publish(String(Configuration._hostname + "/lineA/voltage").c_str(), String(line.voltage).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineA/current").c_str(), String(line.current).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineA/power").c_str(), String(line.power).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineA/cosPhy").c_str(), String(line.cosPhy).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineA/conso").c_str(), String(line.conso).c_str());

  /* Send Line B */
  line = Monitoring.getLineB();
  clientMqtt.publish(String(Configuration._hostname + "/lineB/voltage").c_str(), String(line.voltage).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineB/current").c_str(), String(line.current).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineB/power").c_str(), String(line.power).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineB/cosPhy").c_str(), String(line.cosPhy).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineB/conso").c_str(), String(line.conso).c_str());

  /* Send Line C */
  line = Monitoring.getLineC();
  clientMqtt.publish(String(Configuration._hostname + "/lineC/voltage").c_str(), String(line.voltage).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineC/current").c_str(), String(line.current).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineC/power").c_str(), String(line.power).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineC/cosPhy").c_str(), String(line.cosPhy).c_str());
  clientMqtt.publish(String(Configuration._hostname + "/lineC/conso").c_str(), String(line.conso).c_str());

  /* Send Frequency */
  clientMqtt.publish(String(Configuration._hostname + "/frequency").c_str(), String(Monitoring.GetFrequency()).c_str());
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!clientMqtt.connected()) {
    if ((millis() - tick) >= 5000) {
      Log.print("Attempting MQTT connection...");
      // Create a random clientMqtt ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (clientMqtt.connect(clientId.c_str())) {
        Log.println("connected");
        // Once connected, publish an announcement...
        clientMqtt.publish(String(Configuration._hostname + "/relay").c_str(), String(digitalRead(RELAY_PIN)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/mode").c_str(), String(Configuration._mode).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/version").c_str(), String(VERSION).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/build").c_str(), String(String(__DATE__) + " " + String(__TIME__)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/ip").c_str(), WiFi.localIP().toString().c_str());
        // ... and resubscribe
        clientMqtt.subscribe(String(Configuration._hostname + "/set/#").c_str());
      } else {
        Log.print("failed, rc=");
        Log.print(String(clientMqtt.state()));
        Log.println(" try again in 5 seconds");
        tick = millis();
      }
    }
  }
}

void Mqtt::callback(char* topic, uint8_t* payload, unsigned int length)
{
  String data;

  Log.print("Message arrived [");
  Log.print(topic);
  Log.print("] ");
  for (unsigned int i = 0 ; i < length ; i++) {
    Log.print(String(payload[i]));
    data += (char)payload[i];
  }
  Log.println();

  String topicStr(topic);
  topicStr.remove(0, topicStr.lastIndexOf('/')+1);

  if (topicStr == String("relay")) {
    int status = data.toInt();
    digitalWrite(RELAY_PIN, status);
    Log.println(String("set relay status to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay").c_str(), String(status).c_str());
  }
  else if (topicStr == String("timeIntervalUpdate")) {
    int time = data.toInt();
    Log.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
  }
  else if (topicStr == String("mode")) {
    int mode = data.toInt();
    Log.println(String("set mode to ") + String(mode));
    Configuration._mode = mode;
    Configuration.saveConfig();
    clientMqtt.publish(String(Configuration._hostname + "/mode").c_str(), String(Configuration._mode).c_str());
  }
  else if (topicStr == String("hostname")) {
    Log.println("Change hostname to " + data);
    Configuration._hostname = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("restart")) {
    Log.println("Restart ESP !!!");
    ESP.restart();
  }
  else if (topicStr == String("reset")) {
    Log.println("Reset ESP and restart !!!");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }
  else if (topicStr == String("resetAllConso")) {
    Log.println("Reset All conso");
    Monitoring.resetConsoLineA();
    Monitoring.resetConsoLineB();
    Monitoring.resetConsoLineC();
    publishMonitoringData();
  }
  else {
    Log.println("Unknow command");
  }
}


#if !defined(NO_GLOBAL_INSTANCES) 
Mqtt MqttClient;
#endif
