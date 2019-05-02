#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>
#include <simpleDSTadjust.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "ATM90E32.h"
// #include "screen.h"
#include "Mqtt.h"
#include "settings.h"
// #include "logWriter.h"

#define ENABLE_OTA    // If defined, enable Arduino OTA code.
// #define WIFI_AP       // If defined, Wifi in AP Mode else STA Mode

// OTA
#ifdef ENABLE_OTA
  #include <ArduinoOTA.h>
#endif

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
Ticker tickerEvery1sec;

struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);

// Global variable for Tick
bool readyForNtpUpdate;
bool readyForSendData;

// Ticker every 1 seconds
void secTicker()
{
  static int tickNTPUpdate = Configuration._timeUpdateNtp;
  static int tickSendData = Configuration._timeSendData;

  tickNTPUpdate--;
  if (tickNTPUpdate <= 0) {
    readyForNtpUpdate = true;
    tickNTPUpdate = Configuration._timeUpdateNtp;
  }

  tickSendData--;
  if (tickSendData <= 0) {
   readyForSendData = true;
    tickSendData = Configuration._timeSendData;
  }
}

void printTime()
{
  char buf[40];
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&t);

  sprintf(buf, "DateTime: %02d/%02d/%d, %02d:%02d:%02d %s", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, dstAbbrev);
  Serial.println(buf);
}

void updateNTP() {
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
  printTime();
}

/*************/
/*** SETUP ***/
/*************/
void setup() {
  /* Initialize the serial port to host */
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect. Needed for native USB
  Serial.println("Starting...");

  /* Initialize the ATM90E32 + SPI port */
  Monitoring.setup(ATM90E32_CS);

  /* Initialize the screen */
  // Screen.setup();

  /* Connect to Wifi */
#ifndef WIFI_AP
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);
  Serial.println("\nConnecting to WiFi");
  // Screen.connecting_to_wifi();
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  // Screen.wifi_done();
  Serial.println("\nDone");
  Serial.println(String("Connected to ") + WiFi.SSID());
  Serial.println(String("IP address: ") + WiFi.localIP().toString());
#else
  WiFi.softAP(WIFI_AP_HOTSTNAME, WIFI_AP_PASSWORD);
  Serial.print("Wifi AP Mode, IP address: ");
  Serial.println(WiFi.softAPIP());
#endif

  /* Initialize HTTP Server */
  HTTPServer.setup();
  
  delay(1000);

  /* Initialize MQTT Client */
  MqttClient.setup();

  // Configuration.setup();
  Configuration.restoreDefault();

  /* Init the NTP time */
  updateNTP();

  // Init OTA
#ifdef ENABLE_OTA
  Serial.println("Arduino OTA activated");
  
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([&]() {
    Serial.println("Arduino OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("Arduino OTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Arduino OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Arduino OTA: Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Arduino OTA: Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Arduino OTA: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Arduino OTA: Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("Arduino OTA: End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("");
#endif

  // Ticker every 1 seconds
  tickerEvery1sec.attach(1, secTicker);

  // Screen.clear();
}

/************/
/*** LOOP ***/
/************/
void loop() {

  // Screen.handle();
  MqttClient.handle();

  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if (readyForNtpUpdate) {
    updateNTP();
    readyForNtpUpdate = false;
  }

  if (readyForSendData) {
    // TODO send data to mqtt broker
    Serial.println("Send data to MQTT");
    MqttClient.publish("outTopic", "msgTest");
    readyForSendData = false;
  }

  // char *dstAbbrev;
  // time_t now = dstAdjusted.time(&dstAbbrev);
  // Screen.display_menu(&now);

  // LogWriter.handle(timeinfo);

  // Screen.display_relay_status(false);
 
  delay(50);
}
