/* Thermostat to control Danfoss Fridge  */
//____________________________________________________________

/*-----( Import needed libraries )-----*/
#include <DS1307RTC.h>
#include <Time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <TimeAlarms.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
//#include <SPI.h>
//#include <SD.h>  //sd card

/*---------------Setup the LCD screen-------------------------------------*/
#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

int n = 1;

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);


/*-----( Declare Constants )-----*/
#define ONE_WIRE_BUS 2 /*-(Connect to Pin 2 )-*/
const int compressorControlPin =  7;      // the number of the transistor pin
const int LED =  13;      // the number of the LED pin

/*-----( Declare objects )-----*/
/* Set up a oneWire instance to communicate with any OneWire device*/
OneWire ourWire(ONE_WIRE_BUS);


/* ------------Tell Dallas Temperature Library to use oneWire Library-------------- */
DallasTemperature sensors(&ourWire);

/*-----( Declare Variables )---------------------------------------------------------------*/
// Assign the addresses of your 1-Wire temp sensors.
// See the tutorial on how to obtain these addresses:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html

DeviceAddress Sensor01 = { 0x28, 0x57, 0xEF, 0x56, 0x05, 0x00, 0x00, 0xE4 }; 
DeviceAddress Sensor02 = { 0x28, 0xA0, 0xA4, 0x56, 0x05, 0x00, 0x00, 0x8A };             //Ambient temp
DeviceAddress Sensor03 = { 0x28, 0x99, 0x11, 0x56, 0x05, 0x00, 0x00, 0x6C };             //Cold plate
DeviceAddress Sensor04 = { 0x28, 0xDB, 0xA1, 0x46, 0x05, 0x00, 0x00, 0x00 };             //Locker
DeviceAddress Sensor05 = { 0x28, 0x27, 0xD4, 0x56, 0x05, 0x00, 0x00, 0x88 };
DeviceAddress Sensor06 = { 0x28, 0x61, 0xDE, 0x56, 0x05, 0x00, 0x00, 0xF2 };

float sensor1=0 ; float sensor2=0 ;float sensor3=0 ;float sensor4=0 ; float sensor5=0 ;float sensor6=0;
float oldSensor1=0 ; float oldSensor2=0 ;float oldSensor3=0 ;float oldSensor4=0 ; float oldSensor5=0 ;float oldSensor6=0;

int dutyCycleArray[7200];  // array to hold 2 hours of record of compressor state
int dutyCycleTotal;  

float OnTemp=5;     //temperature when comressor turns on
float OffTemp=3.5;      //temperature when compressor turns off

// set up variables using the SD utility library functions:
const int chipSelect = 10;


int compressorControlState = LOW;             // the transistor on pin 7 controlling the compressor set to off
int KeepOff=LOW;                              // inhibit flag to keep the compressor off
int LEDState=LOW;


//__________________________________________________________________________________________________________________________
//SET UP         SET UP           SET UP            SET UP
//__________________________________________________________________________________________________________________________
void setup() /*----( SETUP: RUNS ONCE )----*/
{
/*-(start serial port to see results )-*/
delay(100);
Serial.begin(9600);
Serial.println("Serial started...");

pinMode(compressorControlPin, OUTPUT);
pinMode(LED, OUTPUT);
delay(100);

/*-( Start up the DallasTemperature library )-*/
sensors.begin();

// set the resolution to 10 bit (Can be 9 to 12 bits .. lower is faster)
  sensors.setResolution(Sensor01, 10);
  sensors.setResolution(Sensor02, 10);             
  sensors.setResolution(Sensor03, 10);             
  sensors.setResolution(Sensor04, 10);
  sensors.setResolution(Sensor05, 10);
  sensors.setResolution(Sensor06, 10);
  
  //_____________________________
  // set system time to real time from rtc
  setSyncProvider(RTC.get);   // the function to get the time from the RTC and set the ststem time to this
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time"); 
     timeToSerial();
  
 //____________________________
//Set up LCD
lcd.begin (20,4); //  <<-----  LCD is 20 wide x 4  lines
 
lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
lcd.setBacklight(HIGH);
lcd.home (); // go home
lcd.print("LCD screen up");



  
  
 // Alarm.timerRepeat(1, Repeats); 

}/*--(end setup )---*/
//______________________________________________________________________________________________________________________________
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//______________________________________________________________________________________________________________________________





