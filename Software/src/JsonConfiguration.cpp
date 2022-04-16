#if defined(ESP8266)
#include <LittleFS.h>
#define FS LittleFS
#elif defined(ESP32)
#include <SPIFFS.h>
#define FS SPIFFS
#endif

#include "JsonConfiguration.h"
#include "Logger.h"

#define PATH_FILE_CONFIG  "/config.json"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

JsonConfiguration::JsonConfiguration()
{
}

JsonConfiguration::~JsonConfiguration()
{
}

void JsonConfiguration::setup(void)
{
  /* Initialize FS */
  if (!FS.begin())
  {
    Log.println("failed to initialize FS, try to format, please wait...");
    FS.format();
    if (!FS.begin())
    {
      Log.println("definitely failed to initialize FS !");
      return;
    }
  }

  if (!readConfig())
  {
    Log.println("Invalid configuration values, restoring default values");
    restoreDefault();
  }

  print();
}

void JsonConfiguration::print(void)
{
  Log.println(String("Current configuration :"));
  Log.println(String("    hostname: ") + _hostname);
  Log.println(String("    timeSaveData: ") + String(_timeSaveData));
  Log.println(String("    timeSendData: ") + String(_timeSendData));
  Log.println(String("    timeoutRelay: ") + String(_timeoutRelay));
  Log.println(String("    stateRelay: ") + String(_stateRelay));
  Log.println(String("    mode: ") + String(_mode));
  Log.println(String("    MQTT: ") + String(_mqttEnable ? "Enable" : "Disable"));
  Log.println(String("      ipServer: ") + _mqttIpServer);
  Log.println(String("      portServer: ") + String(_mqttPortServer));
  Log.println(String("      username: ") + String(_mqttUsername));
  Log.println(String("      password: ") + String(_mqttPassword));
  Log.println(String("      topic: ") + String(_mqttTopic));
  Log.println(String("    lineA: ") + String(_enableA ? "Enable" : "Disable"));
  Log.println(String("      nameA: ") + String(_nameA));
  Log.println(String("      currentClampA: ") + String(_currentClampA));
  Log.println(String("      consoA: ") + String(_consoA));
  Log.println(String("    lineB: ") + String(_enableB ? "Enable" : "Disable"));
  Log.println(String("      nameB: ") + String(_nameB));
  Log.println(String("      currentClampB: ") + String(_currentClampB));
  Log.println(String("      consoB: ") + String(_consoB));
  Log.println(String("    lineC: ") + String(_enableC ? "Enable" : "Disable"));
  Log.println(String("      nameC: ") + String(_nameC));
  Log.println(String("      currentClampC: ") + String(_currentClampC));
  Log.println(String("      consoC: ") + String(_consoC));
}

bool JsonConfiguration::readConfig()
{
  Log.println("Read Configuration file from FS...");

  // Check if file Exist
  if (!FS.exists(PATH_FILE_CONFIG))
  {
    Log.println("config file not exist !");
    return false;
  }

  // Open file
  File configFile = FS.open(PATH_FILE_CONFIG, "r");
  if (!configFile)
  {
    Log.println("Failed to open config file");
    return false;
  }

  uint16_t size = configFile.size();

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  decodeJsonFromFile(buf.get());

  configFile.close();

  return true;
}

bool JsonConfiguration::saveConfig()
{
  DynamicJsonDocument doc(1024);

  Log.print("Try to save config... ");

  encodeToJson(doc);

  File configFile = FS.open(PATH_FILE_CONFIG, "w");
  if (!configFile)
  {
    Log.println("Error: Failed to open config file for writing");
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0)
  {
    Log.println(F("Error: Failed to write to file"));
    return false;
  }

  configFile.close();

  Log.println("Done !");

  return true;
}

void JsonConfiguration::restoreDefault()
{
  _hostname = DEFAULT_HOSTNAME;
  _mqttEnable = true;
  _mqttIpServer = DEFAULT_MQTTIPSERVER;
  _mqttPortServer = DEFAULT_MQTTPORTSERVER;
  _mqttUsername = "";
  _mqttPassword = "";
  _mqttTopic = _hostname + '/';
  _timeSaveData = DEFAULT_SAVE_DATA_INTERVAL_SEC;
  _timeSendData = DEFAULT_SEND_DATA_INTERVAL_SEC;
  _mode = MODE_MONO;
  _enableA = _enableB = _enableC = true;
  _nameA = "lineA";
  _nameB = "lineB";
  _nameC = "lineC";
  _currentClampA = _currentClampB = _currentClampC = DEFAULT_CURRENT_CLAMP;
  _consoA = _consoB = _consoC = 0;
  _timeoutRelay = 0;
  _stateRelay = 0;

  saveConfig();
  Log.println("configuration restored.");
}

