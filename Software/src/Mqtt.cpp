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
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup()
{
  clientMqtt.setClient(espClient);
  clientMqtt.setServer(Configuration._mqttIpServer.c_str(), Configuration._mqttPortServer);
  clientMqtt.setCallback([this](char *topic, uint8_t *payload, unsigned int length) { this->callback(topic, payload, length); });
}

void Mqtt::handle()
{
  if (!clientMqtt.connected())
  {
    reconnect();
  }
  if (!clientMqtt.loop())
  {
    Log.println("Error with MQTT Loop !");
  }
}

bool Mqtt::publish(String topic, String body)
{
  return clientMqtt.publish(String(Configuration._hostname + "/" + topic).c_str(), String(body).c_str());
}

bool Mqtt::subscribe(String topic)
{
  return clientMqtt.subscribe(String(Configuration._hostname + "/" + topic).c_str());
}

void Mqtt::log(String level, String str)
{
  publish("log/" + level, str);
}

bool Mqtt::publishMonitoringData()
{
  metering *line;
  bool ret = true;

  /* Send Status */
  ret &= publish(String("relay"), String(digitalRead(RELAY_PIN)));
  ret &= publish(String("timeSendData"), String(Configuration._timeSendData));
  ret &= publish(String("timeSaveData"), String(Configuration._timeSaveData));
  ret &= publish(String("mode"), String(Configuration._mode));
  ret &= publish(String("currentClampA"), String(Configuration._currentClampA));
  ret &= publish(String("currentClampB"), String(Configuration._currentClampB));
  ret &= publish(String("currentClampC"), String(Configuration._currentClampC));
  // ret &= publish(String("version"), String(VERSION));
  // ret &= publish(String("build"), String(String(__DATE__) + " " + String(__TIME__)));
  // ret &= publish(String("ip"), WiFi.localIP().toString());

  /* Send Line A */
  line = Monitoring.getLineA();
  ret &= publish(String(Configuration._nameA + "/voltage"), String(line->voltage));
  ret &= publish(String(Configuration._nameA + "/current"), String(line->current));
  ret &= publish(String(Configuration._nameA + "/power"), String(line->power));
  ret &= publish(String(Configuration._nameA + "/cosPhy"), String(line->cosPhy));
  ret &= publish(String(Configuration._nameA + "/conso"), String(line->conso));

  // /* Send Line B */
  line = Monitoring.getLineB();
  ret &= publish(String(Configuration._nameB + "/voltage"), String(line->voltage));
  ret &= publish(String(Configuration._nameB + "/current"), String(line->current));
  ret &= publish(String(Configuration._nameB + "/power"), String(line->power));
  ret &= publish(String(Configuration._nameB + "/cosPhy"), String(line->cosPhy));
  ret &= publish(String(Configuration._nameB + "/conso"), String(line->conso));

  // /* Send Line C */
  line = Monitoring.getLineC();
  ret &= publish(String(Configuration._nameC + "/voltage"), String(line->voltage));
  ret &= publish(String(Configuration._nameC + "/current"), String(line->current));
  ret &= publish(String(Configuration._nameC + "/power"), String(line->power));
  ret &= publish(String(Configuration._nameC + "/cosPhy"), String(line->cosPhy));
  ret &= publish(String(Configuration._nameC + "/conso"), String(line->conso));

  /* Send Frequency */
  ret &= publish(String("frequency"), String(Monitoring.GetFrequency()));

  return ret;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!clientMqtt.connected())
  {
    if ((millis() - tick) >= 5000)
    {
      Log.print("Attempting MQTT connection...");
      // Create a random clientMqtt ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (clientMqtt.connect(clientId.c_str()))
      {
        Log.println("connected");
        // Once connected, publish an announcement...
        publish(String("version"), String(VERSION));
        publish(String("build"), String(String(__DATE__) + " " + String(__TIME__)));
        publish(String("ip"), WiFi.localIP().toString());
        publish(String("relay"), String(digitalRead(RELAY_PIN)));
        publish(String("timeSendData"), String(Configuration._timeSendData));
        publish(String("timeSaveData"), String(Configuration._timeSaveData));
        publish(String("mode"), String(Configuration._mode));
        publish(String("currentClampA"), String(Configuration._currentClampA));
        publish(String("currentClampB"), String(Configuration._currentClampB));
        publish(String("currentClampC"), String(Configuration._currentClampC));
        // ... and resubscribe
        subscribe(String("/set/#"));
      }
      else
      {
        Log.print("failed, rc=");
        Log.print(String(clientMqtt.state()));
        Log.println(" try again in 5 seconds");
        tick = millis();
      }
    }
  }
}

