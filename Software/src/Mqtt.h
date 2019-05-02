#pragma once

#include <PubSubClient.h>

#include "settings.h"

class Mqtt 
{
  public:
		Mqtt();
  	virtual ~Mqtt();
  
  	void setup();
		void handle();
		bool publish(const char* topic, const char* payload);

		void reconnect();
    void callback(char* topic, uint8_t* payload, unsigned int length);

  private:
		PubSubClient clientMqtt;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Mqtt MqttClient;
#endif
