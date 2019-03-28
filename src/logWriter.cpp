#include <Arduino.h>

#include "logWriter.h"
#include "JsonConfiguration.h"
#include "ATM90E32.h"

/*********************************/
/********* PUBLIC METHOD *********/
/*********************************/

logWriter::logWriter()
{
  /* Initialize SPIFFS */
  if (!SPIFFS.begin()) {
    Serial.println("failed to initialize SPIFFS");
  }
}

logWriter::~logWriter()
{
}

void logWriter::setup()
{
}

void logWriter::handle(struct tm *timeinfo)
{
  static unsigned long oldMillis;

  if ((millis() - oldMillis) >= (unsigned long)(Configuration.m_timeWriteLog * 1000))
  {
    oldMillis = millis();
    
    /* Reset Buffer */
    memset(m_dataToWrite, 0, sizeof(m_dataToWrite));

    /* Add Timestamp */
    sprintf(m_dataToWrite, "%02d/%02d/%d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
    /* Add Data */
    Monitoring.handle();
    metering lineA = Monitoring.getLineA();
    sprintf(m_dataToWrite, "%s;%.2f;%.2f;%.2f", m_dataToWrite, lineA.voltage, lineA.current, lineA.power);
    metering lineB = Monitoring.getLineB();
    sprintf(m_dataToWrite, "%s;%.2f;%.2f;%.2f", m_dataToWrite, lineB.voltage, lineB.current, lineB.power);
    metering lineC = Monitoring.getLineC();
    sprintf(m_dataToWrite, "%s;%.2f;%.2f;%.2f", m_dataToWrite, lineC.voltage, lineC.current, lineC.power);
    double freq = Monitoring.getFrequency();
    sprintf(m_dataToWrite, "%s;%.2f", m_dataToWrite, freq);

    /* Write data */
    write();  
  }
}

/**********************************/
/********* PRIVATE METHOD *********/
/**********************************/

bool logWriter::openFile(const String &name)
{
  m_logFile = SPIFFS.open(name, "w");

  if (!m_logFile) {
		Serial.println("Failed to create log file");
		return false;
	}

  return true;
}

bool logWriter::write()
{
  openFile("log.csv");
  size_t sizeWr = m_logFile.write((const uint8_t *)m_dataToWrite, strlen(m_dataToWrite));
  closeFile();
  return sizeWr == strlen(m_dataToWrite);
}

void logWriter::closeFile()
{
  m_logFile.close();
}

#if !defined(NO_GLOBAL_INSTANCES)
logWriter LogWriter;
#endif
