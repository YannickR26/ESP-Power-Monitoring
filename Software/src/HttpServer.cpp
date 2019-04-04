#include <FS.h>
#include <ESP8266mDNS.h>

#include "HttpServer.h"
#include "JsonConfiguration.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

HttpServer::HttpServer()
{
}

HttpServer::~HttpServer()
{
}

void HttpServer::setup(void)
{
  MDNS.begin(Configuration.m_hostname.c_str()); 
  MDNS.addService("http", "tcp", 80);

  m_ftpServer.begin(Configuration.m_ftpLogin, Configuration.m_ftpPasswd);
  MDNS.addService("ftp", "tcp", 21);

  m_webServer.begin();

  //called when the url is not defined here
  //use it to load content from SPIFFS
  m_webServer.onNotFound([]() {
    HTTPServer.webServer().send(404, "text/plain", "FileNotFound");
  });
}

void HttpServer::handle(void)
{
  m_webServer.handleClient();
  m_ftpServer.handleFTP();
}

String HttpServer::getContentType(String filename)
{
	if (m_webServer.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

// send the right file to the client (if it exists)
bool HttpServer::handleFileRead(String path)
{
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";          // If a folder is requested, send the index file
  }
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = m_webServer.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

// upload a new file to the SPIFFS
void HttpServer::handleFileUpload()
{ 
  static File fsUploadFile;
  HTTPUpload& upload = m_webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if(!filename.startsWith("/")) {
      filename = "/"+filename;
    }
    Serial.print("handleFileUpload Name: "); 
    Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if(fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); 
      Serial.println(upload.totalSize);
      m_webServer.sendHeader("Location","/success.html");      // Redirect the client to the success page
      m_webServer.send(303);
    } 
    else {
      m_webServer.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

ESP8266WebServer& HttpServer::webServer() 
{
  return m_webServer;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void HttpServer::sendOk()
{
	m_webServer.send(200, "application/json", "{\"result\":true}");
}

void HttpServer::sendOkAnswerWithParams(const String & params)
{
	String data("{\"result\":true, \"data\":");
	data += params;
	data += "}";
	m_webServer.send(200, "application/json", data);
}

void HttpServer::sendKo(const String & message)
{
	String data("{\"result\":false, \"message\":\"");
	data += message;
	data += "\"}";
	m_webServer.send(400, "application/json", data);
}

#if !defined(NO_GLOBAL_INSTANCES) 
HttpServer HTTPServer;
#endif
