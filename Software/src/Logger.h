#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

/******* Debug **************/
#define DEBUG_SERIAL
#define DEBUG_TELNET  // Open a read-only telnet debug port

#define USE_DST_ADJUST

class Logger 
{
  public:
	Logger() {} ;
  	virtual ~Logger() {} ;

	void setup();
	void handle();

	void println(const String &s = String());
  void println(const char str[]) { println(String(str)); }

	void print(const String &s);
  void print(const char str[]) { print(String(str)); }

  private:
	void send(String &s);
	void addTime(String &s);
#ifdef DEBUG_TELNET
	void handleTelnetClient();
#endif

	bool addTimeToString;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Logger Log;
#endif
