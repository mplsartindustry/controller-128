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
};

rgb map_qual(int x, rgb colors[], int len);
rgb map_seq(int x, int min, int max, rgb colors[], int len);
rgb map_div(int x, int min, int max, rgb colors[], int len);

rgb blues[] = {
  rgb(247,251,255),
  rgb(222,235,247),
  rgb(198,219,239),
  rgb(158,202,225),
  rgb(107,174,214),
  rgb(66,146,198),
  rgb(33,113,181),
  rgb(8,81,156),
  rgb(8,48,107)
};

rgb bugn[] = {
  rgb(247,252,253),
  rgb(229,245,249),
  rgb(204,236,230),
  rgb(153,216,201),
  rgb(102,194,164),
  rgb(65,174,118),
  rgb(35,139,69),
  rgb(0,109,44),
  rgb(0,68,27)
};

rgb gnbu[] = {
  rgb(247,252,240),
  rgb(224,243,219),
  rgb(204,235,197),
  rgb(168,221,181),
  rgb(123,204,196),
  rgb(78,179,211),
  rgb(43,140,190),
  rgb(8,104,172),
  rgb(8,64,129)
};

rgb pubu[] = {
  rgb(255,247,251),
  rgb(236,231,242),
  rgb(208,209,230),
  rgb(166,189,219),
  rgb(116,169,207),
  rgb(54,144,192),
  rgb(5,112,176),
  rgb(4,90,141),
  rgb(2,56,88)
};

rgb ylgn[] = {
  rgb(255,255,229),
  rgb(247,252,185),
  rgb(217,240,163),
  rgb(173,221,142),
  rgb(120,198,121),
  rgb(65,171,93),
  rgb(35,132,67),
  rgb(0,104,55),
  rgb(0,69,41)
};

// divergent

rgb prgn[] = {
  rgb(118,42,131),
  rgb(153,112,171),
  rgb(194,207,165),
  rgb(231,212,232),
  rgb(247,247,247),
  rgb(217,240,211),
  rgb(166,219,160),
  rgb(90,174,97),
  rgb(27,120,55)
};

rgb rdbu[] = {
  rgb(178,24,43),
  rgb(214,96,77),
  rgb(244,165,130),
  rgb(253,219,199),
  rgb(247,247,247),
  rgb(209,229,240),
  rgb(146,197,222),
  rgb(67,147,195),
  rgb(33,102,172)
};

rgb rdylbu[] = {
  rgb(215,48,39),
  rgb(244,109,67),
  rgb(253,174,97),
  rgb(254,224,139),
  rgb(255,255,191),
  rgb(224,243,248),
  rgb(171,217,233),
  rgb(116,173,209),
  rgb(69,117,180)
};

rgb rdylgr[] = {
  rgb(215,48,39),
  rgb(244,109,67),
  rgb(253,174,97),
  rgb(254,224,139),
  rgb(255,255,191),
  rgb(217,239,139),
  rgb(166,217,106),
  rgb(102,189,99),
  rgb(26,152,80)
};

rgb rubus[] = {
  rgb(159,30,95),
  rgb(239,118,159),
  rgb(241,199,221),
  rgb(252,231,240),
  rgb(255,255,255),
  rgb(225,243,247),
  rgb(214,239,245),
  rgb(164,221,232),
  rgb(46,190,201)
};

// qualitative

rgb aaas[] = {
  rgb(59,73,146),
  rgb(238,0,0),
  rgb(0,139,69),
  rgb(99,24,121),
  rgb(0,130,128),
  rgb(187,0,33),
  rgb(95,85,155),
  rgb(162,0,86),
  rgb(128,129,128)
};

rgb lancet[] = {
  rgb(0,70,139),
  rgb(237,0,0),
  rgb(66,181,64),
  rgb(0,153,180),
  rgb(146,94,159),
  rgb(253,175,145),
  rgb(173,0,42),
  rgb(173,182,182),
  rgb(27,25,25)
};

rgb npg[] = {
  rgb(230,75,53),
  rgb(77,187,213),
  rgb(0,160,135),
  rgb(60,84,136),
  rgb(243,155,127),
  rgb(132,145,180),
  rgb(145,209,194),
  rgb(220,0,0),
  rgb(126,97,72)
};

#endif
