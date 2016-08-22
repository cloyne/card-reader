
// Counter
// counts 1Hz pulses, prints to Serial

#include <avr/io.h>
#define splits 2 // number of splits! -- if any larger than 2, memory issues ... 
#define check_bits 15
#define max_times 5 // number of things to add up


#define RED 8
#define GREEN 7 // CHANGED! FROM 5, 6  
#define DOOR 3
#define BLINKTIME 10000

#include "sha256.h"

// SD card
#include <SPI.h> 
#include <SD.h>

// SHA-256
uint8_t *hash;

String final = "";

// Setup for card reading
byte data[96*splits] = {0}; // extra bits is for filling in the end
volatile int n = 0;
volatile byte old = 0;
volatile byte new_ = 0;
char data_bits[45];
boolean started = false;
int try_n = 0;

int checklen = check_bits*splits;
boolean existsSignal[check_bits*splits] = {false};

volatile int times = 0;
volatile boolean isOn = false;

void setup() {
  pinMode(RED, OUTPUT); // red LED
  pinMode(GREEN, OUTPUT); // green LED / FOR SOME REASON IF I PUT IT ON IT JUST OSCILLATES , BUT IF IT'S OFF IT'S NORMALISH
  pinMode(DOOR, OUTPUT); // green LED
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(DOOR, LOW);

  // Serial!!
  // Serial.begin(9600);
   Serial.println("\nstart program");   


  if (!SD.begin(10)) {
      Serial.println("SD failure!!");
      return;
      
    } 
    Serial.println("SD Success!!");
    
   // Timer0 is a Counter is Pin4
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS00) |  _BV(CS01) | _BV(CS02);
    OCR0A = 255; //max overflow number

   // Timer1 is the 2.5 kilohertz interrupt signal
    TCCR1A =  _BV(COM1A0) | _BV(COM1B1) | _BV(WGM11) | _BV(WGM10);
    TCCR1B = _BV(WGM13)  |_BV(WGM12) |_BV(WGM11) | _BV(WGM10) |  _BV(CS10) | _BV(CS11) ;
    OCR1A = (100 / splits) - 1; // 1 / (0.4ms) = 2.5 kHz = (16 MHz / 64) / 100
    OCR1B = (100 / splits) - 1;// (100 / splits) - 1;

    


    
  //start recording pinIn counts
   TIMSK1 |= _BV(OCIE1A);   
}



ISR (TIMER1_COMPA_vect) { 
  old = new_; new_ = TCNT0;
  int diff = ((byte) (new_ - old)); // diff in counter
  if (!isOn) {
    if (n >= checklen) {
      n = 0;
    }
    existsSignal[n] = ((diff >= (5 / splits)) && (diff <= (6 / splits) + 5)); // if the number of pulses is within 'read range'
    /*if (((diff >= (5 / splits)) && (diff <= (6 / splits) + 5))) {
      Serial.println("Received _something_.");
    }
    */

    /*
    if (diff != 0) {
      Serial.println(diff);
    }  else {
      Serial.print(0);
    }
    */

    
    n++;
    
    // isOn remains false if any of the checklen codes before fails
    isOn = true; 
    for (int i = 0; i < checklen; i++) {
      if (existsSignal[i] == false) {
         isOn = false;
      }
    }
    if (isOn) {
      started = true;
      n = 0;
    }
  } else {
    if (started) {
      started = false;
      return;
    }
    if (n >= 96*splits) {
      n = 0;
      times++;
    }
    if (times >= max_times) {
      TIMSK1 &= ~(_BV(OCIE1A));
      digitalWrite(RED, HIGH);
      if (!decoder()) {
        digitalWrite(RED, LOW);
         denyAccess();
      } else {
      checkAccess(); // writes final string
      
      digitalWrite(RED, LOW);
      if (checkString()) { // checks whether string is on hash-table
        digitalWrite(DOOR, HIGH);
        digitalWrite(GREEN, HIGH);
        grantAccess();
        digitalWrite(GREEN, LOW);
        digitalWrite(DOOR, LOW);
      } else {
        denyAccess();
      }
      }
      TIMSK1 |= _BV(OCIE1A);; // resets timer 1 interrupt
    } else {
    data[(n) % (96 * splits)] += diff;
    n++;
    }
  } 
}

