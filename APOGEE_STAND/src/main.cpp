#include <Arduino.h>
#include "STATE.h"
#define SD_MISO_PIN 19
#define SD_MOSI_PIN 23
#define SD_SCK_PIN 18
#define SD_CS_PIN 5

State state = NAME;
static String fileName="";
static bool firstTIMEcheck= true;
static uint8_t contador =1;

BluetoothSerial SerialBT;
HX711 scale;
File myFile;
char incomingChar;
static String Message="";
bool SDSTATE;
bool SD_Activated;

void BluetoothRead(){
  while(SerialBT.av,failable()) {
    Message = SerialBT.readStringUntil('\n');
    Message.trim();
  }
}

void MeasureMode(){
  ForceMeasure();
  PressureValue = readPressureSensor();
  dataStore();
  dataTransfer();
  instance = millis() - StartTime;
}

void launchMode(){
  ForceMeasure();
  PressureValue = readPressureSensor();
  if(SDSTATE){
    dataStore();
  }
  dataTransfer();
  instance = millis() - StartTime;
}

void dataStore(){
  myFile = SD.open(fileName, FILE_APPEND);
  if (firstTIMEcheck==true) {
    Serial.println("First thing printed");
    SerialBT.println("First thing printed");
    myFile.println("Force,Pressure,Time");
    firstTIMEcheck= !firstTIMEcheck;
  }
  if (myFile) {
    String data = String(ForceValue)+","+String(PressureValue)+","+String(instance);
    myFile.println(data);
    myFile.close();
  } else {
    SerialBT.print("SD ERROR;\n");
    SDSTATE = false;
  }
}

void ForceMeasure(){
  if(scale.is_ready()){
    ForceValue = scale.get_units()/1000;
  }
}

void dataTransfer(){
  SerialBT.print(String(Contador) + ". F: " + String(ForceValue) + ", PSI: "+ String(PressureValue) + " T: "+ String(instance) + "\n");
  Serial.print(String(Contador) + ". F: " + String(ForceValue) + ", PSI: "+ String(PressureValue) + " T: "+ String(instance) + "\n");
  Contador++;
}

float readPressureSensor() {
  int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
  float voltage = sensorValue * (3.3 / 4095.0);
  float pressure = (voltage / 3.3) * 100;
  return pressure;
}



void setup() {
  Serial.begin(9600);
  SerialBT.begin("Test Stand");

  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(LaunchPin, OUTPUT);
  digitalWrite(LaunchPin, LOW);
  pinMode(Safe_Pin, INPUT);
  pinMode(ContinuityPin, INPUT);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();
  digitalWrite(SD_CS_PIN, HIGH);

  while (!SerialBT.available()) {
  }

  if (!SD.begin(SD_CS_PIN)) {
    SerialBT.print("Error, no SD initialized\n");
    SDSTATE = false;
  }
  else{
    SerialBT.print("SD Connected\n");
    SDSTATE = true;
  }
  
  SerialBT.print("Before anything 'newfile' to set the name that will be recorded in the SD if already set then skip \n ready to go \n 'calibrate' to calibrate scale \n 'set' to use known calibration value \n 'measure' to test without launching \n 'launch' to launch \n stop while running to stop \n  CASE SENSITIVE \n");
  
}

void loop() {

  // ACTIONS
  switch (state) {
    case NAME:
      if (!SDSTATE) {
        SerialBT.print("SD not detected, please insert before continuing. \n");
        break;
      }
    
      if (fileName == "") { // Only if no filename exists
        SerialBT.print("Send 'newfile' to set filename \n");
        
        while (!SerialBT.available()) { 
          delay(100);  // Prevents excessive message flooding
        }
    
        String tempFileName = SerialBT.readStringUntil('\n');
        tempFileName.trim();
        if (tempFileName.length() > 0) {  // Ensure a valid name was entered
          fileName = "/" + tempFileName + ".txt";
          SerialBT.print("File name set: " + fileName + "\n");
        }
      }
      break;
    case CALIBRATE:
      break;
    case SETSCALE:
    if(Message == SetScale){
      SerialBT.print("Enter Known Calibration Value: \n");
      while(!SerialBT.available()){}
      delay(500);
      BluetoothRead();
      CalibrationValue = atoi(Message.c_str());
      SerialBT.print(CalibrationValue);
      SerialBT.print("\n");
    }
    break;

      break;
    case MEASURE:
      if (Message == Measure) {
        scale.set_scale(CalibrationValue);
        Message = "";
        while(Message != EndStop){
          BluetoothRead();
          MeasureMode();  //ONLY BT
          
        }
      break;
    case LAUNCH:
      break;
    case RENAME:
        break;
    case STOP:
      break;
  }

  // TRANSITIONS
  switch (state) {
    case NAME:
      break;
    case CALIBRATE:
      break;
    case SETSCALE:
    break;
      break;
    case MEASURE:
      if(Message == EndStop){
        SerialBT.print("ABORT\n");
        break;
      }
      break;
    case LAUNCH:
      break;
    case STOP:
      break;
  }
  

}