uint8_t JsonConfiguration::encodeToJson(JsonDocument &_json)
{
  _json.clear();
  _json["hostname"] = _hostname;
  _json["timeSaveData"] = _timeSaveData;
  _json["timeSendData"] = _timeSendData;
  _json["timeoutRelay"] = _timeoutRelay;
  _json["mode"] = _mode;
  _json["stateRelay"] = _stateRelay;

  JsonObject mqtt = _json.createNestedObject("mqtt");
  mqtt["enable"] = _mqttEnable;
  mqtt["ipServer"] = _mqttIpServer;
  mqtt["portServer"] = _mqttPortServer;
  mqtt["username"] = _mqttUsername;
  mqtt["password"] = _mqttPassword;
  mqtt["topic"] = _mqttTopic;

  JsonObject lineA = _json.createNestedObject("lineA");
  lineA["enable"] = _enableA;
  lineA["name"] = _nameA;
  lineA["currentClamp"] = _currentClampA;
  lineA["conso"] = _consoA;

  JsonObject lineB = _json.createNestedObject("lineB");
  lineB["enable"] = _enableB;
  lineB["name"] = _nameB;
  lineB["currentClamp"] = _currentClampB;
  lineB["conso"] = _consoB;

  JsonObject lineC = _json.createNestedObject("lineC");
  lineC["enable"] = _enableC;
  lineC["name"] = _nameC;
  lineC["currentClamp"] = _currentClampC;
  lineC["conso"] = _consoC;

  return 0;
}

uint8_t JsonConfiguration::decodeJsonFromFile(const char *input)
{
  DynamicJsonDocument doc(1024);

  doc.clear();
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, input);
  if (error)
  {
    Serial.print("Failed to deserialize JSON, error: ");
    Serial.println(error.c_str());
    // restoreDefault();
    return -1;
  }

  if (!doc["hostname"].isNull())
    _hostname = doc["hostname"].as<String>();

  if (!doc["timeSaveData"].isNull())
    _timeSaveData = doc["timeSaveData"].as<uint16_t>();

  if (!doc["timeSendData"].isNull())
    _timeSendData = doc["timeSendData"].as<uint16_t>();

  if (!doc["mode"].isNull())
    _mode = doc["mode"].as<uint8_t>();

  if (!doc["timeoutRelay"].isNull())
    _timeoutRelay = doc["timeoutRelay"].as<float>();

  if (!doc["stateRelay"].isNull())
    _stateRelay = doc["stateRelay"].as<uint8_t>();

  JsonObject mqtt = doc["mqtt"];
  if (!mqtt.isNull())
  {
    if (!mqtt["enable"].isNull())
      _mqttEnable = mqtt["enable"].as<bool>();

    if (!mqtt["ipServer"].isNull())
      _mqttIpServer = mqtt["ipServer"].as<String>();

    if (!mqtt["portServer"].isNull())
      _mqttPortServer = mqtt["portServer"].as<uint16_t>();

    if (!mqtt["username"].isNull())
      _mqttUsername = mqtt["username"].as<String>();

    if (!mqtt["password"].isNull())
      _mqttPassword = mqtt["password"].as<String>();

    if (!mqtt["topic"].isNull())
      _mqttTopic = mqtt["topic"].as<String>();
  }

  JsonObject lineA = doc["lineA"];
  if (!lineA.isNull())
  {
    if (!lineA["enable"].isNull())
      _enableA = lineA["enable"].as<bool>();

    if (!lineA["name"].isNull())
      _nameA = lineA["name"].as<String>();

    if (!lineA["currentClamp"].isNull())
      _currentClampA = lineA["currentClamp"].as<uint8_t>();

    if (!lineA["conso"].isNull())
      _consoA = lineA["conso"].as<uint32_t>();
  }

  JsonObject lineB = doc["lineB"];
  if (!lineB.isNull())
  {
    if (!lineB["enable"].isNull())
      _enableB = lineB["enable"].as<bool>();

    if (!lineB["name"].isNull())
      _nameB = lineB["name"].as<String>();

    if (!lineB["currentClamp"].isNull())
      _currentClampB = lineB["currentClamp"].as<uint8_t>();

    if (!lineB["conso"].isNull())
      _consoB = lineB["conso"].as<uint32_t>();
  }

  JsonObject lineC = doc["lineC"];
  if (!lineC.isNull())
  {
    if (!lineC["enable"].isNull())
      _enableC = lineC["enable"].as<bool>();

    if (!lineC["name"].isNull())
      _nameC = lineC["name"].as<String>();

    if (!lineC["currentClamp"].isNull())
      _currentClampC = lineC["currentClamp"].as<uint8_t>();

    if (!lineC["conso"].isNull())
      _consoC = lineC["conso"].as<uint32_t>();
  }

  return 0;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES)
JsonConfiguration Configuration;
#endif
