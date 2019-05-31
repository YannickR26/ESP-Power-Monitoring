#include <FS.h>
#include <ArduinoJson.h>

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
  /* Initialize SPIFFS */
  if (!SPIFFS.begin()) {
    Log.println("failed to initialize SPIFFS");
  }

	if (!readConfig()) {
		Log.println("Invalid configuration values, restoring default values");
		restoreDefault();
	}

	Log.println(String("    hostname: ") + _hostname);
	Log.println(String("    mqttIpServer: ") + _mqttIpServer);
	Log.println(String("    mqttPortServer: ") + String(_mqttPortServer));
	Log.println(String("    timeUpdateNTP: ") + String(_timeUpdateNtp));
	Log.println(String("    timeSendData: ") + String(_timeSendData));
	Log.println(String("    mode: ") + String(_mode));
}

bool JsonConfiguration::readConfig()
{
  Log.println("Read Configuration file from SPIFFS...");
  
  // Open file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) 
  {
    Log.println("Failed to open config file");
    return false;
  }
  
  // Allocate a buffer to store contents of the file.
  // StaticJsonDocument<512> doc;
  DynamicJsonDocument doc(512);
  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Log.println(F("Failed to read file, using default configuration"));
  }
  
  _hostname           = doc["hostname"] | DEFAULT_HOSTNAME;
  _mqttIpServer       = doc["mqttIpServer"] | DEFAULT_MQTTIPSERVER; 
  _mqttPortServer     = doc["mqttPortServer"] | DEFAULT_MQTTPORTSERVER; 
  _timeUpdateNtp      = doc["ntpUpdateIntervale"] | DEFAULT_NTP_UPDATE_INTERVAL_SEC;
  _timeSendData       = doc["SendDataIntervale"] | DEFAULT_SEND_DATA_INTERVAL_SEC;
  _mode               = doc["mode"] | MODE_MONO;
  _namePhaseA         = doc["namePhaseA"] | "Phase A";
  _namePhaseB         = doc["namePhaseB"] | "Phase B";
  _namePhaseC         = doc["namePhaseC"] | "Phase C";

  configFile.close();

  return true;
}

bool JsonConfiguration::saveConfig()
{
  // StaticJsonDocument<512> doc;
  DynamicJsonDocument doc(512);
 
	doc["hostname"]               = _hostname;
  doc["mqttIpServer"]           = _mqttIpServer;
  doc["mqttPortServer"]         = _mqttPortServer;
	doc["ntpUpdateIntervale"]     = _timeUpdateNtp;
	doc["SendDataIntervale"]      = _timeSendData;
	doc["mode"]                   = _mode;
	doc["namePhaseA"]             = _namePhaseA;
	doc["namePhaseB"]             = _namePhaseB;
	doc["namePhaseC"]             = _namePhaseC;
  
	File configFile = SPIFFS.open("/config.json", "w");
	if (!configFile) {
		Log.println("Failed to open config file for writing");
		return false;
	}
  
  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Log.println(F("Failed to write to file"));
    return false;
  }

  configFile.close();

  
	Log.println("Save config successfully");
	Log.println(String("    hostname: ") + _hostname);
	Log.println(String("    mqttIpServer: ") + _mqttIpServer);
	Log.println(String("    mqttPortServer: ") + String(_mqttPortServer));
	Log.println(String("    timeUpdateNTP: ") + String(_timeUpdateNtp));
	Log.println(String("    timeSendData: ") + String(_timeSendData));
	Log.println(String("    mode: ") + String(_mode));
	
	return true;
}

void JsonConfiguration::restoreDefault()
{  
	_hostname           = DEFAULT_HOSTNAME;
  _mqttIpServer       = DEFAULT_MQTTIPSERVER; 
  _mqttPortServer     = DEFAULT_MQTTPORTSERVER; 
	_timeUpdateNtp      = DEFAULT_NTP_UPDATE_INTERVAL_SEC;
	_timeSendData       = DEFAULT_SEND_DATA_INTERVAL_SEC;
  _mode               = MODE_MONO;
  _namePhaseA         = "Phase A";
  _namePhaseB         = "Phase B";
  _namePhaseC         = "Phase C";

	saveConfig();
	Log.println("configuration restored.");
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES) 
JsonConfiguration Configuration;
#endif
