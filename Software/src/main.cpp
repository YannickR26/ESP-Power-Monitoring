#include <Arduino.h>
#include <WiFiManager.h>
#include <Ticker.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "ATM90E32.h"
#include "Mqtt.h"
#include "settings.h"
#include "Logger.h"
#include "SimpleRelay.h"

static Ticker tick_blinker;
SimpleRelay relay(RELAY_PIN, "relay");

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
  struct tm timeinfo;

  Log.print("Update NTP...");

  configTzTime(TIMEZONE, NTP_SERVERS);
  getLocalTime(&timeinfo);

  Log.println(" Done !");
  Log.print("Date Time: ");
  Log.println(asctime(&timeinfo));

  Log.println("Save data");
  Configuration._consoA = (uint32_t)Monitoring.getLineA()->conso;
  Configuration._consoB = (uint32_t)Monitoring.getLineB()->conso;
  Configuration._consoC = (uint32_t)Monitoring.getLineC()->conso;
  Configuration.saveConfig();
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

  Log.print("Send data to MQTT... ");
  bool ret = MqttClient.publishMonitoringData();
  if (ret == true)
    Log.println("Done !");
  else
    Log.println("Error !");
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
  WiFiManagerParameter custom_mqtt_username("mqttUsername", "mqtt username", String(Configuration._mqttUsername).c_str(), 40);
  WiFiManagerParameter custom_mqtt_userpassword("mqttPassword", "mqtt password", String(Configuration._mqttPassword).c_str(), 40);
  WiFiManagerParameter custom_time_send_data("timeSendData", "intervale d'envoie des données (s)", String(Configuration._timeSendData).c_str(), 6);
  WiFiManagerParameter custom_time_save_data("timeSaveData", "intervale de sauvegarde des données (s)", String(Configuration._timeSaveData).c_str(), 6);
  WiFiManagerParameter custom_mode("mode", "mode (0: Mono, 1: 1x Tri, 2: 2x Tri)", String(Configuration._mode).c_str(), 1);
  WiFiManagerParameter custom_currentA("currentClampA", "capacité de la pince amperemetrique A (A)", String(Configuration._currentClampA).c_str(), 3);
  WiFiManagerParameter custom_currentB("currentClampB", "capacité de la pince amperemetrique B (A)", String(Configuration._currentClampB).c_str(), 3);
  WiFiManagerParameter custom_currentC("currentClampC", "capacité de la pince amperemetrique C (A)", String(Configuration._currentClampC).c_str(), 3);

  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_hostname);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_userpassword);
  wifiManager.addParameter(&custom_time_send_data);
  wifiManager.addParameter(&custom_time_save_data);
  wifiManager.addParameter(&custom_mode);
  wifiManager.addParameter(&custom_currentA);
  wifiManager.addParameter(&custom_currentB);
  wifiManager.addParameter(&custom_currentC);

  Log.println("Try to connect to WiFi...");
  wifiManager.setWiFiAPChannel(6);
  wifiManager.setConfigPortalTimeout(300); // Set Timeout for portal configuration to 300 seconds
  if (!wifiManager.autoConnect(Configuration._hostname.c_str()))
  {
    Log.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    Log.println("Restart ESP now !");
    ESP.restart();
    delay(5000);
  }

  Log.println(String("Connected to ") + WiFi.SSID());
  Log.println(String("IP address: ") + WiFi.localIP().toString());

  WiFi.enableAP(false);
  WiFi.softAPdisconnect();
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._mqttUsername = custom_mqtt_username.getValue();
  Configuration._mqttPassword = custom_mqtt_userpassword.getValue();
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
  Log.println(String(F("---------- ESP Power Monitoring ----------")));
  Log.println(String(F("  Version: ")) + VERSION);
  Log.println(String(F("  Build: ")) + BUILD_DATE);
  Log.println("==========================================");
  Log.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();
  relay.setTimeout(Configuration._timeoutRelay);
  relay.setState(Configuration._stateRelay);

  wifiSetup();

  // Create ticker for blink LED
  tick_blinker.once_ms(LED_TIME_NOMQTT, blinkLED);

  /* Initialize HTTP Server */
  HTTPServer.setup();

  /* Initialize MQTT Client */
  MqttClient.setup();

  Log.setupTelnet();

  // Init OTA
