#include <Arduino.h>
#include <WiFiManager.h>
#include <Ticker.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "ATM90E32.h"
#include "Mqtt.h"
#include "settings.h"
#include "Logger.h"

static Ticker tick_blinker, tick_sendData, tick_saveData;

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

/**************/
/*** TICKER ***/
/**************/

// Update time from NTP and save data
void updateTimeAndSaveData()
{
  Log.println("Update NTP");
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr))
  {
    Log.print("#");
    delay(1000);
  }

  Log.println("Save data");
  Configuration._consoA = Monitoring.getLineA().conso;
  Configuration._consoB = Monitoring.getLineB().conso;
  Configuration._consoC = Monitoring.getLineC().conso;
  Configuration.saveConfig();

  tick_saveData.once(Configuration._timeSaveData, updateTimeAndSaveData);
}

// LED blink
void blinkLED()
{
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));

  // Check state of MQTT
  if (MqttClient.isConnected())
  {
    tick_blinker.once_ms(LED_TIME_WORK, blinkLED);
  }
  else
  {
    tick_blinker.once_ms(LED_TIME_NOMQTT, blinkLED);
  }
}

// Send Data to MQTT
void sendData()
{
  Log.println("Read data...");
  Monitoring.handle();

  Log.println("Send data to MQTT");
  MqttClient.publishMonitoringData();

  tick_sendData.once(Configuration._timeSendData, sendData);
}

/*************/
/*** SETUP ***/
/*************/

