#include "Logger.h"

#include "settings.h"
#include <simpleDSTadjust.h>

struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);

#ifdef DEBUG_TELNET
WiFiServer telnetServer(23);
WiFiClient telnetClient;
#endif

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
void Logger::setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect. Needed for native USB
  println();
  println("Starting...");
#endif

#ifdef DEBUG_TELNET
  // Setup telnet server for remote debug output
  telnetServer.setNoDelay(true);
  telnetServer.begin();
  println(String(F("Telnet: Started on port 23 - IP:")) + WiFi.localIP().toString());
#endif
}

void Logger::handle()
{
#ifdef DEBUG_TELNET
  handleTelnetClient();
#endif
}

size_t Logger::println(const String &s)
{
  String debugText = s;
  debugText += "\r\n";
  return print(debugText);
}

size_t Logger::print(const String &s)
{ 
  size_t size = 0;
  char time[30];
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&t);

  sprintf(time, "%02d/%02d/%d, %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  String debugTimeText = "[" + String(time) + "] " + s;
  // String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + s;

#ifdef DEBUG_SERIAL
  size = Serial.print(debugTimeText);
  Serial.flush();
#endif

#ifdef DEBUG_TELNET
  if (telnetClient.connected()) {
    const size_t len = debugTimeText.length();
    const char *buffer = debugTimeText.c_str();
    telnetClient.write(buffer, len);
    handleTelnetClient();
  }
#endif
  return size;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#ifdef DEBUG_TELNET
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
    println(String("ESP_Power_Monitoring - Build: ") + String(__DATE__) + " " +  String(__TIME__));
    println(String("ESP_Power_Monitoring - Version: ") + String(VERSION));
    println("=====================================================");
    println();
  }
}
#endif

#if !defined(NO_GLOBAL_INSTANCES) 
Logger Log;
#endif