#ifdef ENABLE_OTA
  Log.println("Arduino OTA activated");

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([]() {
    Log.println("Arduino OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Log.println("Arduino OTA: End");
  });
  // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //   Log.println("Arduino OTA Progress: " + String(progress / (total / 100) + "%"));
  // });
  // ArduinoOTA.onError([](ota_error_t error) {
  //   Log.print("Arduino OTA Error [" + String(error) + "]: ");
  //   if (error == OTA_AUTH_ERROR)
  //     Log.println("Arduino OTA: Auth Failed");
  //   else if (error == OTA_BEGIN_ERROR)
  //     Log.println("Arduino OTA: Begin Failed");
  //   else if (error == OTA_CONNECT_ERROR)
  //     Log.println("Arduino OTA: Connect Failed");
  //   else if (error == OTA_RECEIVE_ERROR)
  //     Log.println("Arduino OTA: Receive Failed");
  //   else if (error == OTA_END_ERROR)
  //     Log.println("Arduino OTA: End Failed");
  // });

  ArduinoOTA.begin();
#endif

  /* Initialize the ATM90E32 + SPI port */
  Monitoring.begin(ATM90E32_CS, Configuration._mode, 0, ATM90E32_UGAIN, Configuration._currentClampA, Configuration._currentClampB, Configuration._currentClampC);
  Monitoring.setConsoLineA(Configuration._consoA);
  Monitoring.setConsoLineB(Configuration._consoB);
  Monitoring.setConsoLineC(Configuration._consoC);

  updateTimeAndSaveData();
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  static uint8_t noWifiConnection = 0;
  static unsigned long tickSendData = 0, tickSaveData = 0, tickPrintData = 0, tickCheckWifi = 0;
  const unsigned long tick = millis();

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

  // Send data
  if ((tick - tickSendData) >= (Configuration._timeSendData * 1000))
  {
    sendData();
    tickSendData = tick;
  }

  // Save data
  if ((tick - tickSaveData) >= (Configuration._timeSaveData * 1000))
  {
    updateTimeAndSaveData();
    tickSaveData = tick;
  }

  // Check wifi connection every 10 seconds 
  if ((tick - tickCheckWifi) >= 10000)
  {
    if (!WiFi.isConnected())
    {
      // If at 60 seconds we have no wifi, we force to reconnect
      if (noWifiConnection >= 6)
      {
        WiFi.reconnect();
        // ESP.restart();
      }
      else
      {
        noWifiConnection++;
      }
    }
    else
    {
      noWifiConnection = 0;
    }
    tickCheckWifi = tick;
  }

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  if ((tick - tickPrintData) >= 2000)
  {
    if (Configuration._mode == MODE_CALIB)
    {
      /* Uncomment if you want calculate the offset of I and U
      *  ! Warning ! the voltage and the current must be at 0
      */
      unsigned short val;

      val = Monitoring.CalculateVIOffset(UrmsA, UrmsALSB);
      Log.println("Offset UA: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(UrmsB, UrmsBLSB);
      Log.println("Offset UB: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(UrmsC, UrmsCLSB);
      Log.println("Offset UC: " + String(val) + " (0x" + String(val, HEX) + ")");

      val = Monitoring.CalculateVIOffset(IrmsA, IrmsALSB);
      Log.println("Offset IA: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(IrmsB, IrmsBLSB);
      Log.println("Offset IB: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(IrmsC, IrmsCLSB);
      Log.println("Offset IC: " + String(val) + " (0x" + String(val, HEX) + ")");

      val = Monitoring.CalculateVIOffset(PmeanA, PmeanALSB);
      Log.println("Offset PA: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(PmeanB, PmeanBLSB);
      Log.println("Offset PB: " + String(val) + " (0x" + String(val, HEX) + ")");
      val = Monitoring.CalculateVIOffset(PmeanC, PmeanCLSB);
      Log.println("Offset PC: " + String(val) + " (0x" + String(val, HEX) + ")");

      Log.println();
    }
    else if (Configuration._mode == MODE_DEBUG)
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
    tickPrintData = tick;
  }

  delay(50);
}
