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
	bool publish(String topic, String body);
	bool subscribe(String topic);
	void log(String level, String str);
	bool publishMonitoringData();
	bool isConnected() { return _clientMqtt.connected(); }

private:
	void reconnect();
	void callback(char *topic, uint8_t *payload, unsigned int length);

	PubSubClient _clientMqtt;
	String startedAt;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Mqtt MqttClient;
#endif
