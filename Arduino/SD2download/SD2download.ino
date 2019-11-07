#include <WiFiClient.h>
#include <ESP32WebServer.h> //  https://github.com/nhatuan84/esp32-webserver to C:/Users/[YOUR_USER_NAME]/Documents/Arduino/libraries
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <mySD.h> // https://github.com/nhatuan84/esp32-micro-sdcard to C:/Users/[YOUR_USER_NAME]/Documents/Arduino/libraries
// http://www.iotsharing.com/2019/07/how-to-turn-esp-with-sdcard-or-spiffs-a-web-file-server.html?m=1

#include "IF.h" //Our HTML webpage contents
#include "credentials.h" // // your WLAN login data
//String serverIndex = "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
//"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
//    "<input type='file' name='update'>"
//    "<input type='submit' value='Upload'>"
//"</form>"
//"<div id='prg'>progress: 0%</div>"
//"<script>"
//"$('form').submit(function(e){"
//    "e.preventDefault();"
//      "var form = $('#upload_form')[0];"
//      "var data = new FormData(form);"
//      " $.ajax({"
//            "url: '/update',"
//            "type: 'POST',"               
//            "data: data,"
//            "contentType: false,"                  
//            "processData:false,"  
//            "xhr: function() {"
//                "var xhr = new window.XMLHttpRequest();"
//                "xhr.upload.addEventListener('progress', function(evt) {"
//                    "if (evt.lengthComputable) {"
//                        "var per = evt.loaded / evt.total;"
//                        "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
//                    "}"
//               "}, false);"
//               "return xhr;"
//            "},"                                
//            "success:function(d, s) {"    
//                "console.log('success!')"
//           "},"
//            "error: function (a, b, c) {"
//            "}"
//          "});"
//"});"
//"</script>";

const char* ssid = "***";
const char* password = "***";
const char* ssidAP = "TrackTracker";
bool WIFIAP= true; // if true ESP32 will be a WLAN Access point "TrackTracker", no pwd, else it will log into your WLAN (credentials needed), good while testing 

ESP32WebServer server(80);
File root;
bool opened = false;

String printDirectory(File dir, int numTabs) {
  String response = "";
  dir.rewindDirectory();
  
  while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');   // we'll have a nice indentation
     }
     // Recurse for directories, otherwise print the file size
     if (entry.isDirectory()) {
       printDirectory(entry, numTabs+1);
     } else {
       response += String("<a href='") + String(entry.name()) + String("'>") + String(entry.name()) + String("</a>") + String("</br>");
     }
     entry.close();
   }
   return String("List files:</br>") + response + String("</br></br> Upload file:") + serverIndex;
}

void handleRoot() {
  root = SD.open("/");
  String res = printDirectory(root, 0);
  server.send(200, "text/html", res);
}

bool loadFromSDCARD(String path){
  path.toLowerCase();
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".txt")) dataType = "text/plain";
  else if(path.endsWith(".zip")) dataType = "application/zip";  
  Serial.println(dataType);
  File dataFile = SD.open(path.c_str());

  if (!dataFile)
    return false;

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleNotFound(){
  if(loadFromSDCARD(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

void setup(void){
  Serial.begin(115200);
  if (WIFIAP== true)  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    Serial.println("");
    Serial.print("Creating to ");
    Serial.println(ssid);
    delay(500);
    Serial.print("Created AP: ");
    Serial.println(WiFi.softAPIP());
  }
  else {
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  }

  
  //use IP or iotsharing.local to access webserver
  if (MDNS.begin("iotsharing")) {
    Serial.println("MDNS responder started");
  }
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  //handle uri  
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  /*handling uploading file */
  server.on("/update", HTTP_POST, [](){
    server.sendHeader("Connection", "close");
  },[](){
    HTTPUpload& upload = server.upload();
    if(opened == false){
      opened = true;
      root = SD.open((String("/") + upload.filename).c_str(), FILE_WRITE);  
      if(!root){
        Serial.println("- failed to open file for writing");
        return;
      }
    } 
    if(upload.status == UPLOAD_FILE_WRITE){
      if(root.write(upload.buf, upload.currentSize) != upload.currentSize){
        Serial.println("- failed to write");
        return;
      }
    } else if(upload.status == UPLOAD_FILE_END){
      root.close();
      Serial.println("UPLOAD_FILE_END");
      opened = false;
    }
  });
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
