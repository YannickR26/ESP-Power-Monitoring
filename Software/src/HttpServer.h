#pragma once

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266FtpServer.h>
#include <ArduinoJson.h>

#include "JsonConfiguration.h"

class HttpServer
{
public:
	HttpServer() ;
	virtual ~HttpServer();

	void setup(void);
	void handle(void);

  ESP8266WebServer& webServer();

	static String getContentType(String filename);
  static bool handleFileRead(String path);
  static void handleNotFound();
  static void handleSet();
  static void getStatus();
  static void getConfig();
  static void setConfig();
  static void sendJson(const uint16 code, JsonDocument &doc);

private:
  ESP8266WebServer          _webServer;
  ESP8266HTTPUpdateServer   _httpUpdater;
  FtpServer                 _ftpServer;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern HttpServer HTTPServer;
#endif