void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{

delay(1000); // wait one second 


sensor1=oldSensor1; sensor2=oldSensor2; sensor3=oldSensor3; sensor4=oldSensor4; sensor5=oldSensor5; sensor6=oldSensor6;   
 // Command all devices on bus to read temperature  
  sensors.requestTemperatures();
  if (sensor1==-127) {sensor1=oldSensor1; }
  if (sensor2==-127) {sensor2=oldSensor2; }
  if (sensor3==-127) {sensor3=oldSensor3; }
  if (sensor4==-127) {sensor4=oldSensor4; }
  if (sensor5==-127) {sensor5=oldSensor5; }
  if (sensor6==-127) {sensor6=oldSensor6; }
  
  
//____________________

//turn on or off the compressor and flash the LED if running
 float tempCheck=sensors.getTempC(Sensor02);

  if (tempCheck > OnTemp) {
    if (KeepOff==LOW){
      compressorControlState = HIGH; 
    }   
    } 
  else if (tempCheck < OffTemp) {
      compressorControlState = LOW;
  }
 
   digitalWrite(compressorControlPin, compressorControlState);
   if (compressorControlState==HIGH){
      LEDState=!LEDState;
      digitalWrite(LED, LEDState);
   }
//________________________
   
  
 // read what comes in from serial and call  the menu function
   if (Serial.available() > 0) {
    int inByte = Serial.read();
    menu(inByte);  
   }
//________________________  

//Write to LCD screen
lcd.setCursor ( 0, 0 );
lcd.print("Amb Temp  =");
lcd.print(sensors.getTempC(Sensor02));

lcd.setCursor ( 0, 1 );
lcd.print("Plate Temp=");
lcd.print(sensors.getTempC(Sensor03));

lcd.setCursor ( 0, 2 );
lcd.print("Lock Temp =");
lcd.print(sensors.getTempC(Sensor04));

lcd.setCursor ( 0, 3 );
lcd.print("Comp 0n   =");
lcd.print(compressorControlState);


//------------------  Defrost ------------------------------
tmElements_t tm;
 if (RTC.read(tm)) {

int hour = tm.Hour;

if (hour==16) {
  lcd.setCursor ( 13, 3 );
  lcd.print("defrost");
  KeepOff=1;
  
   } else {
  lcd.setCursor ( 13, 3 );
  lcd.print("       ");
  KeepOff=0;
   }   
 }
//----------------Send data over bluetooth-------------------
//
Serial.print("Amb=");
Serial.println(sensors.getTempC(Sensor02));

Serial.print("plt=");
Serial.println(sensors.getTempC(Sensor03));

Serial.print("loc=");
Serial.println(sensors.getTempC(Sensor04));

  
  //TEMP____Testing_________________________TEMP______________________________TEMP

 
  
  
  //TEMP______________________________TEMP____________________________TEMP
  
  
  
  
}//--(end main loop )---
//_________________________________________________________________________________________________________________________
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//_________________________________________________________________________________________________________________________


/*-----( Declare User-written Functions )----------------------------------------------------------------------*/


void printToSerial()
{
  // Serial.print("Sensor 01 temperature is:   ");
 // printTemperature(Sensor01);
 // Serial.println();

  Serial.print("Bottom of fridge temperature is:   ");
  printTemperature(Sensor02);
  Serial.println();
 
  Serial.print("Cold plate temperature is:   ");
  printTemperature(Sensor03);
  Serial.println();
   
  Serial.print("Compressor locker ambient temperature is:   ");
  printTemperature(Sensor04);
  Serial.println();
  
  ///Serial.print("Sensor 05 temperature is:   ");
  //printTemperature(Sensor05);
  //Serial.println();
  
  //Serial.print("Sensor 06 temperature is:   ");
 // printTemperature(Sensor06);
 // Serial.println(); 
  Serial.println("********************************");

} //end printToSerial 
//________________________________________________________________________________________________________________


