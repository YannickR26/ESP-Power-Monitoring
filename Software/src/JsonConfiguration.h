#pragma once

#include <ArduinoJson.h>
#include "settings.h"

class JsonConfiguration 
{
  public:
  	JsonConfiguration();
  	virtual ~JsonConfiguration();
  
  	void setup();
	  
	void print(void);

    bool readConfig();
  	bool saveConfig();
  
  	void restoreDefault();
    
	uint8_t encodeToJson(JsonDocument &_json);
	uint8_t decodeJsonFromFile(const char* input);
  
	/* Members */
  	String _hostname;
	String _mqttIpServer;
	uint16_t _mqttPortServer;
	uint16_t _timeSaveData;
	uint16_t _timeSendData;
	uint8_t _mode;
	String _nameA, _nameB, _nameC;
	uint8_t _currentClampA, _currentClampB, _currentClampC;
	uint32_t _consoA, _consoB, _consoC;
     	
  private:
  
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
