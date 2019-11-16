//based on  http://www.iotsharing.com/2019/07/how-to-turn-esp-with-sdcard-or-spiffs-a-web-file-server.html?m=1
#include <WiFiClient.h>
#include <ESP32WebServer.h> //  https://github.com/nhatuan84/esp32-webserver to C:/Users/[YOUR_USER_NAME]/Documents/Arduino/libraries
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <mySD.h> // https://github.com/nhatuan84/esp32-micro-sdcard to C:/Users/[YOUR_USER_NAME]/Documents/Arduino/libraries

#include "IF.h" //Our HTML webpage contents
#include "credentials.h" // // your WLAN login data

const char* ssidAP = "TrackTracker";
bool WIFIAP= false; // if true ESP32 will be a WLAN Access point "TrackTracker", no pwd, else it will log into your WLAN (credentials in credentials.h needed), good while testing 

#include <TinyGPS.h> //  to C:/Users/[YOUR_USER_NAME]/Documents/Arduino/libraries
#include <SoftwareSerial.h>
TinyGPS gps;  // The TinyGPS++ object
SoftwareSerial ss(16, 17); // ss(4, 5); The serial connection to the GPS device was 16,15
float latitude , longitude;
int year , month , date, hour , minute , second;
String date_str , time_str , lat_str , lng_str;
int pm;

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
       Serial.println('entry.name()'); 
       response += String("<tr> <td ALIGN=CENTER > ") + String("<a href='") + String(entry.name()) + String("'>") + String(entry.name()) + String("</a>") + String("</td> </tr>");
     }
     entry.close();
   }
   String ret_str= "<table ALIGN=CENTER style=width:50%> ";
   ret_str += "<tr> <th> List files:</th> </tr>";
   ret_str += response;
   ret_str += "<tr> <td > </tr> </td >";
   ret_str += "<tr> <td > Upload file: </br> ";
   ret_str += serverIndex;
   ret_str += "</tr> </td > </table> </br></br>";
   return ret_str;
}

void handleRoot() {
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age; 
  char sdate[32]="**********";
  char stime[32]="**********";
  char slon[32] ="**********";
  char slat[32] ="**********";
  char sage[32] ="**********";
  float flat, flon;
  gps.f_get_position(&flat, &flon, &age);
  dtostrf(flon, 4, 10, slon);
  dtostrf(flat, 4, 10, slat);
  dtostrf(age, 4, 10, sage);
  Serial.print(slat);Serial.print(slon);Serial.println(sage);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  sprintf(sdate, "%02d/%02d/%02d",month, day, year );
  Serial.println(sdate);
  sprintf(stime, "%02d:%02d:%02d",hour, minute, second);
  Serial.println(stime);
    
  // Prepare the http response
  //first GPS
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n <!DOCTYPE html> <html> <head> <title>GPS Interfacing with D32 for TrackTracker</title> <style>";
  s += "a:link {background-color: YELLOW;text-decoration: none;}";
  s += "table, th, td {border: 1px solid black;} </style> </head> <body> <h1  style=";
  s += "font-size:300%;";
  s += " ALIGN=CENTER> GPS Interfacing with D32 for TrackTracker </h1>";
  s += "<p ALIGN=CENTER style=""font-size:150%;""";
  s += "> <b>Location Details</b></p> ";
  s += "<table ALIGN=CENTER style=width:50%> ";
  s += "<tr> <th>Latitude</th>";
  s += "<td ALIGN=CENTER >";
  s += slat;
  s += "</td> </tr> <tr> <th>Longitude</th> <td ALIGN=CENTER >";
  s += slon;
  s += "</td> </tr> <tr>  <th>Age</th> <td ALIGN=CENTER >";
  s += sage;
  s += "</td> </tr> <tr>  <th>Date</th> <td ALIGN=CENTER >";
  s += sdate;
  s += "</td></tr> <tr> <th>Time</th> <td ALIGN=CENTER >";
  s += stime;
  s += "</td>  </tr>";
  s += "<tr> <td colspan=""2""> <p align=center><a style=""color:RED;font-size:125%;"" href=""http://maps.google.com/maps?&z=15&mrt=yp&t=k&q=";
  s += lat_str;
  s += "+";
  s += lng_str;
  s += """ target=""_top"">Click here!</a> To check the location in Google maps.</p> </td> </tr> </table> </br> </br> ";
  
  //now add hall and voltage data
  int measurement = 0;
  float BatteryVoltage = 0;
  BatteryVoltage = analogRead(35)/4096.0*7.445; 
  measurement = hallRead();
  s += "<p ALIGN=CENTER style=""font-size:150%;> <b>D32 details</b></p> ";
  s += "<table ALIGN=CENTER style=width:50%>";
  s += "<tr> <th>Battery voltage</th> <td ALIGN=CENTER >";
  s += String(BatteryVoltage);
  s += "</td> </tr> <tr>  <th>Hall-Sensor</th> <td ALIGN=CENTER >";
  s += String(measurement);
  s += "</td>  </tr> </table> </br> </br>";
  
  //now SD list
  root = SD.open("/");
  String res = printDirectory(root, 0);
  s += "<p ALIGN=CENTER style=""font-size:150%;> <b>SD_card content</b></p> "; 
  s += res; 
  // finally some WLAN data
  long rssi = WiFi.RSSI();
  char srssi[32] ="**********";
  String WlanMode ="**********";
  String sIP ="**********";
  String sssid = "**********";
  dtostrf(rssi, 4, 1, srssi);
  if (WIFIAP==true)
    WlanMode="Providing AP";
  else
    WlanMode="Connected to AP";
  if (WIFIAP==true)
   { sIP=WiFi.softAPIP().toString();}
  else
   { sIP=WiFi.localIP().toString();   }
  sssid =WiFi.SSID();
  s += "<p ALIGN=CENTER style=""font-size:150%;""> <b>Wlan Details</b></p> ";
  s += "<table ALIGN=CENTER style=width:50%> ";
  s += "<tr> <th>WLAN mode </th>";
  s += "<td ALIGN=CENTER >";
  s += WlanMode;
  s += "</td> </tr> <tr> <th>SSID</th> <td ALIGN=CENTER >";
  s += sssid;
  s += "</td> </tr> <tr> <th>IP</th> <td ALIGN=CENTER >";
  s += sIP;
  s += "</td> </tr> <tr>  <th>Signal Strength</th> <td ALIGN=CENTER >";
  s += srssi;
  s += "</td> </tr>  </table> </br> </br> ";

  s += "</body> </html> \n";
  server.send(200, "text/html", s);
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
  int cnt=0;
  Serial.begin(115200);
  ss.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  if (WIFIAP== true)  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    Serial.println("");
    Serial.print("Creating AP ");
    Serial.println(ssid);
    delay(500);
    Serial.print("Created AP with IP: ");
    Serial.println(WiFi.softAPIP());
  }
  else {
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    cnt+=1;
    delay(500);
    Serial.print(".");
    if (cnt >10)
    {ESP.restart();}
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
    while (ss.available())
  {
  gps.encode(ss.read());
  }
  Serial.println("GPS");
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH); // Builtin LED aus
  delay(100);
}
