#pragma once

#include "settings.h"

class JsonConfiguration 
{
  public:
  	JsonConfiguration();
  	virtual ~JsonConfiguration();
  
  	void setup();

    bool readConfig();
  	bool saveConfig();
  
  	void restoreDefault();
    
    String toJson();
  
	/* Members */
  	String _hostname;
	String _mqttIpServer;
	uint16_t _mqttPortServer;
	uint16_t _timeUpdateNtp;
	uint16_t _timeSendData;
	uint8_t _mode;
	uint8_t _iGain;
	uint32_t _consoA, _consoB, _consoC;
     	
  private:
  
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
