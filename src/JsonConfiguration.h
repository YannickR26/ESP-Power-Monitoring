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
  	String m_hostname;
  	String m_ftpLogin;
  	String m_ftpPasswd;
		int m_timeUpdateNtp;
		int m_timeWriteLog;
		int m_timeUpdateScreen;
		int m_mode;
		String m_namePhaseA, m_namePhaseB, m_namePhaseC;
     	
  private:
  
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
