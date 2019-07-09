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

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // wifiManager.setDebugOutput(false);
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

  Log.println("Try to connect to WiFi...");
  wifiManager.setConfigPortalTimeout(300); // Set Timeout for portal configuration to 120 seconds
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
  uint16_t mmode0 = (Configuration._mode == MODE_MONO) ? 0x87 : 0x185;
  Monitoring.begin(ATM90E32_CS, ATM90E32_PM0, ATM90E32_PM1, mmode0, 0, ATM90E32_UGAIN, ATM90E32_IGAIN);
  
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

  /* Uncomment if you want calculate the offset of I and U
    ! Warning ! the voltage and the current must be at 0
  */
  // Log.println("Offset IA: " + String(Monitoring.CalculateVIOffset(IrmsA, IrmsALSB, IoffsetA)));
  // Log.println("Offset IB: " + String(Monitoring.CalculateVIOffset(IrmsB, IrmsBLSB, IoffsetB)));
  // Log.println("Offset IC: " + String(Monitoring.CalculateVIOffset(IrmsC, IrmsCLSB, IoffsetC)));

  // Log.println("Offset UA: " + String(Monitoring.CalculateVIOffset(UrmsA, UrmsALSB, UoffsetA)));
  // Log.println("Offset UB: " + String(Monitoring.CalculateVIOffset(UrmsB, UrmsBLSB, UoffsetB)));
  // Log.println("Offset UC: " + String(Monitoring.CalculateVIOffset(UrmsC, UrmsCLSB, UoffsetC)));
}

/************/
/*** LOOP ***/
/************/
void loop() {
  static unsigned long tickNTPUpdate, tickSendData, tickPrintData, tickLed;
  unsigned long currentMillis = millis();

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if ((currentMillis - tickNTPUpdate) >= (unsigned long)(Configuration._timeUpdateNtp*1000)) {
    updateNTP();
    tickNTPUpdate = currentMillis;
  }
  
  if ((currentMillis - tickSendData) >= (unsigned long)(Configuration._timeSendData*1000)) {
    Log.println("Send data to MQTT");
    MqttClient.publishMonitoringData();
    tickSendData = currentMillis;
  }

  if ((currentMillis - tickPrintData) >= 1000 ) {
    Log.println("Line A: " + String(Monitoring.GetLineVoltageA()) + "V, " + String(Monitoring.GetLineCurrentA()) + "A, " + String(Monitoring.GetActivePowerA()) + "W");
    Log.println("Line B: " + String(Monitoring.GetLineVoltageB()) + "V, " + String(Monitoring.GetLineCurrentB()) + "A, " + String(Monitoring.GetActivePowerB()) + "W");
    Log.println("Line C: " + String(Monitoring.GetLineVoltageC()) + "V, " + String(Monitoring.GetLineCurrentC()) + "A, " + String(Monitoring.GetActivePowerC()) + "W");
    if (Configuration._mode == MODE_DEBUG) {
      Log.println("Total Active Fond Power: " + String(Monitoring.GetTotalActiveFundPower()) + "W");
      Log.println("Total Active Harm Power: " + String(Monitoring.GetTotalActiveHarPower()) + "W");
      Log.println("Phase => A: " + String(Monitoring.GetPhaseA()) + "°, B: " + String(Monitoring.GetPhaseB()) +  "°, C: " + String(Monitoring.GetPhaseC()) + "°");
    }
    Log.println();
    tickPrintData = currentMillis;
  }

  // if (MqttClient.isConnected()) {
  //   if ((currentMillis - tickLed) >= (unsigned long)(LEDTIME_WORK)) {
  //     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //     tickLed = currentMillis;
  //   }
  // }
  // else {
  //    if ((currentMillis - tickLed) >= (unsigned long)(LEDTIME_NOMQTT)) {
  //     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //     tickLed = currentMillis;
  //   }
  // }

  delay(50);
}