// Wifi setup
void wifiSetup()
{
  WiFiManager wifiManager;
  // wifiManager.setDebugOutput(false);
  // wifiManager.resetSettings();

  // WiFiManagerParameter
  WiFiManagerParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  WiFiManagerParameter custom_mqtt_server("mqttIpServer", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("mqttPortServer", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  WiFiManagerParameter custom_time_send_data("timeSendData", "intervale d'envoie des données (s)", String(Configuration._timeSendData).c_str(), 6);
  WiFiManagerParameter custom_time_save_data("timeSaveData", "intervale de sauvegarde des données (s)", String(Configuration._timeSaveData).c_str(), 6);
  WiFiManagerParameter custom_mode("mode", "mode", String(Configuration._mode).c_str(), 1);
  WiFiManagerParameter custom_currentA("currentClampA", "capacité de la pince amperemetrique A (A)", String(Configuration._currentClampA).c_str(), 3);
  WiFiManagerParameter custom_currentB("currentClampB", "capacité de la pince amperemetrique B (A)", String(Configuration._currentClampB).c_str(), 3);
  WiFiManagerParameter custom_currentC("currentClampC", "capacité de la pince amperemetrique C (A)", String(Configuration._currentClampC).c_str(), 3);

  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_hostname);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_time_send_data);
  wifiManager.addParameter(&custom_time_save_data);
  wifiManager.addParameter(&custom_mode);
  wifiManager.addParameter(&custom_currentA);
  wifiManager.addParameter(&custom_currentB);
  wifiManager.addParameter(&custom_currentC);

  Log.println("Try to connect to WiFi...");
  // wifiManager.setWiFiChannel(6);
  wifiManager.setConfigPortalTimeout(300); // Set Timeout for portal configuration to 120 seconds
  if (!wifiManager.autoConnect(Configuration._hostname.c_str()))
  {
    Log.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Log.println(String("Connected to ") + WiFi.SSID());
  Log.println(String("IP address: ") + WiFi.localIP().toString());

  WiFi.enableAP(false);
  WiFi.softAPdisconnect();

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._timeSendData = atoi(custom_time_send_data.getValue());
  Configuration._timeSaveData = atoi(custom_time_save_data.getValue());
  Configuration._mode = atoi(custom_mode.getValue());
  Configuration._currentClampA = atoi(custom_currentA.getValue());
  Configuration._currentClampB = atoi(custom_currentB.getValue());
  Configuration._currentClampC = atoi(custom_currentC.getValue());
}

// Setup
void setup()
{
  delay(100);

  /* Initialize Logger */
  Log.setup();
  Log.println();
  Log.println("==========================================");
  Log.println(String(F("  === ESP_Power_Monitoring ===")));
  Log.println(String(F("  Version: ")) + F(VERSION));
  Log.println(String(F("  Build: ")) + F(__DATE__) + " " + F(__TIME__));
  Log.println("==========================================");
  Log.println();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

  wifiSetup();

  /* Initialize the ATM90E32 + SPI port */
  Monitoring.begin(ATM90E32_CS, ATM90E32_PM0, ATM90E32_PM1, Configuration._mode, 0, ATM90E32_UGAIN, Configuration._currentClampA, Configuration._currentClampB, Configuration._currentClampC);
  Monitoring.setConsoLineA(Configuration._consoA);
  Monitoring.setConsoLineB(Configuration._consoB);
  Monitoring.setConsoLineC(Configuration._consoC);

  /* Initialize HTTP Server */
  HTTPServer.setup();

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
    if (error == OTA_AUTH_ERROR)
      Log.println("Arduino OTA: Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Log.println("Arduino OTA: Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Log.println("Arduino OTA: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Log.println("Arduino OTA: Receive Failed");
    else if (error == OTA_END_ERROR)
      Log.println("Arduino OTA: End Failed");
  });

  ArduinoOTA.begin();
  Log.println("");
#endif

  Log.setupTelnet();

  updateTimeAndSaveData();

  // Create ticker for blink LED
  tick_blinker.once_ms(LED_TIME_NOMQTT, blinkLED);

  // Create ticker for send Data to MQTT
  if (Configuration._timeSendData == 0)
    Configuration._timeSendData = 1;
  tick_sendData.once(Configuration._timeSendData, sendData);

  // Create ticker for update NTP and save data
  if (Configuration._timeSaveData == 0)
    Configuration._timeSaveData = 1;
  tick_saveData.once(Configuration._timeSaveData, updateTimeAndSaveData);
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  static unsigned long tickPrintData;
  unsigned long currentMillis = millis();

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if ((currentMillis - tickPrintData) >= 2000)
  {
    /* Uncomment if you want calculate the offset of I and U
      ! Warning ! the voltage and the current must be at 0
    */
    // Log.println("Offset IA: " + String(Monitoring.CalculateVIOffset(IrmsA, IrmsALSB, IoffsetA)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(IrmsA, IrmsALSB, IoffsetA), HEX) + ")");
    // Log.println("Offset IB: " + String(Monitoring.CalculateVIOffset(IrmsB, IrmsBLSB, IoffsetB)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(IrmsB, IrmsBLSB, IoffsetB), HEX) + ")");
    // Log.println("Offset IC: " + String(Monitoring.CalculateVIOffset(IrmsC, IrmsCLSB, IoffsetC)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(IrmsC, IrmsCLSB, IoffsetC), HEX) + ")");

    // Log.println("Offset UA: " + String(Monitoring.CalculateVIOffset(UrmsA, UrmsALSB, UoffsetA)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(UrmsA, UrmsALSB, UoffsetA), HEX) + ")");
    // Log.println("Offset UB: " + String(Monitoring.CalculateVIOffset(UrmsB, UrmsBLSB, UoffsetB)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(UrmsB, UrmsBLSB, UoffsetB), HEX) + ")");
    // Log.println("Offset UC: " + String(Monitoring.CalculateVIOffset(UrmsC, UrmsCLSB, UoffsetC)) + " (0x" + String((unsigned short)Monitoring.CalculateVIOffset(UrmsC, UrmsCLSB, UoffsetC), HEX) + ")");
    if (Configuration._mode == MODE_DEBUG)
    {
      Log.println("Line A: " + String(Monitoring.GetLineVoltageA()) + "V, " + String(Monitoring.GetLineCurrentA()) + "A, " + String(Monitoring.GetActivePowerA()) + "W");
      Log.println("Line B: " + String(Monitoring.GetLineVoltageB()) + "V, " + String(Monitoring.GetLineCurrentB()) + "A, " + String(Monitoring.GetActivePowerB()) + "W");
      Log.println("Line C: " + String(Monitoring.GetLineVoltageC()) + "V, " + String(Monitoring.GetLineCurrentC()) + "A, " + String(Monitoring.GetActivePowerC()) + "W");
      Log.println("Frequency: " + String(Monitoring.GetFrequency()) + "Hz");
      Log.println("Total Active Fond Power: " + String(Monitoring.GetTotalActiveFundPower()) + "W");
      Log.println("Total Active Harm Power: " + String(Monitoring.GetTotalActiveHarPower()) + "W");
      Log.println("Reactive Power => A: " + String(Monitoring.GetReactivePowerA()) + "VARS, B: " + String(Monitoring.GetReactivePowerB()) + "VARS, C: " + String(Monitoring.GetReactivePowerC()) + "VARS");
      Log.println("Apparent Power => A: " + String(Monitoring.GetApparentPowerA()) + "VA, B: " + String(Monitoring.GetApparentPowerB()) + "VA, C: " + String(Monitoring.GetApparentPowerC()) + "VA");
      Log.println("Phase => A: " + String(Monitoring.GetPhaseA()) + "°, B: " + String(Monitoring.GetPhaseB()) + "°, C: " + String(Monitoring.GetPhaseC()) + "°");
      Log.println("PF => A: " + String(Monitoring.GetPowerFactorA()) + ", B: " + String(Monitoring.GetPowerFactorB()) + ", C: " + String(Monitoring.GetPowerFactorC()));

      Log.println();
    }
    tickPrintData = currentMillis;
  }

  // delay(50);
}
