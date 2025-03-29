#include <HX711.h>
#include <SD.h>
#include <SPI.h>
#include <BluetoothSerial.h>

//timer
//hw_timer_t * timer = NULL;

#define SD_MISO_PIN 19
#define SD_MOSI_PIN 23
#define SD_SCK_PIN 18
#define SD_CS_PIN 5

static String fileName = "";  /* Store file name static so after first change it wont be renamed again and 
again and for the function to be called multiple times */
static bool firstTIMEcheck = true; //to check if its the first time writing the SD down and put collum values and titles
static uint8_t Contador = 1; //contador para cuantos prints en la terminal

BluetoothSerial SerialBT;

String GroundStation_MAC = "";

HX711 scale;
File myFile;

uint8_t LaunchPin = 14; //Pin for pyrochannel
uint8_t ContinuityPin = 12;
uint8_t Safe_Pin = 15;                
const uint8_t LOADCELL_DOUT_PIN = 32;  //Data pin for hx711
const uint8_t LOADCELL_SCK_PIN = 33;   //sck pin for hx711
//cambio(presion)
const uint8_t PRESSURE_SENSOR_PIN = 13; //falta definir pin
uint8_t FILE_RESET_Button = 22;  //es en un digital output pin

float ForceValue; 
uint32_t StartTime = millis();
uint32_t instance;
int32_t CalibrationValue;
uint32_t KnownWeight;
float PressureValue; //valor del sensor como float (POR DEFINIR)

char incomingChar;
static String Message = "";
String Calibrate = "calibrate";
String Launch = "launch";
String SetScale = "set";
String Measure = "measure";
String EndStop = "stop";
String NewFile = "newfile";

bool SDSTATE;
bool SD_Activated;
uint8_t state = 0;

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

void loop(){
  Message = "";
  BluetoothRead();
  delay(50);
  
  if (state != 0 && fileName == "") {
    state = 0;  
  }

  switch(state){
    case 0: 
      if (!SDSTATE) {
        SerialBT.print("SD not detected, please insert before continuing. \n");
        break;
      }
      if (fileName == "") {
        if (Message != "newfile"){
          SerialBT.print("Enter namefile before any commands. \n");
          break;
        }
        SerialBT.print("Enter file name: ");
        while (!SerialBT.available()) {}
        fileName = SerialBT.readStringUntil('\n');
        fileName.trim();
        fileName = "/" + fileName + ".txt";
        SerialBT.print("File name set: " + fileName + "\n");
      }
      break;
      
    case 1:
      if (Message == Measure) {
        scale.set_scale(CalibrationValue);
        Message = "";
        while(Message != EndStop){
          BluetoothRead();
          MeasureMode();
        }
      }
      break;
      
    case 2:
      if(Message == Launch){
        scale.set_scale(CalibrationValue);
        if(SDSTATE == true){
          for(uint8_t i = 0; i< 10;i++){
            SerialBT.print(10 - i);
            SerialBT.print("\n");
            delay(1000);
          
            if(Message == EndStop){
              SerialBT.print("ABORT\n");
              break;
            }
          }

          digitalWrite(LaunchPin, HIGH);
          delay(1000);
          Message = "";
          while(Message != EndStop) {
            BluetoothRead();
            launchMode();
          }
        }
        else{
          SerialBT.print("ABORT\n");
        }
      }
      break;
      
    default:
      if(Message == Calibrate) {
        SerialBT.print("Enter value of known weight in grams: \n");
        while(!SerialBT.available()){}
        delay(500);
        BluetoothRead(); 
        KnownWeight = atoi(Message.c_str());
        SerialBT.print(KnownWeight);
        SerialBT.print("\n");

        if(scale.is_ready()){
          scale.set_scale();
          SerialBT.print("Remove Weight from scale\n");
          delay(3000);
          SerialBT.print("Now Taring\n");
          scale.tare();
          SerialBT.print("Tare Done\n");
          delay(1000);
          SerialBT.print("Place Known weight on scale\n");
          delay(5000);
          SerialBT.print("Now Measuring\n");
          delay(200);

          int32_t reading = scale.get_units(50);
          CalibrationValue = reading/KnownWeight;
          SerialBT.print("Calibration Value: ");
          SerialBT.print(CalibrationValue);
          SerialBT.print("\n");
        }
      }
      else if(Message == SetScale){
        SerialBT.print("Enter Known Calibration Value: \n");
        while(!SerialBT.available()){}
        delay(500);
        BluetoothRead();
        CalibrationValue = atoi(Message.c_str());
        SerialBT.print(CalibrationValue);
        SerialBT.print("\n");
      }
      break;
  }
  
  if (state == 0) {
    if(digitalRead(Safe_Pin) == HIGH){
      SerialBT.print("Safe Pin Removed\n");
      state++;
    }
  }

  if (state == 1) {
    if (digitalRead(ContinuityPin) == HIGH) {
      SerialBT.print("Continuity on charge\n");
      state++;
    }
  } 
}

void BluetoothRead(){
  while(SerialBT.available()) {
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