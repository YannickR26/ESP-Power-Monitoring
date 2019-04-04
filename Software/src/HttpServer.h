#pragma once

#include <ESP8266WebServer.h>
#include <ESP8266FtpServer.h>

class HttpServer
{
  public:
	HttpServer() ;
	virtual ~HttpServer();

	void setup(void);
	void handle(void);

	String getContentType(String filename);
    bool handleFileRead(String path);
    void handleFileUpload();

    ESP8266WebServer& webServer();

  protected:
  	void sendOk();
  	void sendKo(const String & message);
  	void sendOkAnswerWithParams(const String & params);
	  
  private:
  	ESP8266WebServer  m_webServer;
  	FtpServer         m_ftpServer;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern HttpServer HTTPServer;
#endif
