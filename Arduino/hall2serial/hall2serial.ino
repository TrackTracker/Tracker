#define SensorPWR 15 //GPIO the connector to toogle an additional LED betwenn pin 15 and GND

void setup() {
  Serial.begin(115200);
  pinMode(SensorPWR, OUTPUT); // Port als Ausgang schalten  
  pinMode(LED_BUILTIN, OUTPUT);
}

 
void loop() {
 
    int measurement = 0;
    float BatteryVoltage = 0;
    BatteryVoltage = analogRead(35)/4096.0*7.445; 

    measurement = hallRead();
 
    Serial.print("Hall sensor measurement: ");
    Serial.println(measurement); 
    Serial.print("Battery Voltage ");
    Serial.println(BatteryVoltage); 
    if (measurement < 0) {
      digitalWrite(SensorPWR, HIGH); //LED an 15 ein
      digitalWrite(LED_BUILTIN, HIGH); // Builtin LED aus
    }
    else{
      digitalWrite(SensorPWR, LOW); //LED an 15 aus
      digitalWrite(LED_BUILTIN, LOW);
    }
    
    delay(1000);
}
