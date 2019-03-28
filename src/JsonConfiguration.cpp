#include <FS.h>
#include <ArduinoJson.h>

#include "JsonConfiguration.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

JsonConfiguration::JsonConfiguration()
{
  /* Initialize SPIFFS */
  if (!SPIFFS.begin()) {
    Serial.println("failed to initialize SPIFFS");
  }
}

JsonConfiguration::~JsonConfiguration()
{
}

void JsonConfiguration::setup(void)
{
  readConfig();

	if ((m_hostname.length() == 0) || (m_ftpLogin.length() == 0) || (m_ftpPasswd.length() == 0)) {
		Serial.println("Invalid configuration values, restoring default values");
		restoreDefault();
    return;
	}

	Serial.printf("\thostname : %s\n", m_hostname.c_str());
	Serial.printf("\tftpLogin : %s\n", m_ftpLogin.c_str());
	Serial.printf("\tftpPasswd : %s\n", m_ftpPasswd.c_str());
	Serial.printf("\ttimeUpdateNTP : %d\n", m_timeUpdateNtp);
	Serial.printf("\ttimeWriteLog : %d\n", m_timeWriteLog);
	Serial.printf("\ttumeUpdateScreen : %d\n", m_timeUpdateScreen);
	Serial.printf("\tmode : %d\n", m_mode);
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
  
  m_hostname          = doc["hostname"] | DEFAULT_HOSTNAME;
  m_ftpLogin          = doc["ftp.login"] | DEFAULT_FTP_USER;
  m_ftpPasswd         = doc["ftp.passwd"] | DEFAULT_FTP_PWD;
  m_timeUpdateNtp     = doc["ntpUpdateIntervale"] | DEFAULT_NTP_UPDATE_INTERVAL_SEC;
  m_timeWriteLog      = doc["writeLogIntervale"] | DEFAULT_LOG_WRITE_INTERVAL_SEC;
  m_timeUpdateScreen  = doc["screenUpdateIntervale"] | DEFAULT_SCREEN_UPDATE_INTERVAL_SEC;
  m_mode              = doc["mode"] | MODE_MONO;
  m_namePhaseA        = doc["namePhaseA"] | "Phase A";
  m_namePhaseB        = doc["namePhaseB"] | "Phase B";
  m_namePhaseC        = doc["namePhaseC"] | "Phase C";

  configFile.close();

  return true;
}

bool JsonConfiguration::saveConfig()
{
  StaticJsonDocument<512> doc;
 
	doc["hostname"]               = m_hostname;
	doc["ftp.login"]              = m_ftpLogin;
	doc["ftp.passwd"]             = m_ftpPasswd;
	doc["ntpUpdateIntervale"]     = m_timeUpdateNtp;
	doc["writeLogIntervale"]      = m_timeWriteLog;
	doc["screenUpdateIntervale"]  = m_timeUpdateScreen;
	doc["mode"]                   = m_mode;
	doc["namePhaseA"]             = m_namePhaseA;
	doc["namePhaseB"]             = m_namePhaseB;
	doc["namePhaseC"]             = m_namePhaseC;
  
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
	m_hostname = DEFAULT_HOSTNAME;
	m_ftpLogin = DEFAULT_FTP_USER;
	m_ftpPasswd = DEFAULT_FTP_PWD;
	m_timeUpdateNtp = DEFAULT_NTP_UPDATE_INTERVAL_SEC;
	m_timeWriteLog = DEFAULT_LOG_WRITE_INTERVAL_SEC;
	m_timeUpdateScreen = DEFAULT_SCREEN_UPDATE_INTERVAL_SEC;
  m_mode = MODE_MONO;
  m_namePhaseA = "PHASE A";
  m_namePhaseB = "PHASE B";
  m_namePhaseC = "PHASE C";

	saveConfig();
	Serial.println("configuration restored.");
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES) 
JsonConfiguration Configuration;
#endif
