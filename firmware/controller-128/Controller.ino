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

#define STATE_WIDTH TRELLIS_WIDTH*2
#define STATE_HEIGHT TRELLIS_HEIGHT

#define STEPS_PER_BEAT 8
#define SECONDS_PER_MINUTE 60
#define BPM_PER_CLOCK_ADJUST 1

// Colors
uint32_t blank = color(0, 0, 0);
uint32_t active = color(30, 15, 0);
uint32_t cursor = color(30, 30, 30);
uint32_t outOfBounds = color(10, 0, 0);

bool displayState[STATE_WIDTH][STATE_HEIGHT];

bool playing = false;
int cursorX = 0;

int scrollX = 0;
int scrollY = 0;
int scrollXLimit = 16;
int scrollYLimit = 0;
int patternLength = 16;

// ----- REQUIRED FUNCTIONS -----------------------------

void init() {
  clearDisplay();
  drawOutOfBounds();
  updateBPM();
}

void onButtonPress(int x, int y) {
  if (y == 0) {
    switch (x) {
      case 0: playPause();   break;
      case 1: resetCursor(); break;
      case 2: clearState();  break;
      case 13: clockDown();  break;
      case 14: clockUp();    break;
    }
  } else {
    if (x + scrollX < patternLength) {
      bool state = !displayState[x + scrollX][y + scrollY];
      displayState[x + scrollX][y + scrollY] = state;
      setPixel(x, y, state ? active : blank);
    }
  }
}

void onButtonRelease(int x, int y) {
  
}

void onClockRising() {
  if (playing) {
    eraseCursor();
    cursorX++;
    if (cursorX >= patternLength) cursorX = 0;
    drawCursor();
  }

  byte triggers = 0;
  for (int i = 0; i < 8; i++) {
    triggers |= displayState[cursorX][i] << i;
  }
  outputTriggers(triggers);

  // Clock blinky on
  setPixel(15, 0, active);

  trellis.show();
}

void onClockFalling() {
  // Clear triggers
  outputTriggers(0);
  
  // Clock blinky off
  setPixel(15, 0, blank);

  trellis.show();
}

// -1 is counterclockwise, 1 is clockwise
void onEncoderChange(int encoder, int movement) {
  if (movement > 0) clockUp();
  if (movement < 0) clockDown();
}

void onEncoderPress(int encoder) {
  playPause();
}

void onEncoderRelease(int encoder) {
  
}

// ----- END REQUIRED FUNCTIONS -------------------------

// ----- BUTTON FUNCTIONS -------------------------------

void playPause() {
  if (playing) {
    playing = false;
    setPixel(0, 0, blank);
    eraseCursor();
    trellis.show();
  } else {
    playing = true;
    cursorX = 0;
    setPixel(0, 0, active);
    drawCursor();
    trellis.show();
  }
}

void resetCursor() {
  if (playing) {
    eraseCursor();
    cursorX = 0;
    drawCursor();
    trellis.show();
  }
}

void clearState() {
  clearDisplay();
  drawOutOfBounds();
  trellis.show();
}

void clockUp() {
  clocksPerSecond += (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST;
  if (clocksPerSecond > 100) clocksPerSecond = 100; // Seems like a reasonable limit
  updateBPM();
}

void clockDown() {
  clocksPerSecond -= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST;
  if (clocksPerSecond < 0) clocksPerSecond = 0;
  updateBPM();
}

// ----- END BUTTON FUNCTIONS ---------------------------

// ----- DISPLAY FUNCTIONS ------------------------------

void clearDisplay() {
  // Clear state
  for (int x = 0; x < STATE_WIDTH; x++) {
    for (int y = 0; y < STATE_HEIGHT; y++) {
      displayState[x][y] = false;
    }
  }

  // Clear display
  for (int x = 0; x < TRELLIS_WIDTH; x++) {
    for (int y = 1; y < TRELLIS_HEIGHT; y++) {
      setPixel(x, y, playing && x == cursorX - scrollX ? cursor : blank);
    }
  }
}

void drawOutOfBounds() {
  int oobBegin = patternLength - scrollX;
  for (int x = oobBegin; x < TRELLIS_WIDTH; x++) {
    for (int y = 1; y < TRELLIS_HEIGHT; y++) {
      setPixel(x, y, outOfBounds);
    }
  }
}

void eraseCursor() {
  int x = cursorX - scrollX;
  for (int y = 1; y < TRELLIS_HEIGHT; y++) {
    setPixel(x, y, displayState[cursorX][y + scrollY] ? active : blank);
  }
}

void drawCursor() {
  int x = cursorX - scrollX;
  for (int y = 1; y < TRELLIS_HEIGHT; y++) {
    setPixel(x, y, displayState[cursorX][y + scrollY] ? active : cursor);
  }
}

void updateBPM() {
  int bpm = (int) (clocksPerSecond * (float) SECONDS_PER_MINUTE / (float) STEPS_PER_BEAT);
  lcd.setCursor(0, 1);
  lcd.print(bpm);
  lcd.print(" bpm, ");
  lcd.print("__% sw     ");
}

// ----- END DISPLAY FUNCTIONS --------------------------
