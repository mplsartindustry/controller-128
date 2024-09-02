/*

    mplsartindustry/controller-128
    Copyright (c) 2020-2024 held jointly by the individual authors.

    This file is part of mplsartindustry/controller-128.

    mplsartindustry/controller-128 is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    mplsartindustry/controller-128 is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mplsartindustry/controller-128.  If not, please see
    <http://www.gnu.org/licenses/>.

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
#define TICKS_PER_BEAT 4
#define DEFAULT_BPM 60

#define MIN_TEMPO 2
#define MAX_TEMPO 250

// Shorter way to define a color
#define COLOR(r, g, b) seesaw_NeoPixel::Color((r), (g), (b))

namespace Hardware {
  typedef TrellisCallback (**TrellisCallbackArray)(keyEvent);
  class FastTrellis : public Adafruit_NeoTrellis {
  public:
    FastTrellis(uint8_t addr): Adafruit_NeoTrellis(addr) {}

    inline TrellisCallbackArray getCallbacks() {
      return _callbacks;
    }
  };

  class FastMultiTrellis : public Adafruit_MultiTrellis {
  public:
    FastMultiTrellis(FastTrellis* trellisArray, uint8_t rows, uint8_t cols)
      : Adafruit_MultiTrellis((Adafruit_NeoTrellis*) trellisArray, rows, cols), row(0), col(0) {};

    void read(uint8_t count);
  private:
    uint8_t row, col;
  };
  extern FastTrellis trellisArray[TRELLIS_HEIGHT / 4][TRELLIS_WIDTH / 4];
  extern FastMultiTrellis trellis;

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

  bool isSoftwareClockEnabled();
  void setSoftwareClockEnabled(bool enabled);

  uint16_t getClockBPM();
  void setClockBPM(uint16_t bpm);
  void setClockInterval(uint64_t interval);
  
  void tickClock();
}

#endif
