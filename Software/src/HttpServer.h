#pragma once

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#endif
#include <ArduinoJson.h>

#include "JsonConfiguration.h"

class HttpServer
{
public:
    HttpServer();
    virtual ~HttpServer();

    void setup(void);
    void handle(void);

#if defined(ESP8266)
    ESP8266WebServer &webServer() { return _webServer; }
#elif defined(ESP32)
    WebServer &webServer() { return _webServer; }
#endif


    String getContentType(String filename);
    bool handleFileRead(String path);
    void handleNotFound();
    void handleSet();
    void getStatus();
    void getConfig();
    void setConfig();
    void sendJson(const uint16_t code, JsonDocument &doc);

private:
#if defined(ESP8266)
    ESP8266WebServer _webServer;
    ESP8266HTTPUpdateServer _httpUpdater;
#elif defined(ESP32)
    WebServer _webServer;
    HTTPUpdateServer _httpUpdater;
#endif
};

#if !defined(NO_GLOBAL_INSTANCES)
extern HttpServer HTTPServer;
#endif
