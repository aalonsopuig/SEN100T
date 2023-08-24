/*
  MultiSense Master
  By Alejandro Alonso
  www.automaconm.blogspot.com
  August 2015
  
  
  

  Credits:
  Screen control is based on code created 15 April 2013 by Scott Fitzgerald   http://www.arduino.cc/en/Tutorial/TFTDisplayText
  Temperature sensors (-55 to 125ºC): http://milesburton.com/Dallas_Temperature_Control_Library
 */


//INCLUDES
#include <TFT.h>     // Arduino LCD library http://www.arduino.cc/en/Reference/TFTLibrary
#include <SPI.h>     // Default library
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>



//PIN DEFINITIONS
#define SDSelect    4   //Pin selection of SD card
#define Pin_LCD_rst 8   //Pin used by TFT Screen (see http://www.arduino.cc/en/Main/GTFT) 
#define Pin_LCD_dc  9   //Pin used by TFT Screen (see http://www.arduino.cc/en/Main/GTFT)
#define Pin_LCD_cs 10   //Pin used by TFT Screen (see http://www.arduino.cc/en/Main/GTFT)
#define ONE_WIRE_BUS 2  // Temp Data wire is plugged into pin 2 on the Arduino



//OTHER PREPROCESOR
#define FirstFileNum 100           //First number of datalog files
#define LastFileNum 999            //Last number of datalog files. 



//INSTANCES CREATION
TFT     TFTscreen = TFT(Pin_LCD_cs, Pin_LCD_dc, Pin_LCD_rst); // create an instance of the TFT library
File dataFile;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.



//VARIABLES DEFINITION
char          ValuePrintout[5];         // char array to print to the TFT screen
String        ValStr="";             //Used for conversion of integers to string
unsigned long SREAD_CHK_TIME=1000; //Sensors. Time in milliseconds between reading values
unsigned long SRead_time_init;  //sensor measurement timer
unsigned long SRead_time_lapse; //sensor measurement timer
char          LogFileName[12];  //Logs File name
int           Filenum;
int           SensorVal [6];  // sensors values
int           SensorValPrev [6];  // sensors previous values
byte          SensorsNum;  //Number of sensors conected
int           GraphHeight, GraphWidth; //area of graph
int           GraphX0, GraphY0;        //Absolute origin of graph
int           GraphXMax, GraphYMax;        //Max value of X,Y (Absolute)
byte          n;
int           MinY=-10, MaxY=100;  //Min and Max values of Y
int           MinX=0, MaxX=100;    //Min and Max values of X
int           ValX=0;              //Value of X during graph creation



