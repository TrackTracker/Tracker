/*
 * Micro SD Shield - Datalogger
 *
 * This example shows how to log data from an analog sensor
 * to an SD card using the SD library.
 * 
 * Here we log the Hall sensor data and read the filesize 
 *
 * The WeMos Micro SD Shield uses:
 * D5, D6, D7, D8, 3V3 and G
 *
 * The shield uses SPI bus pins:
 * D5 = CLK
 * D6 = MISO
 * D7 = MOSI
 * D8 = CS
 *
 * The SD card library uses 8.3 format filenames and is case-insensitive.
 * eg. IMAGE.JPG is the same as image.jpg
 *
 * created  24 Nov 2010
 * modified 9 Apr 2012 by Tom Igoe
 *
 * This example code is in the public domain.
 * https://github.com/esp8266/Arduino/blob/master/libraries/SD/examples/Datalogger/Datalogger.ino
 */

#include <SPI.h>
#include <SD.h>

// LOLIN Micro SD Shield V1.2.0: 4 (Default)
const int chipSelect = 4;
File root;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

void loop()
{
  // make a string for assembling the data to log:
  String dataString = "";
  float BatteryVoltage = 0;
  BatteryVoltage = analogRead(35)/4096.0*7.445; 
  dataString += String(BatteryVoltage);
  dataString += "\r\n";

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  //File dataFile = SD.open("datalog2.txt", FILE_WRITE);
  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");
  File file = SD.open("/datalog2.txt",FILE_APPEND);
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
  }
  else {
    
    Serial.println("File already exists");  
    Serial.println(file.size());  
  }
  // if the file is available, write to it:
  if (file) {
    file.print(dataString);
    file.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
  delay(1000);
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
