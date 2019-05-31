#include <Arduino.h>
#include <WiFiManager.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "ATM90E32.h"
#include "Mqtt.h"
#include "settings.h"
#include "Logger.h"

// #define ENABLE_OTA    // If defined, enable Arduino OTA code.

// OTA
#ifdef ENABLE_OTA
  #include <ArduinoOTA.h>
#endif

void updateNTP() {
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Log.print("#");
    delay(1000);
  }
  Log.println("Update NTP");
}

/*************/
/*** SETUP ***/
/*************/
void setup() {
  /* Initialize Logger */
  Log.setup();
  Log.println(String(F("ESP_Power_Monitoring - Build: ")) + F(__DATE__) + " " +  F(__TIME__));

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  // wifiManager.resetSettings();

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

  if (!wifiManager.autoConnect(Configuration._hostname.c_str())) {
    Log.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  
  Log.println(String("Connected to ") + WiFi.SSID());
  Log.println(String("IP address: ") + WiFi.localIP().toString());

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._timeSendData = atoi(custom_time_update.getValue());
  Configuration._mode = atoi(custom_mode.getValue());
  Configuration.saveConfig();

  /* Initialize the ATM90E32 + SPI port */
  Monitoring.begin(ATM90E32_CS, ATM90E32_PM0, ATM90E32_PM1, 0x87, 0, 30250, 9200, 9200, 9200);
  
  /* Initialize HTTP Server */
  HTTPServer.setup();
  
  delay(100);

  /* Initialize MQTT Client */
  MqttClient.setup();

  // Init OTA
#ifdef ENABLE_OTA
  Log.println("Arduino OTA activated");
  
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([&]() {
    Log.println("Arduino OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Log.println("Arduino OTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Log.printf("Arduino OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Log.println("Arduino OTA: Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Log.println("Arduino OTA: Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Log.println("Arduino OTA: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Log.println("Arduino OTA: Receive Failed");
    else if (error == OTA_END_ERROR) Log.println("Arduino OTA: End Failed");
  });

  ArduinoOTA.begin();
  Log.println("");
#endif

  updateNTP();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// get Energy Vals                                               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void getATM90()
{
  float voltageA, voltageB, voltageC, totalVoltage, currentA, currentB, currentC, totalCurrent, realPower, powerFactor, chipTemp, powerFreq, totalWatts;
  unsigned short sys0, sys1, en0, en1;

  delay(10);
  sys0 = Monitoring.GetSysStatus0();
  sys1 = Monitoring.GetSysStatus1();
  en0 = Monitoring.GetMeterStatus0();
  en1 = Monitoring.GetMeterStatus1();

  Log.println(String(F("ATM90: Sys Status: S0:0x")) + String(sys0, HEX) + String(F(" S1:0x")) + String(sys1, HEX));
  Log.println(String(F("ATM90: Meter Status: E0:0x")) + String(en0, HEX) + String(F(" E1:0x")) + String(en1, HEX));

  voltageA    = Monitoring.GetLineVoltageA();
  voltageB    = Monitoring.GetLineVoltageB();
  voltageC    = Monitoring.GetLineVoltageC();
  currentA    = Monitoring.GetLineCurrentA();
  currentB    = Monitoring.GetLineCurrentB();
  currentC    = Monitoring.GetLineCurrentC();
  realPower   = Monitoring.GetTotalActivePower();
  powerFactor = Monitoring.GetTotalPowerFactor();
  chipTemp    = Monitoring.GetTemperature(); 
  powerFreq   = Monitoring.GetFrequency();

  totalVoltage = voltageA + voltageB + voltageC ;
  totalCurrent = currentA + currentB + currentC;
  totalWatts  = (voltageA * currentA) + (voltageB * currentB) + (voltageC * currentC);

  Log.println(String(F("ATM90: VA: ")) + String(voltageA) + String(F("V - VB: ")) + String(voltageB) + String(F("V - VC: ")) + String(voltageC) + String(F("V")));
  Log.println(String(F("ATM90: IA: ")) + String(currentA) + String(F("A - IB: ")) + String(currentB) + String(F("A - IC: ")) + String(currentC, 4) + String(F("A")));
  Log.println(String(F("ATM90: RP: ")) + String(realPower) + String(F(" - PF: ")) + String(powerFactor));
  Log.println(String(F("ATM90: ATM90Temp: ")) + String(chipTemp) + String(F("C - Freq: ")) + String(powerFreq)+ String(F("hz")));
  Log.println(String(F("ATM90: TotalV: ")) + String(totalVoltage) + String(F("V - TotalA: ")) + String(totalCurrent) + String(F("A - TotalW: ")) + String(totalWatts) + String(F("W")));
  Log.println();
}

/************/
/*** LOOP ***/
/************/
void loop() {
  static unsigned long tickNTPUpdate, tickSendData, tickLed;

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if ((millis() - tickNTPUpdate) >= (unsigned long)(Configuration._timeUpdateNtp*1000)) {
    updateNTP();
    tickNTPUpdate = millis();
  }
  
  if ((millis() - tickSendData) >= (unsigned long)(Configuration._timeSendData*1000)) {
    Log.println("Send data to MQTT");
    // MqttClient.publishMonitoringData();
    getATM90();
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
 
  delay(50);
}
