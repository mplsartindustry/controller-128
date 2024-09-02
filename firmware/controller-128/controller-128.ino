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

#include "hardware.h"
#include "controller.h"

void setup() {
  Hardware::init();
  Controller::init();

  Serial.begin(9600);
}

void loop() {
  // uint64_t start = millis();
  Hardware::tickClock();
  // uint64_t middle = millis();
  Controller::tick();
  // uint64_t end = millis();

  // Hardware::lcd.setCursor(0, 0);
  // Hardware::lcd.print("HW: ");
  // Hardware::lcd.print(middle - start);
  // Hardware::lcd.print("   ");

  // Hardware::lcd.setCursor(0, 1);
  // Hardware::lcd.print("SW: ");
  // Hardware::lcd.print(end - middle);
  // Hardware::lcd.print("   ");
}