void setup() {

  //TFT Screen
  TFTscreen.begin();   // Screen:
  TFTscreen.background(0, 0, 0);   // clear the screen with a black background

  //SD CARD SETUP
  SD.begin(SDSelect);
  //File name format: LGxxx.txt, where xxx is a consecutive number.
  for (Filenum = FirstFileNum; Filenum <= LastFileNum; Filenum++) {
    //Decide wich will be the file name
    sprintf(LogFileName, "LG%03d.txt", Filenum);
    if(!SD.exists(LogFileName)) {
      Filenum=LastFileNum+1;  //To go out of FOR
    }
  }
  dataFile = SD.open(LogFileName, FILE_WRITE);
  if (dataFile) {
    dataFile.println("Ms, S1, S2, S3, S4, S5, S6");
    dataFile.close();
  }

  //SERIAL PORT SETUP
  Serial.begin (9600);

  //Dallas Temp ssensor
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  SensorsNum=sensors.getDeviceCount(); //Identify number of sensosrs connected
  
  //Other
  SRead_time_init = millis();
  
  //Graph
  GraphX0=21;
  GraphY0=TFTscreen.height()-13;
  GraphXMax=TFTscreen.width()-35;
  GraphYMax=5;
  GraphHeight=abs(GraphYMax-GraphY0);
  GraphWidth=abs(GraphXMax-GraphX0);
  TFTscreen.setTextSize(1);  // set the font size
  
  //Graph frame
  SetColor(0);
  TFTscreen.line (GraphXMax+1,GraphY0,GraphXMax+1,GraphYMax);  //Line between graph and data
  TFTscreen.text("TEMP:", GraphXMax+4, 5);  
  TFTscreen.line (GraphXMax+1,75,TFTscreen.width(),75);      //Line between sensor data and other data
  TFTscreen.text("FILE:", GraphXMax+4, 80);  
  SetColor(5);
  TFTscreen.text(LogFileName, GraphXMax+4, 90);  // Log file name

  //X Axis
  SetColor(0);
  TFTscreen.line (GraphX0,GraphY0,GraphXMax,GraphY0);  //X axis
  for (n=0;n<6;n++) {
    TFTscreen.line (GraphX0+n*GraphWidth/5,GraphY0,GraphX0+n*GraphWidth/5,GraphY0+3);
    FormatData(map(n,0,5,MinX,MaxX));
    TFTscreen.text(ValuePrintout, GraphX0+n*GraphWidth/5-13, GraphY0+4);  // Write label
  }
  
  //Y Axis
  TFTscreen.line (GraphX0,GraphY0,GraphX0,GraphYMax);  //Y axis
  for (n=0;n<12;n++) {
    TFTscreen.line (GraphX0-3,GraphY0-n*GraphHeight/11,GraphX0,GraphY0-n*GraphHeight/11);
    FormatData(map(n,0,11,MinY,MaxY));
    TFTscreen.text(ValuePrintout, 0, GraphY0-n*GraphHeight/11-3);  // Write label
  }


  //Print labels
  SetColor(0);   // set the font color
  TFTscreen.text("1:", GraphXMax+4, 15);  // Write label
  SetColor(1);   // set the font color
  TFTscreen.text("2:", GraphXMax+4, 25);  // Write label
  SetColor(2);   // set the font color
  TFTscreen.text("3:", GraphXMax+4, 35);  // Write label
  SetColor(3);   // set the font color
  TFTscreen.text("4:", GraphXMax+4, 45);  // Write label
  SetColor(4);   // set the font color
  TFTscreen.text("5:", GraphXMax+4, 55);  // Write label
  SetColor(5);   // set the font color
  TFTscreen.text("6:", GraphXMax+4, 65);  // Write label
}

void loop() {

  if (millis() < SRead_time_init) SRead_time_init = millis(); // Verify overflow of millis
  else {
    SRead_time_lapse = millis() - SRead_time_init;
    if (SRead_time_lapse > SREAD_CHK_TIME) {
      SRead_time_init = millis();

      //SENSORS READINGS  
     sensors.requestTemperatures(); // Send the command to get temperatures
     for (n=0;n<SensorsNum;n++) {
        SensorVal[n]=sensors.getTempCByIndex(n);
        //Sensor limits
        if (SensorVal[n]<-55) SensorVal[n]=-55;
        if (SensorVal[n]>125) SensorVal[n]=125;
     }

 
      ShowTempValues();  //Show the Temp Values at the right of screen
      DrawTempGraph();   //Draw the line graphs

      //LOG FILE DATA SAVING
      String dataString = String(millis());
     for (n=0;n<SensorsNum;n++) {
        dataString += ",";
        if (SensorVal[n]==-55) 
          dataString +='NC';      //A sensor has been disconected
        else                   
          dataString += String(SensorVal[n]);
      }
    
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      dataFile = SD.open(LogFileName, FILE_WRITE);
    
      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
      }
      //if the file isn't open, pop up an error:
      else {
        SetColor(7);
        TFTscreen.text(LogFileName, GraphXMax+4, 90);  // Log file name
        SetColor(5);
        TFTscreen.text("No SD", GraphXMax+4, 90);  
      }
    // print to the serial port too:
    Serial.println(dataString);

     for (n=0;n<SensorsNum;n++) {
      SensorValPrev[n]=SensorVal[n];
      }
      ValX++;  
      if (ValX>MaxX) ValX=MinX;  //reached end of X. Restart again
    }
  }
}

