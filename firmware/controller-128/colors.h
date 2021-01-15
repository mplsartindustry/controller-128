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
#ifndef colors_h
#define colors_h

#include "Arduino.h"

class rgb {
  public:
  uint8_t r;
  uint8_t g;
  uint8_t b;
  rgb();
  rgb(uint8_t r, uint8_t g, uint8_t b);
  //hsv to_hsv();
  //hsv to_hsv(hsv out);
};

class hsv {
  public:
  double h;
  double s;
  double v;
  hsv();
  hsv(double h, double s, double v);
  //rgb to_rgb();
  //rgb to_rgb(rgb out);
};

rgb to_rgb(hsv in);
hsv to_hsv(rgb in);
rgb to_rgb(hsv in, rgb out);
hsv to_hsv(rgb in, hsv out);

rgb map_qual(int x, rgb colors[], int len);
rgb map_seq(int x, int min, int max, rgb colors[], int len);
rgb map_div(int x, int min, int max, rgb colors[], int len);

#endif
