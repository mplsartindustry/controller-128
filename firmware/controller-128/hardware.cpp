/*

    controller-128
    Copyright (c) 2020-2022 held jointly by the individual authors.

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

#include "hardware.h"
#include "controller.h"

// Pin definitions
// Trellis must use pins 5 and 6 (SCL/INT0, SDA/INT1)
#define LCD_RS 10
#define LCD_EN 4
#define LCD_D4 3
#define LCD_D5 0
#define LCD_D6 1
#define LCD_D7 2
#define L_ENCODER_A 14
#define L_ENCODER_B 13
#define L_ENCODER_S 18
#define R_ENCODER_A 16
#define R_ENCODER_B 15
#define R_ENCODER_S 21
#define SHIFT_DATA 17
#define SHIFT_CLK 20
#define SHIFT_LATCH 19
#define COMMON_INTERRUPT 8    // INT3
#define CLOCK_INTERRUPT 7
#define RESET 9

namespace Hardware {
  Adafruit_NeoTrellis trellisArray[TRELLIS_HEIGHT / 4][TRELLIS_WIDTH / 4] = {
    {Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31)},
    {Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x33), Adafruit_NeoTrellis(0x34), Adafruit_NeoTrellis(0x35)}
  };
  Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellisArray, TRELLIS_HEIGHT / 4, TRELLIS_WIDTH / 4);

  LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
  
  struct EncoderState {
    Encoder which;
    uint8_t pinA, pinB, pinS;
    int16_t change;
    int8_t dir;
    bool prevA, prevB;
    bool prevS;

    EncoderState(Encoder _which, uint8_t _pinA, uint8_t _pinB, uint8_t _pinS):
      which(_which), pinA(_pinA), pinB(_pinB), pinS(_pinS), change(0), dir(0), prevA(0), prevB(0), prevS(0) {}

    void init() {
      pinMode(pinA, INPUT_PULLUP);
      pinMode(pinB, INPUT_PULLUP);
      pinMode(pinS, INPUT_PULLUP);
      prevA = !digitalRead(pinA);
      prevB = digitalRead(pinB);
      prevS = digitalRead(pinS);
    }
  
    inline void handlePins(bool a, bool b) {
      if (a && b && (a != prevA || b != prevB) && ((dir == -1 && prevA && !prevB) || (dir == 1 && !prevA && prevB))) {
        change += dir;
        dir = 0;
      } else if (a && !b && prevA && prevB) {
        dir = 1;
      } else if (!a && b && prevA && prevB) {
        dir = -1;
      }
      prevA = a;
      prevB = b;
    }

    void callHandlers() {
      if (change != 0) {
        Controller::onEncoderTurn(which, change);
        change = 0;
      }
      
      bool s = digitalRead(pinS);
      if (!s && prevS) {
        Controller::onEncoderPress(which);
      }
      if (s && !prevS) {
        Controller::onEncoderRelease(which);
      }
      prevS = s;
    }
  };

  // Circular buffer for interrupt pin state queue
  #define INTERRUPT_BUF_SIZE 32
  volatile uint8_t interruptBuffer[INTERRUPT_BUF_SIZE];
  volatile uint8_t interruptWriteIdx = 0;
  uint8_t interruptReadIdx = 0;

  EncoderState leftEncoder(Encoder::LEFT, L_ENCODER_A, L_ENCODER_B, L_ENCODER_S);
  EncoderState rightEncoder(Encoder::RIGHT, R_ENCODER_A, R_ENCODER_B, R_ENCODER_S);

  bool softwareClockEnabled;
  uint64_t prevTime;
  double unprocessedTime;
  double secondsPerPhase;
  bool clockEdge;
  uint16_t bpm;
  
  inline void initShiftRegister() {
    pinMode(SHIFT_DATA,  OUTPUT);
    pinMode(SHIFT_CLK,   OUTPUT);
    pinMode(SHIFT_LATCH, OUTPUT);
    digitalWrite(SHIFT_LATCH, LOW);
    digitalWrite(SHIFT_CLK,   LOW);
    outputTriggers(0);
  }

  inline void initLCD() {
    lcd.begin(16, 2);
    lcd.print("Starting...");
  }

  inline void initTrellis() {
    if (!trellis.begin()) {
      lcd.setCursor(0, 0);
      lcd.print("Start failed!    ");
      while (true)
        delay(1);
    }

    for (uint8_t x = 0; x < TRELLIS_WIDTH; x++) {
      for (uint8_t y = 0; y < TRELLIS_HEIGHT; y++) {
        trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING,  true);
        trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
        trellis.registerCallback(x, y, buttonCallback);
      }
    }
  }

  inline void initEncoders() {
    leftEncoder.init();
    rightEncoder.init();
  }

  #define INTERRUPT(pin, fn) attachInterrupt(digitalPinToInterrupt(pin), fn, CHANGE)
  inline void initInterrupts() {
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, LOW);
    
    pinMode(COMMON_INTERRUPT, INPUT);
    INTERRUPT(COMMON_INTERRUPT, handleInterrupt);
  }

  inline void initClock() {
    prevTime = millis();
    clockEdge = false;

    softwareClockEnabled = true;
    setClockBPM(DEFAULT_BPM);
  }

  inline void initSerial() {
    Serial.begin(9600);
    lcd.setCursor(0, 0);
    lcd.print("Awaiting serial ");
    while (!Serial)
      delay(1);
  }
  
  void init() {
    initShiftRegister();
    initLCD();
    initTrellis();
    initEncoders();
    initInterrupts();
    initClock();
  }

  void outputTriggers(uint8_t out) {
    digitalWrite(SHIFT_CLK, LOW);
    shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, out);
    digitalWrite(SHIFT_LATCH, HIGH);
    digitalWrite(SHIFT_LATCH, LOW);
  }

  TrellisCallback buttonCallback(keyEvent evt) {
    if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
      Controller::onButtonPress(evt.bit.NUM % TRELLIS_WIDTH, evt.bit.NUM / TRELLIS_WIDTH);
    } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
      Controller::onButtonRelease(evt.bit.NUM % TRELLIS_WIDTH, evt.bit.NUM / TRELLIS_WIDTH);
    }

    return 0;
  }

  void handleInterrupt() {
    // Access port registers directly for fastest possible speed
    uint8_t state = (PINB & 0b01110000) | (PINF & 0b10000000);

    // Write to circular buffer
    interruptBuffer[interruptWriteIdx++] = state;
    if (interruptWriteIdx >= INTERRUPT_BUF_SIZE)
      interruptWriteIdx -= INTERRUPT_BUF_SIZE;
  }

  uint16_t getClockBPM() {
    return bpm;
  }

  void setClockBPM(uint16_t newBPM) {
    bpm = newBPM;  
    secondsPerPhase = 30.0 / (double) (bpm * TICKS_PER_BEAT);
  }

  bool isSoftwareClockEnabled() {
    return softwareClockEnabled;
  }

  void setSoftwareClockEnabled(bool enabled) {
    softwareClockEnabled = enabled;
  }

  void tickClock() {
    trellis.read();

    // Encoders & reset
    while (interruptReadIdx != interruptWriteIdx) {
      uint8_t state = interruptBuffer[interruptReadIdx++];
      if (interruptReadIdx >= INTERRUPT_BUF_SIZE)
        interruptReadIdx -= INTERRUPT_BUF_SIZE;

      leftEncoder.handlePins((state & 0b00100000) != 0, (state & 0b00010000) != 0);
      rightEncoder.handlePins((state & 0b10000000) != 0, (state & 0b01000000) != 0);
    }
    
    leftEncoder.callHandlers();
    rightEncoder.callHandlers();

    // Software clock
    if (softwareClockEnabled) {
      uint64_t time = millis();
      uint64_t passedTime = time - prevTime;
      prevTime = time;
      unprocessedTime += passedTime / 1000.0;
      while (unprocessedTime > secondsPerPhase) {
        if (clockEdge) {
          Controller::onClockRising();    
        } else {
          Controller::onClockFalling();
        }
        clockEdge = !clockEdge;
        unprocessedTime -= secondsPerPhase;
      }
    } else {
      if (!clockEdge) {
        Controller::onClockFalling();
        clockEdge = true;
      }

      unprocessedTime = 0;
      prevTime = millis();
    }
  }
}