void FormatData(int NumData)
//Convert a number to an array of chars (ValuePrintout[]), aligned to the right 
//in the three first positions. Example: -1 -> " -1  "
{
    ValStr=String(NumData);
    ValStr.toCharArray(ValuePrintout, 5);  // convert the reading to a char array
    //Align right
    if (ValuePrintout[1]==NULL) {
      ValuePrintout[2]=NULL;
      ValuePrintout[1]=ValuePrintout[0];
      ValuePrintout[0]=' ';
    }
    if (ValuePrintout[2]==NULL) {
      ValuePrintout[2]=ValuePrintout[1];
      ValuePrintout[1]=ValuePrintout[0];
      ValuePrintout[0]=' ';
    }
}

void SetColor(byte color)
//Set a color value between a set of predefined colors
{
  switch (color) {
    case 0:  
      TFTscreen.stroke(255,255,255);   // white
      break;
    case 1:  
      TFTscreen.stroke(255, 51,255);   // magenta
      break;
    case 2:  
      TFTscreen.stroke(0, 255, 0);   // green
      break;
    case 3:  
      TFTscreen.stroke(255,204,0);   // orange
      break;
    case 4:  
      TFTscreen.stroke(255, 255, 0);   // yellow
      break;
    case 5:  
      TFTscreen.stroke(0, 255, 255);   // light blue
      break;
    case 6:  
      TFTscreen.stroke(0, 0, 255);   // dark blue
      break;
    case 7:  
      TFTscreen.stroke(0, 0, 0);   // black
      break;
    case 8:  
      TFTscreen.stroke(255, 0, 0);   // red
      break;
  }
}
void ShowTempValues(){
  //Show the temperature values at the right of the screen
  
//  TFTscreen.background(0, 0, 0);   // clear the screen with a black background


  for (byte index=0;index<SensorsNum;index++) {
  
    //Delete previous value if different
    if (SensorValPrev[index]!=SensorVal[index]) {
      FormatData(SensorValPrev[index]);
      SetColor(7);   // set the font color to black (delete)
      TFTscreen.text(ValuePrintout, GraphXMax+15, index*10+15);   // print the sensor value again to delete
    }

    SetColor(index); //Set color
    
    //Print new value 
    if (SensorVal[index]!=-55) { //No sensor has been disconected
      FormatData(SensorVal[index]);
      TFTscreen.text(ValuePrintout, GraphXMax+15, index*10+15);   // print the sensor value
    }    
  }
}

void DrawTempGraph()
//Draw the temperatures graph
{
  if (ValX>1)
  {
    //Clear area where we are going to draw graph
    SetColor(7);   // set color to black
    TFTscreen.fill(0,0,0); //set the fill color to black
    TFTscreen.rect(map(ValX-1,MinX,MaxX,GraphX0,GraphXMax), GraphYMax,
         map(ValX,MinX,MaxX,GraphX0,GraphXMax)-map(ValX-1,MinX,MaxX,GraphX0,GraphXMax), abs(GraphYMax-GraphY0));

    //Cursor Line
    if (ValX<MaxX) {
      SetColor(6);   // set color to blue
      TFTscreen.line(map(ValX+1,MinX,MaxX,GraphX0,GraphXMax), GraphY0,
                     map(ValX+1,MinX,MaxX,GraphX0,GraphXMax), GraphYMax);
    }
         
    //Sensors graph
    for (byte index=0;index<SensorsNum;index++) {
        SetColor(index);   // set color 
        if ((SensorVal[index]<=MaxY)&&(SensorVal[index]>=MinY)&&(SensorValPrev[index]<=MaxY)&&(SensorValPrev[index]>=MinY)) {
          TFTscreen.line(map(ValX-1,MinX,MaxX,GraphX0,GraphXMax), map(SensorValPrev[index],MinY,MaxY,GraphY0,GraphYMax),
                         map(ValX  ,MinX,MaxX,GraphX0,GraphXMax), map(SensorVal[index]    ,MinY,MaxY,GraphY0,GraphYMax));
        }
    }
  }    
}



