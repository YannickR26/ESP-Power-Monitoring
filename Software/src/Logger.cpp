#include "Logger.h"
#include "settings.h"
#include "Mqtt.h"


/********************************************************/
/******************** Public Method *********************/
/********************************************************/
void Logger::setup()
{
  addTimeToString = true;

#ifdef DEBUG_BY_SERIAL
  Serial.begin(115200);
  delay(100);

  println();
  println("Starting...");
#endif
}

void Logger::setupTelnet()
{
#ifdef DEBUG_BY_TELNET
  // Setup telnet server for remote debug output
  telnetServer.begin(23);
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

char *Logger::getDateTimeString()
{
  static char time[30];

  time_t t = ::time(0);
  struct tm *timeinfo = localtime(&t);

  sprintf(time, "%02d/%02d/%d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  return time;
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
  char *time = getDateTimeString();
  s = "[" + String(time) + "] " + s;
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
    println("==========================================");
    println(String(F("---------- ESP Power Monitoring ----------")));
    println(String(F("  Version: ")) + VERSION);
    println(String(F("  Build: ")) + BUILD_DATE);
    println("==========================================");
    println();
  }
}
#endif

#if !defined(NO_GLOBAL_INSTANCES)
Logger Log;
#endif