void Mqtt::callback(char *topic, uint8_t *payload, unsigned int length)
{
  String data;

  Log.print("Message arrived [");
  Log.print(topic);
  Log.print("] ");
  for (unsigned int i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  Log.println(data);

  String topicStr(topic);
  topicStr.remove(0, topicStr.lastIndexOf('/') + 1);

  if (topicStr == String("relay"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_PIN, status);
    Log.println(String("set relay status to ") + String(status));
    publish(String("relay"), String(status));
  }
  else if (topicStr == String("timeSendData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    publish(String("timeSendData"), String(Configuration._timeSendData));
  }
  else if (topicStr == String("timeSaveData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSaveData to: ") + String(time));
    Configuration._timeSaveData = time;
    Configuration.saveConfig();
    publish(String("timeSaveData"), String(Configuration._timeSaveData));
  }
  else if (topicStr == String("mode"))
  {
    int mode = data.toInt();
    Log.println(String("set mode to ") + String(mode));
    Configuration._mode = mode;
    Configuration.saveConfig();
    publish(String("mode"), String(Configuration._mode));
  }
  else if (topicStr == String("currentClampA"))
  {
    int current = data.toInt();
    Log.println(String("set currentClampA to ") + String(current) + String("A"));
    Configuration._currentClampA = current;
    Configuration.saveConfig();
    publish(String("currentClampA"), String(Configuration._currentClampA));
  }
  else if (topicStr == String("currentClampB"))
  {
    int current = data.toInt();
    Log.println(String("set currentClampB to ") + String(current) + String("A"));
    Configuration._currentClampB = current;
    Configuration.saveConfig();
    publish(String("currentClampB"), String(Configuration._currentClampB));
  }
  else if (topicStr == String("currentClampC"))
  {
    int current = data.toInt();
    Log.println(String("set currentClampC to ") + String(current) + String("A"));
    Configuration._currentClampC = current;
    Configuration.saveConfig();
    publish(String("currentClampC"), String(Configuration._currentClampC));
  }
  else if (topicStr == String("hostname"))
  {
    Log.println("Change hostname to " + data);
    Configuration._hostname = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("restart"))
  {
    Log.println("Restart ESP !!!");
    ESP.restart();
  }
  else if (topicStr == String("reset"))
  {
    Log.println("Reset ESP and restart !!!");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }
  else if (topicStr == String("resetAllConso"))
  {
    Log.println("Reset All conso");
    Monitoring.resetConsoLineA();
    Monitoring.resetConsoLineB();
    Monitoring.resetConsoLineC();
    publishMonitoringData();
  }
  else if (topicStr == String("nameA"))
  {
    Log.println("Set name A to " + data);
    Configuration._nameA = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("nameB"))
  {
    Log.println("Set name B to " + data);
    Configuration._nameB = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("nameC"))
  {
    Log.println("Set name C to " + data);
    Configuration._nameC = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("consoA"))
  {
    Log.println("Set conso A to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineA(conso);
  }
  else if (topicStr == String("consoB"))
  {
    Log.println("Set conso B to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineB(conso);
  }
  else if (topicStr == String("consoC"))
  {
    Log.println("Set conso C to " + data);
    int conso = data.toInt();
    Monitoring.setConsoLineC(conso);
  }
  else
  {
    Log.println("Unknow command");
    return;
  }

  Log.println("Save data");
  Configuration._consoA = Monitoring.getLineA()->conso;
  Configuration._consoB = Monitoring.getLineB()->conso;
  Configuration._consoC = Monitoring.getLineC()->conso;
  Configuration.saveConfig();
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
