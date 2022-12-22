/*

    controller-128
    Copyright (c) 2020 held jointly by the individual authors.

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

class Encoder {
  final color col = color(64, 64, 64);
  
  final int x, y, size;
  final char ccw, cw, s;
  
  public Encoder(int x, int y, int size, char ccw, char s, char cw) {
    this.x = x;
    this.y = y;
    this.size = size;
    
    this.ccw = ccw;
    this.cw = cw;
    this.s = s;
  }
  
  void draw() {
    stroke(0);
    fill(col);
    ellipseMode(CORNER);
    ellipse(x, y, size, size);
    
    float textY = y + size/2 + textAscent()/2;
    float quarter = size/4;
    
    fill(255);
    text(ccw, x + quarter, textY);
    text(s, x - textWidth(s) / 2 + size/2, textY);
    text(cw, x + size - quarter - textWidth(cw), textY);
  }
}
