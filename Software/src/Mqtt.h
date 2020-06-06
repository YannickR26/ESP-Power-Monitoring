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
	void publish(String topic, String body);
	void publishMonitoringData();
	bool isConnected() { return clientMqtt.connected(); }

private:
	void reconnect();
	void callback(char *topic, uint8_t *payload, unsigned int length);

	PubSubClient clientMqtt;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Mqtt MqttClient;
#endif
