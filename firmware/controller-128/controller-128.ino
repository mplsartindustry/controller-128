/*

    controller-128
    Copyright (c) 2020-2021 held jointly by the individual authors.

    This file is part of controller-128.

    controller-128 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    controller-128 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with controller-128.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <LiquidCrystal.h>
#include <Adafruit_NeoTrellis.h>
#include "colors.h"

// Pin definitions
// Trellis must use pins 5 and 6 (SCL/INT0, SDA/INT1)
#define LCD_RS 0
#define LCD_EN 1
#define LCD_D4 9
#define LCD_D5 10
#define LCD_D6 3
#define LCD_D7 2
#define ENCODER_A 21
#define ENCODER_B 20
#define ENCODER_S 19
#define COMMON_INTERRUPT 7    // INT2

#define TRELLIS_WIDTH 16
#define TRELLIS_HEIGHT 8

Adafruit_NeoTrellis trellisArray[TRELLIS_HEIGHT / 4][TRELLIS_WIDTH / 4] = {
  {Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31)},
  {Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x33), Adafruit_NeoTrellis(0x34), Adafruit_NeoTrellis(0x35)}
};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellisArray, TRELLIS_HEIGHT / 4, TRELLIS_WIDTH / 4);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

int lastTime = millis();
double unprocessedTime = 0;
float clocksPerSecond = 2;
bool clockEdge = false;

// Helpful methods
inline uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
  return seesaw_NeoPixel::Color(r, g, b);
}
inline void setPixel(int x, int y, uint32_t color) {
  trellis.setPixelColor(x, y, color);
}

// This is probably wrong
TrellisCallback callback(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    onButtonPress(evt.bit.NUM % TRELLIS_WIDTH, evt.bit.NUM / TRELLIS_WIDTH);
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    onButtonRelease(evt.bit.NUM % TRELLIS_WIDTH, evt.bit.NUM / TRELLIS_WIDTH);
  }
  
  trellis.show();
  
  return 0;
}

volatile bool lastA, lastB;
volatile int dir = 0;
volatile int encoderChange = 0;
bool lastS;

/*
 Interrupts are XOR'ed into the common interrupt pin, so we have to check which interrupt it was.
 
 All times an interrupt is triggered:
   - An encoder rotation signal changes
*/
void evaluateInterrupt() {
  // Check encoder
  bool currentA = digitalRead(ENCODER_A);
  bool currentB = digitalRead(ENCODER_B);
  if (currentA && currentB && (currentA != lastA || currentB != lastB) && ((dir == -1 && lastA && !lastB) || (dir == 1 && !lastA && lastB))) {
    encoderChange += dir;
    dir = 0;
  } else if (currentA && !currentB && lastA && lastB) {
    dir = 1;
  } else if (!currentA && currentB && lastA && lastB) {
    dir = -1;
  }
  lastA = currentA;
  lastB = currentB;
}

void setup() {
  // Start serial
  Serial.begin(9600);

  // Start LCD
  lcd.begin(16, 2);
  lcd.print("Starting trelli");

  // Start trellis
  if (!trellis.begin()) {
    Serial.println("Could not start trellis");
    lcd.setCursor(0, 0);
    lcd.print("Start failed!     ");
    while(1);
  } else {
    Serial.println("NeoPixel Trellis started");
    lcd.setCursor(0, 0);
    lcd.print("Started           ");
  }

  for (int x = 0; x < TRELLIS_WIDTH; x++) {
    for (int y = 0; y < TRELLIS_HEIGHT; y++) {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, callback);
    }
  }

  // Start encoder
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_S, INPUT_PULLUP);
  lastA = digitalRead(ENCODER_A);
  lastB = digitalRead(ENCODER_B);
  lastS = digitalRead(ENCODER_S);

  // Start controller
  init();

  // Attach interrupts
  pinMode(COMMON_INTERRUPT, INPUT);
  attachInterrupt(digitalPinToInterrupt(COMMON_INTERRUPT), evaluateInterrupt, CHANGE);
}

void loop() {
  trellis.read();
  
  if (encoderChange != 0) {
    onEncoderChange(0, encoderChange);
    encoderChange = 0;
  }
  bool currentS = digitalRead(ENCODER_S);
  // S is active low (low is pressed)
  if (!currentS && lastS) {
    onEncoderPress(0);
  }
  if (currentS && !lastS) {
    onEncoderRelease(0);
  }
  lastS = currentS;

  int currentTime = millis();
  int passedTime = currentTime - lastTime;
  lastTime = currentTime;
  unprocessedTime += passedTime / 1000.0;
  float secondsPerClock = 0.5f / clocksPerSecond;
  while (unprocessedTime > secondsPerClock) {
    if (clockEdge) {
      onClockRising();
    } else {
      onClockFalling();
    }
    clockEdge = !clockEdge;
    unprocessedTime -= secondsPerClock;
  }
}
