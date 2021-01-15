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

/*
hsv rgb::to_hsv() {
  hsv hsv;
  return this->to_hsv(hsv);
}

hsv rgb::to_hsv(hsv hsv) {
  return to_hsv(this, hsv);
}
*/

hsv::hsv() {
  this->h = 0.0d;
  this->s = 0.0d;
  this->v = 0.0d;
}

hsv::hsv(double h, double s, double v) {
  this->h = h;
  this->s = s;
  this->v = v;
}

rgb to_rgb(hsv in) {
  rgb out;
  return to_rgb(in, out);
}

rgb to_rgb(hsv in, rgb out) {

  if (in.h > 360) {
    in.h -= 360;
  }
  if (in.h < 0) {
    in.h += 360;
  }

  in.h = constrain(in.h, 0, 360);
  in.s = constrain((in.s / 100.0), 0, 1);
  in.v = constrain((in.v / 100.0), 0, 1);

  float c = in.v * in.s;
  float x = c * (1 - fabsf(fmod((in.h / 360), 2) - 1));
  float m = in.v - c;
  float rp, gp, bp;

  int a = in.h / 60;

  switch (a) {
    case 0:
      rp = c;
      gp = x;
      bp = 0;
      break;

    case 1:
      rp = x;
      gp = c;
      bp = 0;
      break;

    case 2:
      rp = 0;
      gp = c;
      bp = x;
      break;

    case 3:
      rp = 0;
      gp = x;
      bp = c;
      break;

    case 4:
      rp = x;
      gp = 0;
      bp = c;
      break;

    default:
      rp = c;
      gp = 0;
      bp = x;
      break;
  }

  out.r = (rp + m) * 255;
  out.g = (gp + m) * 255;
  out.b = (bp + m) * 255;  
  return out;
}

hsv to_hsv(rgb in) {
  hsv out;
  return to_hsv(in, out);
}

hsv to_hsv(rgb in, hsv out) {

  float max = max(max(in.r, in.g), in.b);
  float min  = min(min(in.r, in.g), in.b);
  float delta = max - min;
  
  if (delta > 0) {
    if (max == in.r) {
      out.h = 60 * (fmod(((in.g - in.b) / delta), 6));
    }
    else if (max == in.g) {
      out.h = 60 * (((in.b - in.r) / delta) + 2);
    }
    else if (max == in.b) {
      out.h = 60 * (((in.r - in.g) / delta) + 4);
    }

    if (max > 0) {
      out.s = (delta / max) * 100;
    }
    else {
      out.s = 0;
    }

    out.v = map(max, 0, 255, 0, 100);
  }
  else {
    out.h = 0;
    out.s = 0;
    out.v = map(max, 0, 255, 0, 100);
  }

  if (out.h < 0) {
    out.h = 360 + out.h;
  }
  return out;
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
