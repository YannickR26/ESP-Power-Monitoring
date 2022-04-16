#include "Mqtt.h"

#include "JsonConfiguration.h"
#include "ATM90E32.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiManager.h>
#include "Logger.h"
#include "SimpleRelay.h"

extern SimpleRelay relay;

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
  _clientMqtt.setClient(espClient);
  _clientMqtt.setServer(Configuration._mqttIpServer.c_str(), Configuration._mqttPortServer);
  _clientMqtt.setCallback([this](char *topic, uint8_t *payload, unsigned int length) { this->callback(topic, payload, length); });
  startedAt = String(Log.getDateTimeString());
}

void Mqtt::handle()
{
  if (!_clientMqtt.connected())
  {
    reconnect();
  }
  if (!_clientMqtt.loop())
  {
    Log.println("Error with MQTT Loop !");
  }
}

bool Mqtt::publish(String topic, String body)
{
  return _clientMqtt.publish(String(Configuration._mqttTopic + topic).c_str(), String(body).c_str());
}

bool Mqtt::publish(String topic, JsonDocument &doc)
{
  bool ret;

  ret = _clientMqtt.beginPublish(String(Configuration._mqttTopic + topic).c_str(), measureJson(doc), false);
  serializeJson(doc, _clientMqtt);
  _clientMqtt.endPublish();

  return ret;
}

bool Mqtt::subscribe(String topic)
{
  return _clientMqtt.subscribe(String(Configuration._mqttTopic + topic).c_str());
}

void Mqtt::log(String level, String str)
{
  publish("log/" + level, str);
}

bool Mqtt::publishMonitoringData()
{
  metering *line;
  DynamicJsonDocument doc(1024);

  bool ret = true;

  /* Send Line A */
  line = Monitoring.getLineA();

  doc.clear();
  doc["voltage"] = String(line->voltage);
  doc["current"] = String(line->current);
  doc["power"] = String(line->power);
  doc["cosPhy"] = String(line->cosPhy);
  doc["conso"] = String(line->conso);

  ret &= publish(String(Configuration._nameA), doc);

  /* Send Line B */
  line = Monitoring.getLineB();

  doc.clear();
  doc["voltage"] = String(line->voltage);
  doc["current"] = String(line->current);
  doc["power"] = String(line->power);
  doc["cosPhy"] = String(line->cosPhy);
  doc["conso"] = String(line->conso);

  ret &= publish(String(Configuration._nameB), doc);

  /* Send Line C */
  line = Monitoring.getLineC();

  doc.clear();
  doc["voltage"] = String(line->voltage);
  doc["current"] = String(line->current);
  doc["power"] = String(line->power);
  doc["cosPhy"] = String(line->cosPhy);
  doc["conso"] = String(line->conso);

  ret &= publish(String(Configuration._nameC), doc);

  /* Send Frequency */
  ret &= publish(String("frequency"), String(Monitoring.GetFrequency()));

  return ret;
}

bool Mqtt::publishConfiguration()
{
    // Send Config to Mqtt
    DynamicJsonDocument doc(1024);

    Configuration.encodeToJson(doc);

    return MqttClient.publish(String("configuration"), doc);
}

bool Mqtt::publishInformations()
{
    // Send Config to Mqtt
    DynamicJsonDocument doc(128);

    doc["version"] = VERSION;
    doc["buildDate"] = BUILD_DATE;
    doc["startedAt"] = startedAt;
    char *time = Log.getDateTimeString();
    doc["connectedFrom"] = String(time);

    return MqttClient.publish(String("information"), doc);
}


/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!_clientMqtt.connected())
  {
    if ((millis() - tick) >= 5000)
    {
      Log.print("Attempting MQTT connection...");
      // Create a random clientMqtt ID
      String clientId = Configuration._hostname + String(random(0xffff), HEX);
      // Attempt to connect
      bool res = _clientMqtt.connect(clientId.c_str(), Configuration._mqttUsername.c_str(), Configuration._mqttPassword.c_str(), NULL, 1, 0, NULL, true);

      if (res)
      {
        Log.println("connected");
        // Once connected, publish an announcement...
        publish(String("relay"), String(digitalRead(RELAY_PIN)));
        publishConfiguration();
        publishInformations();
        publishMonitoringData();
        // ... and resubscribe
        subscribe(String("set/#"));
      }
      else
      {
        Log.print("failed, rc=");
        Log.print(String(_clientMqtt.state()));
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
    int state = data.toInt();
    relay.setState(state);
  }
  else if (topicStr == String("timeSendData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    publish(String("timeSendData"), String(Configuration._timeSendData));
  }
  else if (topicStr == String("timeSaveData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSaveData to: ") + String(time));
    Configuration._timeSaveData = time;
    publish(String("timeSaveData"), String(Configuration._timeSaveData));
  }
  else if (topicStr == String("mode"))
  {
    int mode = data.toInt();
    Log.println(String("set mode to ") + String(mode));
    Configuration._mode = mode;
    publish(String("mode"), String(Configuration._mode));
  }
  else if (topicStr == String("currentClampA"))
  {
    int current = data.toInt();
    Log.println(String("set currentClampA to ") + String(current) + String("A"));
    Configuration._currentClampA = current;
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
    publish(String("currentClampC"), String(Configuration._currentClampC));
  }
  else if (topicStr == String("hostname"))
  {
    Log.println("Change hostname to " + data);
    Configuration._hostname = data;
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
  }
  else if (topicStr == String("nameB"))
  {
    Log.println("Set name B to " + data);
    Configuration._nameB = data;
  }
  else if (topicStr == String("nameC"))
  {
    Log.println("Set name C to " + data);
    Configuration._nameC = data;
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
  else if (topicStr == String("timeoutRelay"))
  {
    Log.println("Set timeoutRelay to " + data + " s");
    Configuration._timeoutRelay = data.toInt();
    publish(String("timeoutRelay"), String(Configuration._timeoutRelay));
  }
  else
  {
    Log.println("Unknow command");
    return;
  }

  Configuration._consoA = Monitoring.getLineA()->conso;
  Configuration._consoB = Monitoring.getLineB()->conso;
  Configuration._consoC = Monitoring.getLineC()->conso;
  Configuration.saveConfig();
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
