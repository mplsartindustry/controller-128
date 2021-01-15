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

int cursorPos = 0;

const uint32_t off = color(0, 0, 30);
const uint32_t on = color(0, 30, 30);
uint32_t currentColor = off;

int eventCount = 0;
int lastDir = 0;
bool cleared = false;

void init() {
  
}

void updatePixel() {
  setPixel(cursorPos % 4, cursorPos / 4, currentColor);
  trellis.pixels.show();
}

void onClockRising() {
  currentColor = on;
  updatePixel();
}

void onClockFalling() {
  currentColor = off;
  updatePixel();
}

void onButtonPress(int x, int y) {
  int i = x + y * 4;
  if (i < 8) {
    setPixel(cursorPos % 4, cursorPos / 4, 0);
    cursorPos = i;
    updatePixel();
  }

  if (!cleared) {
    lcd.clear();
    cleared = true;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("(");
  lcd.print(x);
  lcd.print(", ");
  lcd.print(y);
  lcd.print(") pressed     ");
}

void onButtonRelease(int x, int y) {
  if (!cleared) {
    lcd.clear();
    cleared = true;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("(");
  lcd.print(x);
  lcd.print(", ");
  lcd.print(y);
  lcd.print(") released     ");
}

// -1 is counterclockwise, 1 is clockwise
void onEncoderChange(int encoder, int movement) {
  int dir = sign(movement);

  if (dir == lastDir) {
    eventCount++;
  } else {
    eventCount = 1;
  }

  if (!cleared) {
    lcd.clear();
    cleared = true;
  }

  lcd.setCursor(0, 0);
  lcd.print(dir == 1 ? "Encoder CW" : "Encoder CCW");
  if (eventCount > 1) {
    lcd.print(" x");
    lcd.print(eventCount);
  }
  lcd.print("   ");

  lastDir = dir;
  setPixel(cursorPos % 4, cursorPos / 4, 0);
  cursorPos += movement;
  if (cursorPos < 0) cursorPos += 8;
  if (cursorPos >= 8) cursorPos -= 8;
  updatePixel();
}

void onEncoderPress(int encoder) {
  if (!cleared) {
    lcd.clear();
    cleared = true;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Encoder pressed ");
}

void onEncoderRelease(int encoder) {
  if (!cleared) {
    lcd.clear();
    cleared = true;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Encoder released");
}

inline int sign(int val) {
 if (val < 0) return -1;
 if (val == 0) return 0;
 return 1;
}
