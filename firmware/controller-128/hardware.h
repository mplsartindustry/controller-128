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

#ifndef hardware_h
#define hardware_h

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Adafruit_NeoTrellis.h>

// Size of the NeoTrellis array
#define TRELLIS_WIDTH 16
#define TRELLIS_HEIGHT 8

// Constants for BPM calculation
#define TICKS_PER_BEAT 8
#define DEFAULT_BPM 60

// Shorter way to define a color
#define COLOR(r, g, b) seesaw_NeoPixel::Color((r), (g), (b))

namespace Hardware {
  extern Adafruit_NeoTrellis trellisArray[TRELLIS_HEIGHT / 4][TRELLIS_WIDTH / 4];
  extern Adafruit_MultiTrellis trellis;

  extern LiquidCrystal lcd;

  enum Encoder : uint8_t {
    LEFT = 0,
    RIGHT = 1
  };
  
  void init();

  // Outputs
  inline void setPixel(uint8_t x, uint8_t y, uint32_t color) { trellis.setPixelColor(x, y, color); }
  inline void updateTrellis() { trellis.show(); }
  void outputTriggers(uint8_t out);

  // Interrupt handlers
  TrellisCallback buttonCallback(keyEvent event);
  void handleInterrupt();

  uint16_t getClockBPM();
  void setClockBPM(uint16_t bpm);
  
  void tickClock();
}

#endif
