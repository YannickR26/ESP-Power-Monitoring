#include <LittleFS.h>

#include "JsonConfiguration.h"
#include "Logger.h"

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
  /* Initialize LittleFS */
  if (!LittleFS.begin())
  {
    Log.println("failed to initialize LittleFS, try to format");
    LittleFS.format();
    if (!LittleFS.begin())
    {
      Log.println("definitely failed to initialize LittleFS !");
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
  Log.println(String("    mqttIpServer: ") + _mqttIpServer);
  Log.println(String("    mqttPortServer: ") + String(_mqttPortServer));
  Log.println(String("    timeSaveData: ") + String(_timeSaveData));
  Log.println(String("    timeSendData: ") + String(_timeSendData));
  Log.println(String("    mode: ") + String(_mode));
  Log.println(String("    nameA: ") + String(_nameA));
  Log.println(String("    nameB: ") + String(_nameB));
  Log.println(String("    nameC: ") + String(_nameC));
  Log.println(String("    currentClampA: ") + String(_currentClampA));
  Log.println(String("    currentClampB: ") + String(_currentClampB));
  Log.println(String("    currentClampC: ") + String(_currentClampC));
  Log.println(String("    consoA: ") + String(_consoA));
  Log.println(String("    consoB: ") + String(_consoB));
  Log.println(String("    consoC: ") + String(_consoC));
}

bool JsonConfiguration::readConfig()
{
  Log.println("Read Configuration file from LittleFS...");

  // Open file
  File configFile = LittleFS.open("/config.json", "r");
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
  // StaticJsonDocument<512> doc;
  DynamicJsonDocument doc(1024);

  encodeToJson(doc);

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
  {
    Log.println("Failed to open config file for writing");
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0)
  {
    Log.println(F("Failed to write to file"));
    return false;
  }

  configFile.close();

  Log.println("Save config successfully");

  return true;
}

void JsonConfiguration::restoreDefault()
{
  _hostname = DEFAULT_HOSTNAME;
  _mqttIpServer = DEFAULT_MQTTIPSERVER;
  _mqttPortServer = DEFAULT_MQTTPORTSERVER;
  _timeSaveData = DEFAULT_SAVE_DATA_INTERVAL_SEC;
  _timeSendData = DEFAULT_SEND_DATA_INTERVAL_SEC;
  _mode = MODE_MONO;
  _nameA = "lineA";
  _nameB = "lineB";
  _nameC = "lineC";
  _currentClampA = DEFAULT_CURRENT_CLAMP;
  _currentClampB = DEFAULT_CURRENT_CLAMP;
  _currentClampC = DEFAULT_CURRENT_CLAMP;
  _consoA = 0;
  _consoB = 0;
  _consoC = 0;

  saveConfig();
  Log.println("configuration restored.");
}

uint8_t JsonConfiguration::encodeToJson(JsonDocument &_json)
{
  _json.clear();
  _json["hostname"] = _hostname;
  _json["mqttIpServer"] = _mqttIpServer;
  _json["mqttPortServer"] = _mqttPortServer;
  _json["timeSaveData"] = _timeSaveData;
  _json["timeSendData"] = _timeSendData;
  _json["mode"] = _mode;
  _json["nameA"] = _nameA;
  _json["nameB"] = _nameB;
  _json["nameC"] = _nameC;
  _json["currentClampA"] = _currentClampA;
  _json["currentClampB"] = _currentClampB;
  _json["currentClampC"] = _currentClampC;
  _json["consoA"] = _consoA;
  _json["consoB"] = _consoB;
  _json["consoC"] = _consoC;

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

  _hostname = doc["hostname"] | DEFAULT_HOSTNAME;
  _mqttIpServer = doc["mqttIpServer"] | DEFAULT_MQTTIPSERVER;
  _mqttPortServer = doc["mqttPortServer"] | DEFAULT_MQTTPORTSERVER;
  _timeSaveData = doc["timeSaveData"] | DEFAULT_SAVE_DATA_INTERVAL_SEC;
  _timeSendData = doc["timeSendData"] | DEFAULT_SEND_DATA_INTERVAL_SEC;
  _mode = doc["mode"].as<uint8_t>();
  _nameA = doc["nameA"] | "lineA";
  _nameB = doc["nameB"] | "lineB";
  _nameC = doc["nameC"] | "lineC";
  _currentClampA = doc["currentClampA"] | DEFAULT_CURRENT_CLAMP;
  _currentClampB = doc["currentClampB"] | DEFAULT_CURRENT_CLAMP;
  _currentClampC = doc["currentClampC"] | DEFAULT_CURRENT_CLAMP;
  _consoA = doc["consoA"].as<uint32_t>();
  _consoB = doc["consoB"].as<uint32_t>();
  _consoC = doc["consoC"].as<uint32_t>();

  return 0;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES)
JsonConfiguration Configuration;
#endif
