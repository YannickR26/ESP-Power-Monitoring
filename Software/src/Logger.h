#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

/******* Debug **************/
#define DEBUG_SERIAL
#define DEBUG_TELNET  // Open a read-only telnet debug port

class Logger 
{
  public:
	Logger() {} ;
  	virtual ~Logger() {} ;

	void setup();
	void handle();

	size_t println(const String &s = String());
  size_t println(const char str[]) { return println(String(str)); }

	size_t print(const String &s);
  size_t print(const char str[]) { return print(String(str)); }

  private:
#ifdef DEBUG_TELNET
	void handleTelnetClient();
#endif
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Logger Log;
#endif
