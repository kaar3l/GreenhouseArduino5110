#include <OneWire.h>
#include <LCD5110_Basic.h>
#include <math.h>
LCD5110 myGLCD(2,3,4,6,5);
extern uint8_t SmallFont[];

int cwPin = 8; //Kinni ,out
int ccwPin = 9; //Lahti ,out
const int countPin = 11; //RevCounter ,in
const int homeSwitchPin = 12; //Homeswitchi pin , in

int buttonPushCounter = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
int countDownTimer=0;

int homeSwitchState = 0;     // variable to store the read value
float lowTemp = 26; //Temperatuur millest hakkab uks ennast timmima
float highTemp = 32; //Temperatuur millest ylespoole on 100% lahti uks
float lastDoorState=0; //Ukse hetke asukoht
float maxDoorState=750; //Maksimum ukse asukoht 610

float tempC=0;

OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)

void setup(void) {
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  
  pinMode(cwPin, OUTPUT);
  pinMode(ccwPin, OUTPUT);
  pinMode(countPin, INPUT);
  pinMode(homeSwitchPin, INPUT);
  //Turn pins to low
  digitalWrite(cwPin, LOW);
  digitalWrite(ccwPin, LOW);
  
  Serial.begin(9600);
  
  homeSwitchState=digitalRead(homeSwitchPin);   // read the input pin
  Serial.print(homeSwitchState); //Mis asendis on ukse lüliti
  if(homeSwitchState==0){ //Uks on lahti, seega tuleks kinni panna
        myGLCD.clrScr();
        myGLCD.print("SULGEN UST", CENTER, 24);
        Serial.println("Panen ukse kinni");
    digitalWrite(cwPin, HIGH); //Mootor kinni minema
    while(homeSwitchState < 1){ //Vaatame kas uks tuleb kinni:
      homeSwitchState=digitalRead(homeSwitchPin);   // Vaatame pinni, millal saab ukse kinni
                Serial.println("Ootan ukse lülitit");
                delay(10);
    }
    digitalWrite(cwPin, LOW); //Mootor välja lülitada
    Serial.println("Uks kinni, mootor välja lülitada");
    lastDoorState=0;
        myGLCD.clrScr();
        myGLCD.print("UKS KINNI", CENTER, 24);
        delay(1000);
  }
}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  char celsiuschar[10];
  char tempMinchar[10];
  char tempMaxchar[10];
  char lastDoorStateChar[10];
  char nextDoorStateChar[10];
  char maxDoorStateChar[10];
  char doorRotCounter[10];
  char countDownTimerChar[10];


  
  if ( !ds.search(addr)) {
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  
  dtostrf(celsius, 3, 2, celsiuschar);
  dtostrf(lowTemp, 3, 2, tempMinchar);
  dtostrf(highTemp, 3, 2, tempMaxchar);

  
  myGLCD.clrScr();
  myGLCD.print("TEMP:", LEFT, 0);
  myGLCD.print(celsiuschar, RIGHT, 0);
  
  myGLCD.print("MINTEMP:", LEFT, 8);
  myGLCD.print(tempMinchar, RIGHT, 8);
  
  myGLCD.print("MAXTEMP:", LEFT, 16);
  myGLCD.print(tempMaxchar, RIGHT, 16);
  delay (1000);
  
  tempC=celsius;
//  tempC=25;

  
    //TEMPERATUURI PÕHJAL OTSUSTAMA MIS TOIMUMA HAKKAB
  
  //TEMPERATUUR MADAL PANEME KINNI:
  if(tempC<=lowTemp){
    Serial.println("UKS KINNI PANNA(TEMP MADAL)"); //Tuleks panna uks kinni
        myGLCD.print("              ", CENTER, 40);
        myGLCD.print("TAISKINNI", CENTER, 40);
        Serial.print("HomeSwitchState:");
        Serial.println(digitalRead(homeSwitchPin));
        Serial.print("RotSwitch:");
        Serial.println(digitalRead(countPin));
    if(digitalRead(homeSwitchPin)==0){
                myGLCD.print("  ->KINNI  ", CENTER, 40);
                delay(1000);
      digitalWrite(cwPin, HIGH); //Mootor kinni minema
      while(digitalRead(homeSwitchPin) < 1){ //Vaatame kas uks tuleb kinni:
        homeSwitchState=digitalRead(homeSwitchPin);   // Vaatame pinni, millal saab ukse kinni
                        Serial.println("Yks peaks sulguma");
                        //COUNTING TO DISPLAY  
                            myGLCD.print("HOMESWITCH", CENTER, 36);
                        ///COUNTING TO DISPLAY  
      }
      digitalWrite(cwPin, LOW); //Mootor välja lülitada
      lastDoorState=0;
    }
  }
  
  //TEMPERATUUR KÕRGE TEEME LAHTI:
  else if(tempC>=highTemp){
        myGLCD.print("              ", CENTER, 40);
        myGLCD.print("TAISLAHTI", CENTER, 40);
    Serial.println("UKS TÄITSA LAHTI TEHA(TEMP KÕRGE)");
  
  
  
        delay(2000);
          if(lastDoorState!=maxDoorState){
        digitalWrite(ccwPin, HIGH); //Mootor lahti minema
        while(buttonPushCounter<maxDoorState-lastDoorState){ //Nii kaua kuni oleme 0 juures tagasi
              buttonState=digitalRead(countPin);
                if (buttonState != lastButtonState){ //Countimine
            buttonPushCounter++;
          }
          lastButtonState = buttonState;
//COUNTING TO DISPLAY  
              dtostrf(buttonPushCounter, 3, 0, doorRotCounter);
              myGLCD.print(doorRotCounter, CENTER, 36);
///COUNTING TO DISPLAY  
          Serial.print(buttonPushCounter);
          Serial.print(" -> ");
          Serial.println(maxDoorState-lastDoorState);
      }
          lastDoorState=maxDoorState;
      digitalWrite(ccwPin, LOW); //Mootor offi
          }
  
  
  }
  
  //TEMPERATUUR JÄÄB VAHEPEALE:
  else{
    float nextDoorState=round(((tempC-lowTemp)/(highTemp-lowTemp))*maxDoorState);
    float doorDifference=nextDoorState-lastDoorState;
      Serial.println(" ");
    Serial.print("LastDoorState: ");
    Serial.print(lastDoorState);
    Serial.println(" ");
    Serial.print("NextDoorState: ");
    Serial.print(nextDoorState);
        Serial.println(" ");
    Serial.print("DoorDifference: ");
    Serial.print(doorDifference);
        Serial.println(" ");
//        myGLCD.print("->VAHEPEAL", CENTER, 40);


    //Ust on vaja kinnipoole panna:
    if (doorDifference<0){ //Uks kinnipoole
      Serial.println("UST ON VAJA KINNIPOOLE PANNA");
          myGLCD.print("KINNIPOOLE", CENTER, 40);
      delay(2000);
      digitalWrite(cwPin, HIGH); //Mootor kinni minema
      while(buttonPushCounter<abs(doorDifference)){ //Nii kaua kuni oleme 0 juures tagasi
            buttonState=digitalRead(countPin);
        if (buttonState != lastButtonState){ //Countimine
          buttonPushCounter++;
        }
        lastButtonState = buttonState;
//COUNTING TO DISPLAY  
            dtostrf(buttonPushCounter, 3, 0, doorRotCounter);
            myGLCD.print(doorRotCounter, CENTER, 36);
///COUNTING TO DISPLAY    
        Serial.print(buttonPushCounter);
        Serial.print(" -> ");
        Serial.println(doorDifference);
      }
          lastDoorState=nextDoorState;
      digitalWrite(cwPin, LOW); //Mootor offi
    }
    //Ust on lahtipoole teha:
    else if (doorDifference>0){ //Uks lahtipoole
      Serial.println("UST ON VAJA LAHTIPOOLE TEHA");
          myGLCD.print("LAHTIPOOLE", CENTER, 40);
      delay(2000);
      digitalWrite(ccwPin, HIGH); //Mootor lahti minema
      while(buttonPushCounter<doorDifference){ //Nii kaua kuni oleme 0 juures tagasi
            buttonState=digitalRead(countPin);
        if (buttonState != lastButtonState){ //Countimine
          buttonPushCounter++;
        }
        lastButtonState = buttonState;
//COUNTING TO DISPLAY  
            dtostrf(buttonPushCounter, 3, 0, doorRotCounter);
            myGLCD.print(doorRotCounter, CENTER, 36);
///COUNTING TO DISPLAY  
        Serial.print(buttonPushCounter);
        Serial.print(" -> ");
        Serial.println(doorDifference);
      }
          lastDoorState=nextDoorState;
      digitalWrite(ccwPin, LOW); //Mootor offi
    }
    //Uks on vaja samaks jätta
    else{
      //Ära tee midagi
      Serial.println("UST POLE VAJA LIIGUTADA");
                myGLCD.print("EI LIIGUTA", CENTER, 40);
      delay(2000);
    }
  }
  buttonState = digitalRead(countPin); //Loeme herkoni asendit
  //Turn pins off
  digitalWrite(cwPin, LOW);
  digitalWrite(ccwPin, LOW);
  
  dtostrf(lastDoorState, 3, 0, lastDoorStateChar);
  dtostrf(maxDoorState, 3, 0, maxDoorStateChar);
 
  myGLCD.print("                     ", CENTER, 36);
  myGLCD.print(lastDoorStateChar, LEFT, 36);
  myGLCD.print("/", CENTER, 36);
  myGLCD.print(maxDoorStateChar, RIGHT, 36);
  delay(2000);

  buttonPushCounter=0;
  lastButtonState=0;
  Serial.println("DONE");
  myGLCD.print("               ", CENTER, 40);
  myGLCD.print("TEHTUD", CENTER, 40);
  delay(2000);
  myGLCD.print("               ", CENTER, 40);
  myGLCD.print("OOTAN", CENTER, 40);

///////////////////////////////////////////////////////
//Countdown timer
  myGLCD.print("               ", CENTER, 25);
//Timer=600*1000ms=600sec=10min
  int countDownTimer=600;
  while(countDownTimer>1){
  countDownTimer--;
  myGLCD.print("               ", CENTER, 25);
  dtostrf(countDownTimer, 3, 0, countDownTimerChar);
  myGLCD.print(countDownTimerChar, CENTER, 25);
  delay(1000);
  }
}
//////////////////////////////////////////////////////