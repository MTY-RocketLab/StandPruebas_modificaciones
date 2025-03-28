
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
const uint8_t PRESSURE_SENSOR_PIN = 34; //falta definir pin
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
//FROM PAST PRESURE THE DELAY USED TO BE 100 ms Current is 50
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
//ver si mover funciones que ponen nombre a el text file ponerlas en otro lado
void MeasureMode(){
  ForceMeasure();
  PressureValue = readPressureSensor(); // Lectura del sensor de presión
  dataTransfer();
  instance = millis() - StartTime;
  //segun yo es al reves  ]]]]]]]]]]]]]]]]]]]]]]]]]]] osea starttime-milis porq es final-inicial
  dataStore();
  
}

void launchMode(){
  ForceMeasure();
  PressureValue = readPressureSensor(); // Lectura del sensor de presión
    if(SDSTATE){
      dataStore();
    }
    dataTransfer();
    instance = StartTime - millis();
  }


void dataStore(){
  //SD card must me < 32 gb and formated FAT32

  if (digitalRead(FILE_RESET_Button)==true){// el pin corresponde al boton (ver si hay una forma de hacerlo sin tener que lanzar cohete)
  fileName="";
  }
  
  
  if (fileName == "") {  // Ask for file name only once due to the if and the static string
    SerialBT.print("Enter file name: ");
    while (!SerialBT.available()) {}  /* When a input is added a \n will be placed which will set a limit to the inputed "read" code
    as seen in the next line of code*/
    fileName = SerialBT.readStringUntil('\n'); 
    fileName.trim(); // I dont like spaces in names
    fileName = "/" + fileName + ".txt";  // To format the file as a file path
  }

  myFile = SD.open(fileName, FILE_APPEND); //part of code that just writes down info (SHOULDNT BE TOUCHED BY ME SINCE I DONT FULLY UNDERSTAND IT)
  if (firstTIMEcheck==true) {
    Serial.println("First thing printed");
    SerialBT.println("First thing printed");
    myFile.print("Force,Pressure,Time");
    firstTIMEcheck= !firstTIMEcheck;
  }
  if (myFile) {
    
    
    String data = String(ForceValue)+","+String(PressureValue)+","+String(instance);
    myFile.print(data);
  
    //CHANGE TO A MATRIX WITH A TITLE AS A COLUMN THAT STATES WHAT NAME OF THE COLUMN the colum can be made with a static value as the previous
    
/*  The snipet in coment should be more effcient as a writer, since it writes in binaries but due to knowledge constraints 
on bytes the first part in the argument makes the arduino String into a c++ string since it's the only one that works 
for the .write and the min(31, data.lenght())) basically ensures that if more than 31 bytes are writen for only the lenght of 31 bytes to be accepted,
however, I am not sure if every character means 1 byte therefore I am hesitant in using this however it should be more optimised*/
    //myFile.write(data.c_str(), min(31, data.length()));  // Write max 31 bytes (Normally the last byte isnt a full byte)
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



//cambiar para mejor estructura (agregar el string del fuerza:, Tiempo:, Presion:)
void dataTransfer(){
  
  SerialBT.print(String(Contador) + ". Fuerza: " + String(ForceValue) + ", Pression: "+ String(PressureValue) + "tiempo: "+ String(instance));
  Serial.print(String(Contador) + ". Fuerza: " + String(ForceValue) + ", Pression: "+ String(PressureValue) + "tiempo: "+ String(instance));
  Contador++;
}
float readPressureSensor() {
  int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
  float voltage = sensorValue * (3.3 / 4095.0); // convertir valor leido a voltaje
  float pressure = (voltage / 3.3) * 100; // 0-100 psi 
  return pressure;
}

//FALTA DEFINIR PRESURE COMO GLOBAL