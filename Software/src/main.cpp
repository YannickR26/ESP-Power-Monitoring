#include <Arduino.h>
#include <simpleDSTadjust.h>
#include <WiFiManager.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "ATM90E32.h"
#include "Mqtt.h"
#include "settings.h"

// #define ENABLE_OTA    // If defined, enable Arduino OTA code.

// OTA
#ifdef ENABLE_OTA
  #include <ArduinoOTA.h>
#endif

struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);

void printTime()
{
  char buf[40];
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&t);

  sprintf(buf, "%02d/%02d/%d, %02d:%02d:%02d %s => ", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, dstAbbrev);
  Serial.print(buf);
}

void updateNTP() {
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
  printTime();
  Serial.println("Update NTP");
}

/*************/
/*** SETUP ***/
/*************/
void setup() {
  /* Initialize the serial port to host */
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect. Needed for native USB
  Serial.println();
  Serial.println();
  Serial.println("Starting...");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

  /* Initialize the ATM90E32 + SPI port */
  Monitoring.setup(ATM90E32_CS);

  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // WiFiManagerParameter
  WiFiManagerParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  WiFiManagerParameter custom_time_update("timeUpdate", "time update data (s)", String(Configuration._timeSendData).c_str(), 6);
  WiFiManagerParameter custom_mode("mode", "mode", String(Configuration._mode).c_str(), 1);

  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_hostname);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_time_update);
  wifiManager.addParameter(&custom_mode);

  // wifiManager.resetSettings();

  if (!wifiManager.autoConnect(WIFI_AP_HOTSTNAME)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  
  Serial.println(String("Connected to ") + WiFi.SSID());
  Serial.println(String("IP address: ") + WiFi.localIP().toString());

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._timeSendData = atoi(custom_time_update.getValue());
  Configuration._mode = atoi(custom_mode.getValue());
  Configuration.saveConfig();

  /* Initialize HTTP Server */
  HTTPServer.setup();
  
  delay(100);

  /* Initialize MQTT Client */
  MqttClient.setup();

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

  updateNTP();
}

/************/
/*** LOOP ***/
/************/
void loop() {
  static unsigned long tickNTPUpdate, tickSendData, tickLed;

  MqttClient.handle();

  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if ((millis() - tickNTPUpdate) >= (unsigned long)(Configuration._timeUpdateNtp*1000)) {
    updateNTP();
    tickNTPUpdate = millis();
  }
  
  if ((millis() - tickSendData) >= (unsigned long)(Configuration._timeSendData*1000)) {
    printTime();
    Serial.println("Send data to MQTT");
    MqttClient.publishMonitoringData();
    tickSendData = millis();
  }

  if (MqttClient.isConnected()) {
    if ((millis() - tickLed) >= (unsigned long)(LEDTIME_WORK)) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      tickLed = millis();
    }
  }
  else {
     if ((millis() - tickLed) >= (unsigned long)(LEDTIME_NOMQTT)) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      tickLed = millis();
    }
  }
 
  delay(10);
}
