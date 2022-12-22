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

#ifndef controller_h
#define controller_h

#include <Arduino.h>
#include "hardware.h"

namespace Controller {
  void init();

  void onButtonPress(uint8_t x, uint8_t y);
  void onButtonRelease(uint8_t x, uint8_t y);

  void onClockRising();
  void onClockFalling();
  void onReset();

  void onEncoderTurn(Hardware::Encoder encoder, int16_t movement);
  void onEncoderPress(Hardware::Encoder encoder);
  void onEncoderRelease(Hardware::Encoder encoder);
}

#endif
