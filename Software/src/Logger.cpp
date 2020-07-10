#include "Logger.h"
#include "settings.h"
#include "Mqtt.h"

#ifdef USE_DST_ADJUST
#include <simpleDSTadjust.h>

struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);
#endif

#ifdef DEBUG_BY_TELNET
WiFiServer telnetServer(23);
WiFiClient telnetClient;
#endif

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
void Logger::setup()
{
  addTimeToString = true;

#ifdef DEBUG_BY_SERIAL
  Serial.begin(115200);
  while (!Serial)
  {
  } // wait for serial port to connect. Needed for native USB
  println();
  println("Starting...");
#endif
}

void Logger::setupTelnet()
{
#ifdef DEBUG_BY_TELNET
  // Setup telnet server for remote debug output
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  println("Telnet: Started on port 23 - IP: " + WiFi.localIP().toString());
#endif
}

void Logger::handle()
{
#ifdef DEBUG_BY_TELNET
  handleTelnetClient();
#endif
}

void Logger::println(const String &s)
{
  String debugText = s;
  debugText += "\r\n";

  if (addTimeToString)
  {
    addTime(debugText);
  }
  addTimeToString = true;

  send(debugText);
}

void Logger::print(const String &s)
{
  String debugText = s;

  if (addTimeToString)
  {
    addTime(debugText);
    addTimeToString = false;
  }

  send(debugText);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Logger::send(String &s)
{
#ifdef DEBUG_BY_SERIAL
  Serial.print(s);
  Serial.flush();
#endif

#ifdef DEBUG_BY_TELNET
  if (telnetClient.connected())
  {
    const size_t len = s.length();
    const char *buffer = s.c_str();
    telnetClient.write(buffer, len);
    handleTelnetClient();
  }
#endif

#ifdef DEBUG_BY_MQTT
  if (MqttClient.isConnected())
  {
    MqttClient.log("debug", s);
  }
#endif
}

void Logger::addTime(String &s)
{
#ifdef USE_DST_ADJUST
  char time[30];
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&t);

  sprintf(time, "%02d/%02d/%d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  s = "[" + String(time) + "] " + s;
#else
  s = "[" + String(millis()) + "] " + s;
#endif
}

#ifdef DEBUG_BY_TELNET
void Logger::handleTelnetClient()
{
  if (telnetServer.hasClient())
  {
    // client is connected
    if (!telnetClient || !telnetClient.connected())
    {
      if (telnetClient)
        telnetClient.stop();                   // client disconnected
      telnetClient = telnetServer.available(); // ready for new client
    }
    else
    {
      telnetServer.available().stop(); // have client, block new connections
    }
  }
  // Handle client input from telnet connection.
  if (telnetClient && telnetClient.connected() && telnetClient.available())
  {
    // client input processing
    while (telnetClient.available())
    {
      // Read data from telnet just to clear out the buffer
      telnetClient.read();
    }
    println("=====================================================");
    println(String("ESP_Power_Monitoring - Build: ") + F(__DATE__) + " " + F(__TIME__));
    println(String("ESP_Power_Monitoring - Version: ") + String(VERSION));
    println("=====================================================");
    println();
  }
}
#endif

#if !defined(NO_GLOBAL_INSTANCES)
Logger Log;
#endif
