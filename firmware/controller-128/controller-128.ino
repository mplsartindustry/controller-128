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
#include "colors.h"

void setup() {
  rgb c1 = rgb(10, 20, 30);
  hsv c2 = hsv(0.6d, 0.4d, 0.2d);

  hsv c3 = to_hsv(c1);
  rgb c4 = to_rgb(c2);

  hsv c5;
  c5 = to_hsv(c1, c5);
  rgb c6;
  c6 = to_rgb(c2, c6);

  int len = 8;
  rgb colors[] = {
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30),
    rgb(10, 20, 30)
  };

  rgb c7 = map_seq(2, 0, 127, colors, len);
  rgb c8 = map_div(2, -127, 127, colors, len);
  rgb c9 = map_div(-2, -127, 127, colors, len);
  rgb c0 = map_qual(2, colors, len);
}

void loop() {
  // empty
}