void printTemperature(DeviceAddress deviceAddress)
{

float tempC = sensors.getTempC(deviceAddress);

   if (tempC == -127.00) 
   {
   Serial.print("Error getting temperature  ");
   } 
   else
   {
   Serial.print(tempC);
   Serial.print("C");
   }
}// End printTemperature

//__________________________________________________________________________________________________________________________________

/*      MENU MENU               */
void menu(int inByte)
{
 switch(inByte){
   
   case 'H':
  Serial.println("Help:");
  Serial.println("Press 'S' for settings");
  Serial.println("Press 'L' for live data");
  Serial.println("Press 'T' to display time ");
  Serial.println("Press 'V' to increase turn on temp ");
  Serial.println("Press 'C' to decrease turn on temp ");
  Serial.println("Press 'X' to increase turn off temp ");
  Serial.println("Press 'Z' to decrease turn on temp ");
  Serial.println("Press '1' to keep turned off for 10 seconds ");
  Serial.println("Press '2' to keep turned off for 10 minutes ");
  Serial.println("********************************");
  break;
   
  case 'S':
  Serial.println("settings:");  
  Serial.print("Turn on compressor when temperture above ");
  Serial.println(OnTemp);
  Serial.print("Turn off compressor when temperture below ");
  Serial.println(OffTemp);
  Serial.print("Inhibit state="); 
  Serial.println(KeepOff);
  Serial.print("Compressor control state=");
  Serial.println(compressorControlState);
  Serial.println("********************************");
  break;
  
  case 'V':
  OnTemp=OnTemp+0.1;
  Serial.print("Turn on compressor when temperture reaches ");
  Serial.println(OnTemp);
  Serial.print("Turn off compressor when temperture reaches ");
  Serial.println(OffTemp);  
  Serial.println("********************************");
  break;
  
  case 'C':
  OnTemp=OnTemp-0.1;
  Serial.print("Turn on compressor when temperture reaches ");
  Serial.println(OnTemp);
  Serial.print("Turn off compressor when temperture reaches ");
  Serial.println(OffTemp);  
  Serial.println("********************************");
  break;
  
  case 'X':
  OffTemp=OffTemp+0.1;
  Serial.print("Turn on compressor when temperture reaches ");
  Serial.println(OnTemp);
  Serial.print("Turn off compressor when temperture reaches ");
  Serial.println(OffTemp);  
  Serial.println("********************************");
  break;
  
  case 'Z':
  OffTemp=OffTemp-0.1;
  Serial.print("Turn on compressor when temperture reaches ");
  Serial.println(OnTemp);
  Serial.print("Turn off compressor when temperture reaches ");
  Serial.println(OffTemp);  
  Serial.println("********************************");
  break;
  
  
  
  case 'L':
   Serial.print("Compressor is ");
  if (compressorControlState == LOW)  {
    Serial.println("not running");
  }
  else {
    Serial.println("running"); 
  }
  printToSerial();
  break;
  
  case 'T':
  compressorControlState =!compressorControlState;
  KeepOff=LOW;
  Serial.print("Compressor control state=");
  Serial.println(compressorControlState);
  Serial.print("KeepOff=");
  Serial.println(KeepOff);
  timeToSerial();

  break;
  
 case '1':
    compressorControlState=LOW;
    KeepOff=HIGH;
    Serial.println("Delay 10 sec");
    Serial.print("KeepOff=");
    Serial.print(KeepOff);
    Alarm.timerOnce(10, OnceOnly);
  break; 
  
  case '2':
    compressorControlState=LOW;
    KeepOff=HIGH;
    Serial.println("Delay 600 sec");
    Serial.print("KeepOff=");
    Serial.print(KeepOff);
    Alarm.timerOnce(600, OnceOnly);
  break; 
  
 }
}//end menu


//______________________________________________________________________________________________________________________________________


void timeToSerial(){
   tmElements_t tm;

  if (RTC.read(tm)) {
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
    Serial.println("********************************");
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }

  }
}
//__________________________________________________________________________________________________

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
//_______________________________________________________________________________________________________


void OnceOnly(){
  Serial.println("This timer only triggers once");
  KeepOff=LOW;
       
}
//_______________________________________________________________________________________________________


//*********( THE END )***********
