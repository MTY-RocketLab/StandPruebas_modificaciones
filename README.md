# StandPruebas_modificaciones
## Naming the Text file 
Another change was to make a function in the data recording part of the SD to ask for the name of the text file this is to separate text files of different launches also the idea to make a button or a certain action in the blutooth terminal could be a option to reset the text file and commence a new text file.

Los siguientes cambios son para el nombrar a el texto aunque faltan los cambios para pedir si comensar un nuevo text.file (se me dijo que se podia usar el safty pin para poder hacer eso pero la verdad aun no se nada de eso)
```c++
static String fileName = "";  // Store file name static so after first change it wont be renamed again and again and for the function to be called multiple times

//using this and the following changes in the function void datastore

void dataStore() {
  if (fileName == "") {  // Ask for file name only once due to the if and the static string
    SerialBT.print("Enter file name: ");
    while (!SerialBT.available()) {}  /* When a input is added a \n will be placed which will set a limit to the inputed "read" code
    as seen in the next line of code*/
    fileName = SerialBT.readStringUntil('\n'); 
    fileName.trim(); // I dont like spaces in names
    fileName = "/" + fileName + ".txt";  // To format the file as a file path
  }

  myFile = SD.open(fileName, FILE_APPEND); //part of code that just writes down info (SHOULDNT BE TOUCHED BY ME SINCE I DONT FULLY UNDERSTAND IT)
  if (myFile) {
    String data = "Time instance" + String(instance) + "," + " Force value" + String(ForceValue, 1) + "\n";
    myFile.print(data);
    
/*  The snipet in coment should be more effcient as a writer, since it writes in binaries but due to knowledge constraints 
on bytes the first part in the argument makes the arduino string into a c++ string since it's the only one that works 
for the .write and the min(31, data.lenght())) basically ensures that if more than 31 bytes are writen for only the lenght of 31 bytes to be accepted,
however, I am not sure if every character means 1 byte therefore I am hesitant in using this however it should be more optimised*/
    //myFile.write(data.c_str(), min(31, data.length()));  // Write max 31 bytes (Normally the last byte isnt a full byte)
    myFile.close();
  } else {
    SerialBT.print("SD ERROR;\n");
    SDSTATE = false;
  }
}
```
## Adding the pressure sensor
Currently there isnt a dirrect presure sensor which was a change that has been set to be made. 

Therefore we added certain functions to incorporate the sensor however a formula to get the exact PSI of the rocket is yet to be made.

## Livestyle changes to the txt file
The main changes in this were insted of giving a array based on 
3 values like

>[12,33,44] (*tiempo,fuerza,presion*)

we tried to transform it into a more comprehensive file following this format 

> Time:xx, Force:xx Presure:xx (all values would be a string)


