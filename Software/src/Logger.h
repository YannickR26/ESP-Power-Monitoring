#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <Arduino.h>

class Logger
{
public:
	Logger()
#ifdef DEBUG_BY_TELNET
		: telnetServer(23)
#endif
	{};
	virtual ~Logger(){};

	void setup();
	void setupTelnet();
	void handle();

	void println(const String &s = String());
	void println(const char str[]) { println(String(str)); }

	void print(const String &s);
	void print(const char str[]) { print(String(str)); }

	char *getDateTimeString();

private:
	void send(String &s);
	void addTime(String &s);

	bool addTimeToString;

#ifdef DEBUG_BY_TELNET
	WiFiServer telnetServer;
	WiFiClient telnetClient;
	void handleTelnetClient();
#endif
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Logger Log;
#endif
