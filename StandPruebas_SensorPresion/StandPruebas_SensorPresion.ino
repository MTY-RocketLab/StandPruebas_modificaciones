#include <HX711.h>
#include <SD.h>
#include <SPI.h>
#include <BluetoothSerial.h>

#define SD_MISO_PIN 19
#define SD_MOSI_PIN 23
#define SD_SCK_PIN 18
#define SD_CS_PIN 5

BluetoothSerial SerialBT;

String GroundStation_MAC = "";

HX711 scale;
File myFile;

uint8_t LaunchPin = 14; //Pin for pyrochannel
uint8_t ContinuityPin = 12;
uint8_t Safe_Pin = 15;                
const uint8_t LOADCELL_DOUT_PIN = 32;  //Data pin for hx711
const uint8_t LOADCELL_SCK_PIN = 33;   //sck pin for hx711
const uint8_t PRESSURE_SENSOR_PIN = 34; // Sensor de presión

float ForceValue;
uint32_t StartTime;
uint32_t instance;
int32_t CalibrationValue;
uint32_t KnownWeight;
float PressureValue; // valor del sensor de presión

char incomingChar;
String Message = "";
String Calibrate = "calibrate";
String Launch = "launch";
String SetScale = "set";
String Measure = "measure";
String EndStop = "stop";

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

  StartTime = millis();
  
  while (!SerialBT.available()) {}

  if (!SD.begin(SD_CS_PIN)) {
    SerialBT.print("Error, no SD initialized\n");
    SDSTATE = false;
  } else {
    SerialBT.print("SD Connected\n");
    SDSTATE = true;
  }
  
  SerialBT.print("ready to go \n 'calibrate' to calibrate scale \n 'set' to use known calibration value \n 'measure' to test without launching \n 'launch' to launch \n stop while running to stop \n CASE SENSITIVE \n");
}

void loop() {
  Message = "";
  BluetoothRead();
  delay(100);

  switch(state) {
    case 1:
      if (strcmp(Message.c_str(), Measure.c_str()) == 0) {
        scale.set_scale(CalibrationValue);
        Message = "";

        while(strcmp(Message.c_str(), EndStop.c_str()) != 0){
          BluetoothRead();
          MeasureMode();
        }
      }
      break;

    case 2:
      if(strcmp(Message.c_str(), Launch.c_str()) == 0){
        scale.set_scale(CalibrationValue);
        if(SDSTATE == true){
          for(uint8_t i = 0; i < 10; i++){
            SerialBT.print(10 - i);
            SerialBT.print("\n");
            delay(1000);

            if(strcmp(Message.c_str(), EndStop.c_str()) == 0){
              SerialBT.print("ABORT\n");
              break;
            }
          }

          digitalWrite(LaunchPin, HIGH);
          delay(1000);
          Message = "";
          while(strcmp(Message.c_str(), EndStop.c_str()) != 0) {
            BluetoothRead();
            launchMode();
          }
        } else {
          SerialBT.print("ABORT\n");
        }
      }
      break;

    default:
      if(strcmp(Message.c_str(), Calibrate.c_str()) == 0) {
        Serial.println("yes");
        Message = "";
        SerialBT.print("Enter value of known weight in grams: \n");

        while(!SerialBT.available()) {}

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
          CalibrationValue = reading / KnownWeight;
          SerialBT.print("Calibration Value: ");
          SerialBT.print(CalibrationValue);
          SerialBT.print("\n");
        }
      } else if(strcmp(Message.c_str(), SetScale.c_str()) == 0){
        Message = "";
        SerialBT.print("Enter Known Calibration Value: \n");

        while(!SerialBT.available()) {}

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
    if (digitalRead(ContinuityPin) == 1) {
      SerialBT.print("Continuity on charge\n");
      state++;
    }
  } 
}

void BluetoothRead() {
  while(SerialBT.available()) {
    Message = SerialBT.readStringUntil('\n');
    Message.remove(Message.length() - 1, 1);
    Serial.println(Message);
  }
  delay(200);  
}

void MeasureMode() {
  ForceMeasure();
  PressureValue = readPressureSensor(); // Lectura del sensor de presión
  dataTransfer();
  instance = millis() - StartTime;
  dataStore();
}

void launchMode() {
  ForceMeasure();
  PressureValue = readPressureSensor(); // Lectura del sensor de presión
  if(SDSTATE){
    dataStore();
  }
  dataTransfer();
  instance = StartTime - millis();
}

void dataStore() {
  myFile = SD.open("/Test.txt", FILE_APPEND);
  if(myFile){
    myFile.print(instance);
    myFile.print(",");
    myFile.print(ForceValue, 1);
    myFile.print(",");
    myFile.println(PressureValue, 1); // Sensor de presión
    myFile.close();
  } else {
    SerialBT.print("SD ERROR");
    SerialBT.print(";");
    myFile.close();
    SDSTATE = false;
  }
}

void ForceMeasure() {
  if(scale.is_ready()){
    ForceValue = scale.get_units() / 1000;
  }
}

void dataTransfer() {
  SerialBT.print(ForceValue, 1);
  SerialBT.print(",");
  SerialBT.print(PressureValue, 1); // Sensor de presión
  SerialBT.print(",");
  SerialBT.println(instance);
  Serial.print(ForceValue, 1);
  Serial.print(",");
  Serial.print(PressureValue, 1); // Sensor de presión
  Serial.print(",");
  Serial.println(instance);
}

float readPressureSensor() {
  int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
  float voltage = sensorValue * (3.3 / 4095.0); // convertir valor leido a voltaje
  float pressure = (voltage / 3.3) * 100; // 0-100 psi 
  return pressure;
}
