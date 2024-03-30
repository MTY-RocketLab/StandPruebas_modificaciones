
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


BluetoothSerial SerialBT;

String GroundStation_MAC = "";

HX711 scale;
File myFile;

uint8_t LaunchPin = 14; //Pin for pyrochannel
uint8_t ContinuityPin = 12;
uint8_t Safe_Pin = 15;                
const uint8_t LOADCELL_DOUT_PIN = 32;  //Data pin for hx711
const uint8_t LOADCELL_SCK_PIN = 33;   //sck pin for hx711


float ForceValue;
uint32_t StartTime = millis();
uint32_t instance;
int32_t CalibrationValue;
uint32_t KnownWeight;


char incomingChar;
static String Message = "";
String Calibrate = "calibrate";
String Launch = "launch";
String SetScale = "set";
String Measure = "measure";
String EndStop = "stop";


bool SDSTATE;
bool SD_Activated;
uint8_t state = 0;

//SPIClass vspi = SPIClass(VSPI);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  SerialBT.begin("Test Stand");
  //vspi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

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
  
  SerialBT.print("ready to go \n 'calibrate' to calibrate scale \n 'set' to use known calibration value \n 'measure' to test without launching \n 'launch' to launch \n stop while running to stop \n CASE SENSITIVE \n");
  
//  timer = timerBegin(0, 800, true);             // timer 0, prescalar: 80, UP counting
//  timerAttachInterrupt(timer, &loop, true);   // Attach interrupt
//  timerAlarmWrite(timer, 100000, true);     // Match value= 1000000 for 1 sec. delay.
//  timerAlarmEnable(timer);  
}

void loop(){
  Message = "";
  BluetoothRead();
  delay(50);

  switch(state){

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
            //measure
          
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
        Serial.println("yes");
        Message = "";
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
      Message = "";
      SerialBT.print("Enter Known Calibration Value: \n");
      

      while(!SerialBT.available()){

      }
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

void BluetoothRead(){
  while(SerialBT.available()) {
    Message = SerialBT.readStringUntil('\n');
    Message.remove(Message.length() - 1,1);
    //Serial.println(Message);
  }
}

void MeasureMode(){
  ForceMeasure();
  dataTransfer();
  instance = millis() - StartTime;
  dataStore();
  
}

void launchMode(){
  ForceMeasure();
    if(SDSTATE){
      dataStore();
    }
    dataTransfer();
    instance = StartTime - millis();
  }


void dataStore(){
  //SD card must me < 32 gb and formated FAT32
  
  myFile = SD.open("/Test.txt", FILE_APPEND);
  if(myFile){
    myFile.print(instance);
    myFile.print(",");
    myFile.println(ForceValue,1);
    myFile.close();
  }
  else{
    SerialBT.print("SD ERROR");
    SerialBT.print(";");
    myFile.close();
    SDSTATE = false;
  }
}

void ForceMeasure(){
  if(scale.is_ready()){
    ForceValue = scale.get_units()/1000;
  }
}

void dataTransfer(){
  SerialBT.print(ForceValue, 1);
  SerialBT.print(",");
  SerialBT.println(instance);
  Serial.print(ForceValue, 1);
  Serial.print(",");
  Serial.println(instance);
}
