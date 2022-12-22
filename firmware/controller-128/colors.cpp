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
#include "colors.h"

rgb::rgb() {
  this->r = 0;
  this->g = 0;
  this->b = 0;
}

rgb::rgb(uint8_t r, uint8_t g, uint8_t b) {
  this->r = r;
  this->g = g;
  this->b = b;
}

rgb map_qual(int x, rgb colors[], int len) {
  return colors[(len % x)];
}

rgb map_seq(int x, int min, int max, rgb colors[], int len) {
  return colors[map(x, min, max, 0, len)];
}

rgb map_div(int x, int min, int max, rgb colors[], int len) {
  if (x > 0) {
    return colors[map(x, 0, max, len / 2, len)];
  }
  else {
    return colors[map(x, min, 0, 0, len / 2)];
  }
}
