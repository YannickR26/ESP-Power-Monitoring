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

void Mqtt::publish(String topic, String body)
{
  clientMqtt.publish(String(Configuration._hostname + topic).c_str(), String(body).c_str());
}

void Mqtt::publishMonitoringData()
{
  metering line;

  /* Send Status */
  publish(String("/relay"), String(digitalRead(RELAY_PIN)));
  publish(String("/timeIntervalUpdate"), String(Configuration._timeSendData));
  publish(String("/mode"), String(Configuration._mode));
  publish(String("/version"), String(VERSION));
  publish(String("/build"), String(String(__DATE__) + " " + String(__TIME__)));
  publish(String("/ip"), WiFi.localIP().toString());

  /* Send Line A */
  line = Monitoring.getLineA();
  publish(String("/lineA/voltage"), String(line.voltage));
  publish(String("/lineA/current"), String(line.current));
  publish(String("/lineA/power"), String(line.power));
  publish(String("/lineA/cosPhy"), String(line.cosPhy));
  publish(String("/lineA/conso"), String(line.conso));

  /* Send Line B */
  line = Monitoring.getLineB();
  publish(String("/lineB/voltage"), String(line.voltage));
  publish(String("/lineB/current"), String(line.current));
  publish(String("/lineB/power"), String(line.power));
  publish(String("/lineB/cosPhy"), String(line.cosPhy));
  publish(String("/lineB/conso"), String(line.conso));

  /* Send Line C */
  line = Monitoring.getLineC();
  publish(String("/lineC/voltage"), String(line.voltage));
  publish(String("/lineC/current"), String(line.current));
  publish(String("/lineC/power"), String(line.power));
  publish(String("/lineC/cosPhy"), String(line.cosPhy));
  publish(String("/lineC/conso"), String(line.conso));

  /* Send Frequency */
  publish(String("/frequency"), String(Monitoring.GetFrequency()));
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
        publish(String("/relay"), String(digitalRead(RELAY_PIN)));
        publish(String("/timeIntervalUpdate"), String(Configuration._timeSendData));
        publish(String("/mode"), String(Configuration._mode));
        publish(String("/current"), String(Configuration._iGain));
        publish(String("/version"), String(VERSION));
        publish(String("/build"), String(String(__DATE__) + " " + String(__TIME__)));
        publish(String("/ip"), WiFi.localIP().toString());
        // ... and resubscribe
        clientMqtt.subscribe(String("/set/#").c_str());
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
    publish(String("/relay"), String(status));
  }
  else if (topicStr == String("timeIntervalUpdate")) {
    int time = data.toInt();
    Log.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    publish(String("/timeIntervalUpdate"), String(Configuration._timeSendData));
  }
  else if (topicStr == String("mode")) {
    int mode = data.toInt();
    Log.println(String("set mode to ") + String(mode));
    Configuration._mode = mode;
    Configuration.saveConfig();
    publish(String("/mode"), String(Configuration._mode));
  }
  else if (topicStr == String("current")) {
    int current = data.toInt();
    Log.println(String("set current to ") + String(current) + String("V"));
    Configuration._iGain = current;
    Configuration.saveConfig();
    publish(String("/current"), String(Configuration._iGain));
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
  else if (topicStr == String("consoA")) {
    Log.println("Set conso A to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineA(conso);
  }
  else if (topicStr == String("consoB")) {
    Log.println("Set conso B to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineB(conso);
  }
  else if (topicStr == String("consoC")) {
    Log.println("Set conso C to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineC(conso);
  }
  else {
    Log.println("Unknow command");
    return;
  }

  Log.println("Save data");
  Configuration._consoA = Monitoring.getLineA().conso;
  Configuration._consoB = Monitoring.getLineB().conso;
  Configuration._consoC = Monitoring.getLineC().conso;
  Configuration.saveConfig();
}


#if !defined(NO_GLOBAL_INSTANCES) 
Mqtt MqttClient;
#endif
