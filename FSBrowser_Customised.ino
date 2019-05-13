/* 
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

#define DBG_OUTPUT_PORT Serial

const char* ssid = "SEUN 6228";
const char* password = "Gorill@r94";
const char* host = "esp8266fs";

/*  Additional code

*/
#define TOKEN "jN80oNCqadNp1tvMQJcf"
#define DEBUG 0
#define GPIO0_PIN 29
#define GPIO2_PIN 31

char thingsboardServer[] = "demo.thingsboard.io";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
int status = WL_IDLE_STATUS;

bool stringComplete = false;  // whether the string is complete
int ledPin = 0;
String statusTopic="";

boolean gpioState[] = {false, false};
String distanceJson = "{distance:100}";
int distance = 100;  ///DDEBUGGING
/*
  End of additional code
*/

ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;


/*  Telemetry data

*/

//telemetryData["distance"] = distance;
//telemetryData["29"] = gpioState[0];
//telemetryData["31"] = (bool)false;
 
//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}"; 
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
 }

 void handleControlGpio(){
    if(server.args() == 0) return server.send(500, "text/plain", "Bad Request");
    String pinName = server.argName(0);
    Serial.println(pinName);
    int pin = pinName.toInt();
    String pinValue =server.arg(pinName);
    Serial.println(pinValue);
    int enable = pinValue.toInt();
    set_gpio_status(pin,enable);
    server.send(200,"text/plain","OK");
 }

 void  sendTelemetry(){
    StaticJsonBuffer<200> telemetryBuffer;
    JsonObject& telemetryData = telemetryBuffer.createObject();
    String telemetryString;
    telemetryData["29"] = gpioState[0]? true : false;
    telemetryData["31"] = gpioState[1]? true : false;
    telemetryData["distance"] = distance;
    telemetryData.printTo(telemetryString);
    Serial.println(telemetryString);
    server.send(200, "text/json", telemetryString); //for testing
 }

 
void setup(void){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
  
  
  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  
  server.on("/gpio", HTTP_POST,handleControlGpio);
  
  //get telemetry value read from TM4C
  server.on("/telemetry", HTTP_GET, sendTelemetry);
  
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  client.setServer( thingsboardServer, 1883 );
  client.setCallback(on_message);

  DBG_OUTPUT_PORT.println("MQTT Client started");
}
 
void loop(void){
  server.handleClient();

  if (Serial.available()){
    serialEvent();
  }
  if ( !client.connected() ) {
    reconnect();
//    Serial.println("problem here");
  }
  client.loop();
}


/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    String inString = (String)Serial.readStringUntil('\n');
    // add it to the inputString:
//    char buf[20];
//    inString.toCharArray(buf,sizeof(inString));
    //Serial.printf("-d %s\r\n",buf);
    SerialEvaluate(inString);
  }
}

//Main interpreter function
void SerialEvaluate(String inputString){
    char  command[3];
    inputString.substring(0,2).toCharArray(command,3);
    String output=inputString.substring(3);   

    //for controlling ESP8266 LED
    if (strcmp("-l",command) == 0){
        int state = output.toInt();
        Serial.printf("-d %s\r","ACK");   //respond data received
        digitalWrite(ledPin,state);
    }

    //for sending status to server
    if (strcmp("-s",command) == 0){
       getDistance(output);     
       client.publish("v1/devices/me/attributes",output.c_str());  
     }   
    if (strcmp("-g",command) == 0){
//       char buf[32]
//       topic.toCharArray(buf,32);
//       Serial.print(statusTopic);
       getLED(output);
       publishToTopic(statusTopic,output);
    }  
}


//helper method to publish to any topic
void publishToTopic(String top,String payload){
      client.publish(top.c_str(),payload.c_str());
      client.publish("v1/devices/me/attributes",payload.c_str());     
 }
void on_message(const char* topic, byte* payload, unsigned int length) {
  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    //send debug message
    Serial.printf("-d parseObject() failed \r");
    return;
  }

  // Check request method
  String methodName = String((const char*)data["method"]);

  if (methodName.equals("getGpioStatus")) {
    // Reply with GPIO status
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    statusTopic = String(responseTopic);
    get_gpio_status();
//    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
  } else if (methodName.equals("setGpioStatus")) {
    // Update GPIO status and reply
    set_gpio_status(data["params"]["pin"], data["params"]["enabled"]);
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    statusTopic = String(responseTopic);
    get_gpio_status();

    //for testing,comment later
    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  }
}

String get_gpio_status() {
  // Prepare gpios JSON payload string
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data[String(GPIO0_PIN)] = gpioState[0] ? true : false;
  data[String(GPIO2_PIN)] = gpioState[1] ? true : false;
  char payload[256];
  data.printTo(payload, sizeof(payload));
  String strPayload = String(payload);
//  getGpio = true;
     //String strPayload = "";
     //Serial.print("-s \r");
//  Serial.println(strPayload);

  getLED(strPayload);
  return strPayload;
}

void set_gpio_status(int pin, boolean enabled) {
  if (pin == GPIO0_PIN) {
    // Output GPIOs state
    Serial.printf("-r %d\r",enabled? 1 : 0);
    // Update GPIOs state
    gpioState[0] = enabled;
  } else if (pin == GPIO2_PIN) {
    // Output GPIOs state
    Serial.printf("-g %d\r",enabled? 1 : 0);
    // Update GPIOs state
    gpioState[1] = enabled;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
      Serial.printf("-d Connected in Station mode \r");
    }
//    Serial.print("-d Connecting to ThingsBoard");
    // Attempt to connect (clientId, username, password)
    if ( client.connect("TM4C123", TOKEN, NULL) ) {
//      Serial.println( "-d [DONE]" );
      // Subscribing to receive RPC requests
      client.subscribe("v1/devices/me/rpc/request/+");
      // Sending current GPIO status
//      Serial.println("-d Sending GPIO status");
//      client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
        get_gpio_status();
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void getDistance(String distanceJson){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& distanceData = jsonBuffer.parseObject(distanceJson);
  if (distanceData.success())
  {
    distance = distanceData["distance"].as<int>();
    return;
  }
}
void getLED(String ledJson){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& ledData = jsonBuffer.parseObject(ledJson);
  Serial.println("In get led");
  if (ledData.success()){
    gpioState[0] = ledData["29"].as<bool>();
    gpioState[1] = ledData["31"].as<bool>();
    return;
  }
}
