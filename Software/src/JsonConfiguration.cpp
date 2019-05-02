#include <FS.h>
#include <ArduinoJson.h>

#include "JsonConfiguration.h"

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
    Serial.println("failed to initialize SPIFFS");
  }

	if (!readConfig()) {
		Serial.println("Invalid configuration values, restoring default values");
		restoreDefault();
    return;
	}

	Serial.printf("\thostname : %s\n", _hostname.c_str());
	Serial.printf("\tmqttIpServer : %s\n", _mqttIpServer.c_str());
	Serial.printf("\tmqttPortServer : %s\n", _mqttPortServer);
	Serial.printf("\ttimeUpdateNTP : %d\n", _timeUpdateNtp);
	Serial.printf("\ttimeSendData : %d\n", _timeSendData);
  Serial.printf("\ttimeUpdateScreen : %d\n", _timeUpdateScreen);
	Serial.printf("\tmode : %d\n", _mode);
}

bool JsonConfiguration::readConfig()
{
  Serial.println("Read Configuration file from SPIFFS...");
  
  // Open file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) 
  {
    Serial.println("Failed to open config file");
    return false;
  }
  
  // Allocate a buffer to store contents of the file.
  StaticJsonDocument<512> doc;
  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration"));
  }
  
  _hostname           = doc["hostname"] | DEFAULT_HOSTNAME;
  _mqttIpServer       = doc["mqttIpServer"] | DEFAULT_MQTTIPSERVER; 
  _mqttPortServer     = doc["mqttPortServer"] | DEFAULT_MQTTPORTSERVER; 
  _timeUpdateNtp      = doc["ntpUpdateIntervale"] | DEFAULT_NTP_UPDATE_INTERVAL_SEC;
  _timeUpdateScreen   = doc["screenUpdateIntervale"] | DEFAULT_SCREEN_UPDATE_INTERVAL_SEC;
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
  StaticJsonDocument<512> doc;
 
	doc["hostname"]               = _hostname;
  doc["mqttIpServer"]           = _mqttIpServer;
  doc["mqttPortServer"]         = _mqttPortServer;
	doc["ntpUpdateIntervale"]     = _timeUpdateNtp;
	doc["screenUpdateIntervale"]  = _timeUpdateScreen;
	doc["SendDataIntervale"]      = _timeSendData;
	doc["mode"]                   = _mode;
	doc["namePhaseA"]             = _namePhaseA;
	doc["namePhaseB"]             = _namePhaseB;
	doc["namePhaseC"]             = _namePhaseC;
  
	File configFile = SPIFFS.open("/config.json", "w");
	if (!configFile) {
		Serial.println("Failed to open config file for writing");
		return false;
	}
  
  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
    return false;
  }

  configFile.close();
	
	return true;
}

void JsonConfiguration::restoreDefault()
{  
	_hostname           = DEFAULT_HOSTNAME;
  _mqttIpServer       = DEFAULT_MQTTIPSERVER; 
  _mqttPortServer     = DEFAULT_MQTTPORTSERVER; 
	_timeUpdateNtp      = DEFAULT_NTP_UPDATE_INTERVAL_SEC;
	_timeSendData       = DEFAULT_SEND_DATA_INTERVAL_SEC;
	_timeUpdateScreen   = DEFAULT_SCREEN_UPDATE_INTERVAL_SEC;
  _mode               = MODE_MONO;
  _namePhaseA         = "PHASE A";
  _namePhaseB         = "PHASE B";
  _namePhaseC         = "PHASE C";

	saveConfig();
	Serial.println("configuration restored.");
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES) 
JsonConfiguration Configuration;
#endif
