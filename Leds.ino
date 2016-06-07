#include "LedControl.h"

int blinkLedPin = 13;
int buzz_pin = 50;

LedControl lc1 = LedControl(49,53,45,4);

void fled_setup(int bled) {
  blinkLedPin = bled;
  pinMode(blinkLedPin, OUTPUT);

  for(int ii=0; ii<4; ii++) {
    lc1.shutdown(ii, false);
    delay(10);
    lc1.setIntensity(ii, 3);
    delay(10);
    lc1.clearDisplay(ii);
    delay(10);
  }
}

void fled_blink(int veces, int espera) {
  for(int ii=0; ii<veces; ii++) {
    digitalWrite(blinkLedPin, HIGH);
    delay(espera);
    digitalWrite(blinkLedPin, LOW);
    delay(espera);
  }
}

void fbuzz_short() {
  tone(buzz_pin, 2000, 25);
}

void fbuzz_long() {
  tone(buzz_pin, 2200);
}

void fbuzz_off() {
  noTone(buzz_pin);
}

void fled_display_time(int t) {
  byte segs = t % 60;
  byte mins = t / 60;
  byte min1 = mins/10;
  byte min2 = mins%10;
  byte seg1 = segs/10;
  byte seg2 = segs%10;

  lc1.setDigit2(0, min1);
  delay(5);
  lc1.setDigit2(1, min2);
  delay(5);
  lc1.setDigit2(2, seg1);
  delay(5);
  lc1.setDigit2(3, seg2);
  delay(5);  
}

void fled_display_msg(const char * msg) {
  for(int ii=0; msg[ii] && ii<4; ii++) {
    lc1.clearDisplay(ii);
    lc1.setChar2(ii, msg[ii]);
  }
}


