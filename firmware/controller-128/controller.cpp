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

#include "controller.h"

#define STATE_WIDTH (TRELLIS_WIDTH*2)
#define STATE_HEIGHT TRELLIS_HEIGHT

#define CLOCK_ADJUST_STEP 1
#define MAX_BPM 100

namespace Controller {
  static const uint32_t blank  = COLOR( 0,  0,  0);
  static const uint32_t active = COLOR(30, 15,  0);
  static const uint32_t cursor = COLOR(30, 30, 30);
  static const uint32_t outOfBounds = COLOR(10, 0, 0);

  uint8_t displayState[STATE_WIDTH];

  bool playing = false;
  int cursorX = 0;

  int scrollX = 0;
  int scrollY = 0;
  int scrollXLimit = 16;
  int scrollYLimit = 0;
  int patternLength = 16;

  bool getState(uint8_t x, uint8_t y) {
    uint8_t mask = 1 << y;
    return (displayState[x] & mask) != 0;
  }

  void setState(uint8_t x, uint8_t y, bool b) {
    uint8_t mask = 1 << y;

    if (b) {
      displayState[x] |= mask;
    } else {
      displayState[x] &= ~mask;
    }
  }

  void clearDisplay() {
    // Clear state
    memset(displayState, 0, sizeof(displayState));

    // Clear display
    for (uint8_t x = 0; x < TRELLIS_WIDTH; x++) {
      for (uint8_t y = 1; y < TRELLIS_HEIGHT; y++) {
        Hardware::setPixel(x, y, playing && x == cursorX - scrollX ? cursor : blank);
      }
    }
  }

  void drawOutOfBounds() {
    int oobBegin = patternLength - scrollX;
    for (int x = oobBegin; x < TRELLIS_WIDTH; x++) {
      for (int y = 1; y < TRELLIS_HEIGHT; y++) {
        Hardware::setPixel(x, y, outOfBounds);
      }
    }
  }

  void eraseCursor() {
    int x = cursorX - scrollX;
    for (int y = 1; y < TRELLIS_HEIGHT; y++) {
      Hardware::setPixel(x, y, getState(cursorX, y + scrollY) ? active : blank);
    }
  }

  void drawCursor() {
    int x = cursorX - scrollX;
    for (int y = 1; y < TRELLIS_HEIGHT; y++) {
      Hardware::setPixel(x, y, getState(cursorX, y + scrollY) ? active : cursor);
    }
  }

  void showBPM() {
    uint16_t bpm = Hardware::getClockBPM();
    Hardware::lcd.setCursor(0, 1);
    Hardware::lcd.print(bpm);
    Hardware::lcd.print(" bpm, ");
    Hardware::lcd.print("__% sw     ");
  }

  void playPause() {
    if (playing) {
      playing = false;
      Hardware::setPixel(0, 0, blank);
      eraseCursor();
      Hardware::updateTrellis();
    } else {
      playing = true;
      cursorX = 0;
      Hardware::setPixel(0, 0, active);
      drawCursor();
      Hardware::updateTrellis();
    }
  }

  void resetCursor() {
    if (playing) {
      eraseCursor();
      cursorX = 0;
      drawCursor();
      Hardware::updateTrellis();
    }
  }

  void clearState() {
    clearDisplay();
    drawOutOfBounds();
    Hardware::updateTrellis();
  }

  void clockUp() {
    uint16_t bpm = Hardware::getClockBPM() + CLOCK_ADJUST_STEP;
    if (bpm > MAX_BPM) bpm = MAX_BPM;
    Hardware::setClockBPM(bpm);
    showBPM();
  }

  void clockDown() {
    uint16_t bpm = Hardware::getClockBPM() - CLOCK_ADJUST_STEP;
    if (bpm > MAX_BPM) bpm = 0; // Wrapped around to negative
    Hardware::setClockBPM(bpm);
    showBPM();
  }
  
  void init() {
    clearDisplay();
    drawOutOfBounds();
    showBPM();

    Hardware::updateTrellis();
    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Running         ");
  }

  void onButtonPress(uint8_t x, uint8_t y) {
    if (y == 0) {
      switch (x) {
        case 0:  playPause();   break;
        case 1:  resetCursor(); break;
        case 2:  clearState();  break;
        case 13: clockDown();   break;
        case 14: clockUp();     break;
      }
    } else {
      if (x + scrollX < patternLength) {
        bool state = !getState(x + scrollX, y + scrollY);
        setState(x + scrollX, y + scrollY, state);
        Hardware::setPixel(x, y, state ? active : blank);
      }
    }
    Hardware::updateTrellis();
  }

  void onButtonRelease(uint8_t x, uint8_t y) {
    
  }

  void onClockRising() {
    if (playing) {
      eraseCursor();
      cursorX++;
      if (cursorX >= patternLength) cursorX = 0;
      drawCursor();

      uint8_t triggers = displayState[cursorX];
      Hardware::outputTriggers(triggers);
    }

    Hardware::setPixel(15, 0, active);

    Hardware::updateTrellis();
  }

  void onClockFalling() {
    Hardware::outputTriggers(0);

    Hardware::setPixel(15, 0, blank);

    Hardware::updateTrellis();
  }

  void onReset() {
    resetCursor();
  }

  void onEncoderTurn(Hardware::Encoder encoder, int16_t movement) {
    while (movement != 0) {
      if (movement > 0) {
        clockUp();
        movement--;
      }
      if (movement < 0) {
        clockDown();
        movement++;
      }
    }
  }

  void onEncoderPress(Hardware::Encoder encoder) {
    playPause();
  }

  void onEncoderRelease(Hardware::Encoder encoder) {
    
  }
}
