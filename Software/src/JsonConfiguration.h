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
		int _mqttPortServer;
		int _timeUpdateNtp;
		int _timeSendData;
		int _timeUpdateScreen;
		int _mode;
		String _namePhaseA, _namePhaseB, _namePhaseC;
     	
  private:
  
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
