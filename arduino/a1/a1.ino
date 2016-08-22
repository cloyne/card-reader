// no need to short cables 5 and 2
// V_out = pin 3

#include <avr/io.h>
#include <util/delay.h>


void setup() {
  //Serial.begin(9600); 
  // put your setup code here, to run once:
  // output:  
  pinMode(3, OUTPUT);  
 
  TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  // Normal port operation, OC2A disconnected | 
  // Clear OC2B on Compare Match when up-counting. Set OC2B on Compare Match when down-counting.
  // Fast PWM
  TCCR2B = _BV(WGM22) | _BV(CS21);
  // divide ratio 8
  OCR2A = 15; // 12.5 kHz = (16 MHz / 8) / 160
  OCR2B = 7;
}
void loop() {
  // put your main code here, to run repeatedly:

}