boolean decoder() {
  // received data is printed out.
  //Serial.println(try_n);
  //try_n++;
  Serial.println("received data");

  /*
  Serial.print("data = [");
  for (int i = 0; i < 96*splits; i++) {Serial.print(data[i]); Serial.print(", ");}
  Serial.println();
  Serial.println("]");
  */
  //Serial.println("phase_checker(data)");

    boolean bits[96] = {false};
    {
      byte phases[96][splits] = {0};
      float average = 0;
      
      for (int i = 0; i < 96; i++) {
        for (int j = 0; j < splits; j++) {
          average += data[i*splits + j];
          for (int k = 0; k < splits; k++) {
           phases[i][j] += data[(i*splits + j + k) % (96*splits)]; 
          }
        }
      }
      average /= 96;
      
      // computes the best phase to use!
      float best_var = 0;
      int best_phase = 0; 
      for (int i = 0; i < splits; i++) {
        float var = 0;
        for (int j = 0; j < 96; j++) {
          var += (phases[j][i] - average) * (phases[j][i] - average);
        }
        var /= 96;
        /*Serial.print("phase ");
        Serial.print(i);
        Serial.print("; var "); 
        Serial.println(var);*/    
        if (var > best_var) {
          best_var = var;
          best_phase = i;
        }
      }
  
    for (int i = 0; i < 96; i++) {
      bits[i] = (phases[i][best_phase] < average);
    }
  }

  // look for '000' or '111'
  int start;
  for (int i = 0; i < 96; i++) {
    if ((bits[i] == true) && (bits[(i+1)%96] == true) && (bits[(i+2)%96] == true)) {
      start = (i+3) % 96;
    }
  }


  for (int i = 0; i < 96; i++) {
    if (bits[(start + i) % 96] == false) {
      Serial.print('0');
    } else {
      Serial.print('1');
    }
  }


  Serial.println();
  
  for (int i = 0; i < 45; i++) {
    if (bits[(start + 2*i) % 96] == false) {
      data_bits[i] = '0';
      Serial.print('0');
    } else {
      data_bits[i] = '1';
      Serial.print('1');
    }
    if (bits[(start + 2*i) % 96] == bits[(start + 2*i  + 1) % 96]) {
      //Serial.print("(p)");
      for (int j = 0; i < 45; i++) {
        data_bits[j] = '0';
      }
      return false;
    }
  }
  return true;
}

void cleanUp() {
  // clean stuff up!
  memset(data, 0, sizeof(data));
  
  n = 0;
  old = 0;
  new_ = 0;
  memset(existsSignal, false, sizeof(existsSignal));
  times = 0;
  isOn = false;
  final = "";
  
  //Serial.println("clean'd");
}


void checkAccess() {
  // checks whether data stored in data_bits has access!
  //Serial.println("checking");

  // computes the encoded number
  long number = 0;
  {  long twos = 1;
    for (int i = 0; i < 20; i++) {
      if (data_bits[43 -i] == '1') {
        number += twos;
      }
      twos *= 2;
    }
  }
  Serial.println();
  //Serial.print("checking number: ");
  Serial.println(number);
  String numberString = String(number);
  while (numberString.length() < 6) {
    numberString = "0" + numberString;
  }
  
  Sha256.init();


  // THE FOLLOWING IS REDACTED FOR SECURITY REASONS
  // with access of the salt, Cal1IDs on the SD card can be cracked.
  // for the actual salt, email mlyzhong@gmail.com 
  //Sha256.print("something secret");
  
  for (int i = 0; i < 6; i++) {
    Sha256.print(numberString[i]);
  }
  
  hash = Sha256.result();

  final = "";
  for (int i = 0; i < 32; i++) {
    String letter = String(hash[i], HEX);
    while (letter.length() < 2) {
      letter = "0" + letter;
    }
    final += letter;
  }

  //Serial.println(final);
  return;

}

boolean checkString() {
  // check whether hash is stored in files
  File dataBase = SD.open("database.txt");
  if (dataBase) {
    int counter = 0;
    boolean match = true;
    while (dataBase.available()) {
      char current = dataBase.read();
      if (current != final[counter]) {
        match = false;
      } 
      counter++;
      if ((match == true) && (counter == 64)) {
        dataBase.close();
        return true;
      }
      if (current == '\n') {
        counter = 0; 
        match = true;
      }
    }
   
  }
  dataBase.close(); 
  return false;
}

void grantAccess() {
   // opens the door, blinks green LED
   Serial.println("\n\nGRANTED\n\n");
   for (int i = 0; i < BLINKTIME; i++) {
     digitalWrite(GREEN, HIGH);
     digitalWrite(DOOR, HIGH);
     
   }
   digitalWrite(GREEN, LOW);
   cleanUp();
}

void denyAccess() {
  // blinks red LED
  
  //perhahps square wave isn't perfectly at 125kHz? 
  Serial.println("\n\ndenied!\n\n");
  for (int i = 0; i < BLINKTIME; i++) {
     digitalWrite(RED, HIGH);
     digitalWrite(RED, LOW);
  }
  cleanUp();
}


void loop() {
}


