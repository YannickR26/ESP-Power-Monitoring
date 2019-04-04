#pragma once

#include <FS.h>

class logWriter
{
  public:
    logWriter();
    virtual ~logWriter();

    void setup();
    void handle(struct tm *timeinfo);

  protected:
    bool openFile(const String &name);
    bool write();
    void closeFile();

  private:
    File m_logFile;
    char m_dataToWrite[300];

};

#if !defined(NO_GLOBAL_INSTANCES)
extern logWriter LogWriter;
#endif