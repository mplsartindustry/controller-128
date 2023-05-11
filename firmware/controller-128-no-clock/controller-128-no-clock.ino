/*

    controller-128-no-clock
    Copyright (c) 2020-2023 held jointly by the individual authors.

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

#include <Adafruit_NeoTrellis.h>

/*

  Teensy 2 pins:

  1 - CLOCK IN
  2 - RESET IN

  5/D0 - Trellis green
  6/D1 - Trellis white
  7/D2 - save for Ryan port
  8/D3 - save for Ryan port

  10 - TRIGGER OUT 1

  11 - built-in LED

  12 - TRIGGER OUT 2
  13 - TRIGGER OUT 3
  14 - TRIGGER OUT 4
  15 - TRIGGER OUT 5
  16 - TRIGGER OUT 6
  17 - TRIGGER OUT 7

 */
#define CLOCK 1
#define RESET 2
#define TRIGGER_1 10
#define TRIGGER_2 12
#define TRIGGER_3 13
#define TRIGGER_4 14
#define TRIGGER_5 15
#define TRIGGER_6 16
#define TRIGGER_7 17


//
// trellis
//

#define TRELLIS_WIDTH 16
#define TRELLIS_HEIGHT 8

Adafruit_NeoTrellis trellisArray[TRELLIS_HEIGHT / 4][TRELLIS_WIDTH / 4] = {
  {Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31)},
  {Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x33), Adafruit_NeoTrellis(0x34), Adafruit_NeoTrellis(0x35)}
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellisArray, TRELLIS_HEIGHT / 4, TRELLIS_WIDTH / 4);


//
// colors
//

#define rgb(r, g, b) seesaw_NeoPixel::Color((r), (g), (b))

uint32_t blues[] = {
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

uint32_t highlighted = blues[4];
uint32_t selected = blues[5];
uint32_t pressed = blues[6];
uint32_t deselected = blues[8];
uint32_t off = rgb(0, 0, 0);

uint32_t bugn[] = {
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

uint32_t fhighlighted = bugn[4];
uint32_t fselected = bugn[5];
uint32_t fpressed = bugn[6];
uint32_t fdeselected = bugn[8];

uint32_t map_seq(int x, int min, int max, uint32_t colors[], int len) {
  return colors[map(x, min, max, 0, len)];
}


//
// tracks
//

// todo: use #define or enum or strategy pattern
const int ARP_FORWARD = 0;
const int ARP_REVERSE = 1;

struct Track {
  // starting position
  int start = 0;
  
  // length of track in steps
  int length = 16;
  
  // current step, offset from start
  int current = 0;
  
  // current position after reset, offset from start
  int phase = 0;
  
  // current number of ticks
  int ticks = 0;

  // number of ticks to increment current step
  int duration = 1;

  // arp mode
  int arp = ARP_FORWARD;

  // bits
  boolean bits[16]; // todo: extend to available memory

  void tick() {
    ticks++;
    if (ticks >= duration) {
      step();
    }
  }

  void step() {
    ticks = 0;
    if (arp == ARP_FORWARD) {
      current++;
      if (current >= length) {
        current = 0;
      }
    }
    else if (arp == ARP_REVERSE) {
      current--;
      if (current <= start) {
        current = length - 1;
      }
    }
    // todo: add'l arp ...
  }

  void reset() {
    ticks = 0;
    current = phase;
  }

  boolean get() {
    return bits[start + current];
  }

  boolean get(int i) {
    return bits[start + i];
  }

  void set(int i, boolean v) {
    bits[start + i] = v;
  }

  void toggle(int i) {
    if (get(i)) {
      set(i, false);
    }
    else {
      set(i, true);
    }
  }

  void clear() {
    for (int i = 0; i < length; i++) {
      set(i, false);
    }
  }

  boolean check(int i) {
    return (i >= start) && (i < (start + length));
  }

  boolean highlighted(int i) {
    return i == (current + start);
  }

  boolean selected(int i) {
    return check(i) && get(i);
  }

  boolean pressed(int i) {
    return false;    
  }

  boolean deselected(int i) {
    return check(i);
  }
};

Track tracks[7];


//
// buttons
//

// top row buttons
#define CLOCK_BUTTON 0
#define RESET_BUTTON 1
#define CLEAR_BUTTON 2
#define INCREASE_START_BUTTON 3 // or cycle start?
#define DECREASE_START_BUTTON 4
#define INCREASE_PHASE_BUTTON 5
#define DECREASE_PHASE_BUTTON 6
#define INCREASE_LENGTH_BUTTON 7
#define DECREASE_LENGTH_BUTTON 8
#define INCREASE_DURATION_BUTTON 9
#define DECREASE_DURATION_BUTTON 10
#define INCREASE_ARP_BUTTON 11
#define DECREASE_ARP_BUTTON 12
#define FOCUS_BUTTON 13
#define INCREASE_FOCUS_BUTTON 14 // or page right?
#define DECREASE_FOCUS_BUTTON 15 // or page left

// focus
int focus_index = 0;
boolean focus_mode = false;

// cache trigger values
boolean trigger1 = false;
boolean trigger2 = false;
boolean trigger3 = false;
boolean trigger4 = false;
boolean trigger5 = false;
boolean trigger6 = false;
boolean trigger7 = false;

void clock_pressed() {
  trellis.setPixelColor(CLOCK_BUTTON, 0, pressed);
  trellis.show();
}

void clock_released() {
  trellis.setPixelColor(CLOCK_BUTTON, 0, off);
  trellis.show();

  // tick
  for (int i = 0; i < 7; i++) {
    Track* t = &tracks[i];
    t->tick();
    if (t->get()) {
      switch (i) {
        case 0: digitalWrite(TRIGGER_1, HIGH); trigger1 = true; break;
        case 1: digitalWrite(TRIGGER_2, HIGH); trigger2 = true; break;
        case 2: digitalWrite(TRIGGER_3, HIGH); trigger3 = true; break;
        case 3: digitalWrite(TRIGGER_4, HIGH); trigger4 = true; break;
        case 4: digitalWrite(TRIGGER_5, HIGH); trigger5 = true; break;
        case 5: digitalWrite(TRIGGER_6, HIGH); trigger6 = true; break;
        case 6: digitalWrite(TRIGGER_7, HIGH); trigger7 = true; break;
        default: break;
      }
    }
  }
}

void reset_pressed() {
  trellis.setPixelColor(RESET_BUTTON, 0, pressed);
  trellis.show();
}

void reset_released() {
  trellis.setPixelColor(RESET_BUTTON, 0, off);
  trellis.show();

  for (int i = 0; i < 7; i++) {
    Track* t = &tracks[i];
    t->reset();
  }
}

void clear_pressed() {
  trellis.setPixelColor(CLEAR_BUTTON, 0, pressed);
  trellis.show();
}

void clear_released() {
  trellis.setPixelColor(CLEAR_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->clear();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->clear();
    }
  }
}

void increase_start_pressed() {
  trellis.setPixelColor(INCREASE_START_BUTTON, 0, pressed);
  trellis.show();
}

void increase_start_released() {
  trellis.setPixelColor(INCREASE_START_BUTTON, 0, off);
  trellis.show();

  // todo: all these incr/decr need bounds checking
  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->start++;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->start++;
    }
  }
}

void decrease_start_pressed() {
  trellis.setPixelColor(DECREASE_START_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_start_released() {
  trellis.setPixelColor(DECREASE_START_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->start--;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->start--;
    }
  }
}

void increase_phase_pressed() {
  trellis.setPixelColor(INCREASE_PHASE_BUTTON, 0, pressed);
  trellis.show();
}

void increase_phase_released() {
  trellis.setPixelColor(INCREASE_PHASE_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->phase++;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->phase++;
    }
  }
}

void decrease_phase_pressed() {
  trellis.setPixelColor(DECREASE_PHASE_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_phase_released() {
  trellis.setPixelColor(DECREASE_PHASE_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->phase--;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->phase--;
    }
  }
}

void increase_length_pressed() {
  trellis.setPixelColor(INCREASE_LENGTH_BUTTON, 0, pressed);
  trellis.show();
}

void increase_length_released() {
  trellis.setPixelColor(INCREASE_LENGTH_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->length++;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->length++;
    }
  }
}

void decrease_length_pressed() {
  trellis.setPixelColor(DECREASE_LENGTH_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_length_released() {
  trellis.setPixelColor(DECREASE_LENGTH_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->length--;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->length--;
    }
  }
}

void increase_duration_pressed() {
  trellis.setPixelColor(INCREASE_DURATION_BUTTON, 0, pressed);
  trellis.show();
}

void increase_duration_released() {
  trellis.setPixelColor(INCREASE_DURATION_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->duration++;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->duration++;
    }
  }
}

void decrease_duration_pressed() {
  trellis.setPixelColor(DECREASE_DURATION_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_duration_released() {
  trellis.setPixelColor(DECREASE_DURATION_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->duration--;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->duration--;
    }
  }
}

void focus_pressed() {
  trellis.setPixelColor(FOCUS_BUTTON, 0, pressed);
  trellis.show();

  focus_mode = !focus_mode;
}

void focus_released() {
  if (focus_mode) {
    trellis.setPixelColor(FOCUS_BUTTON, 0, selected);
  }
  else {
    trellis.setPixelColor(FOCUS_BUTTON, 0, off);
  }
  trellis.show();
}

void increase_focus_pressed() {
  trellis.setPixelColor(INCREASE_FOCUS_BUTTON, 0, pressed);
  trellis.show();
}

void increase_focus_released() {
  trellis.setPixelColor(INCREASE_FOCUS_BUTTON, 0, off);
  trellis.show();

  focus_index++;
}

void decrease_focus_pressed() {
  trellis.setPixelColor(DECREASE_FOCUS_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_focus_released() {
  trellis.setPixelColor(DECREASE_FOCUS_BUTTON, 0, off);
  trellis.show();

  focus_index--;
}

void increase_arp_pressed() {
  trellis.setPixelColor(INCREASE_ARP_BUTTON, 0, pressed);
  trellis.show();
}

void increase_arp_released() {
  trellis.setPixelColor(INCREASE_ARP_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->arp++;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->arp++;
    }
  }
}

void decrease_arp_pressed() {
  trellis.setPixelColor(DECREASE_ARP_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_arp_released() {
  trellis.setPixelColor(DECREASE_ARP_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->arp--;
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->arp--;
    }
  }
}

void toggle_pressed(int x, int y) {
  Track* t = &tracks[y-1];
  t->toggle(x - t->start);

  // waiting for loop is too slow
  boolean track_focused = focus_mode && (focus_index == (y - 1));
  if (t->highlighted(x)) {
    set_color(x, y, track_focused ? fhighlighted : highlighted);
  }
  else if (t->selected(x)) {
    set_color(x, y, track_focused ? fselected : selected);
  }
  else if (t->pressed(x)) {
    set_color(x, y, track_focused ? fpressed : pressed);
  }
  else if (t->deselected(x)) {
    set_color(x, y, track_focused ? fdeselected : deselected);
  }
  else {
    set_color(x, y, off);
  }
  trellis.show();
}

void toggle_released(int x, int y) {
  // empty
}

TrellisCallback buttonCallback(keyEvent evt) {
  int x = evt.bit.NUM % TRELLIS_WIDTH;
  int y = evt.bit.NUM / TRELLIS_WIDTH;

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {

    // button row
    if (y == 0) {
      switch (x) {
        case CLOCK_BUTTON: clock_pressed(); break;
        case RESET_BUTTON: reset_pressed(); break;
        case CLEAR_BUTTON: clear_pressed(); break;
        case INCREASE_START_BUTTON: increase_start_pressed(); break;
        case DECREASE_START_BUTTON: decrease_start_pressed(); break;
        case INCREASE_PHASE_BUTTON: increase_phase_pressed(); break;
        case DECREASE_PHASE_BUTTON: decrease_phase_pressed(); break;
        case INCREASE_LENGTH_BUTTON: increase_length_pressed(); break;
        case DECREASE_LENGTH_BUTTON: decrease_length_pressed(); break;
        case INCREASE_DURATION_BUTTON: increase_duration_pressed(); break;
        case DECREASE_DURATION_BUTTON: decrease_duration_pressed(); break;
        case FOCUS_BUTTON: focus_pressed(); break;
        case INCREASE_FOCUS_BUTTON: increase_focus_pressed(); break;
        case DECREASE_FOCUS_BUTTON: decrease_focus_pressed(); break;
        case INCREASE_ARP_BUTTON: increase_arp_pressed(); break;
        case DECREASE_ARP_BUTTON: decrease_arp_pressed(); break;
        default: break;
      }
    }
    else {
      toggle_pressed(x, y);
    }
  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {

    // button row
    if (y == 0) {
      switch (x) {
        case CLOCK_BUTTON: clock_released(); break;
        case RESET_BUTTON: reset_released(); break;
        case CLEAR_BUTTON: clear_released(); break;
        case INCREASE_START_BUTTON: increase_start_released(); break;
        case DECREASE_START_BUTTON: decrease_start_released(); break;
        case INCREASE_PHASE_BUTTON: increase_phase_released(); break;
        case DECREASE_PHASE_BUTTON: decrease_phase_released(); break;
        case INCREASE_LENGTH_BUTTON: increase_length_released(); break;
        case DECREASE_LENGTH_BUTTON: decrease_length_released(); break;
        case INCREASE_DURATION_BUTTON: increase_duration_released(); break;
        case DECREASE_DURATION_BUTTON: decrease_duration_released(); break;
        case FOCUS_BUTTON: focus_released(); break;
        case INCREASE_FOCUS_BUTTON: increase_focus_released(); break;
        case DECREASE_FOCUS_BUTTON: decrease_focus_released(); break;
        case INCREASE_ARP_BUTTON: increase_arp_released(); break;
        case DECREASE_ARP_BUTTON: decrease_arp_released(); break;
        default: break;
      }
    }
    else {
      toggle_released(x, y);
    }
  }
  return 0;
}


//
// setup
//

// frame buffer of LED colors
uint32_t buffer[7*16];

void setup() {

  // set pin modes
  pinMode(CLOCK, INPUT);
  pinMode(RESET, INPUT);
  pinMode(TRIGGER_1, OUTPUT);
  pinMode(TRIGGER_2, OUTPUT);
  pinMode(TRIGGER_3, OUTPUT);
  pinMode(TRIGGER_4, OUTPUT);
  pinMode(TRIGGER_5, OUTPUT);
  pinMode(TRIGGER_6, OUTPUT);
  pinMode(TRIGGER_7, OUTPUT);

  // initialize trellis
  trellis.begin();

  // register trellis callback
  for (uint8_t x = 0; x < TRELLIS_WIDTH; x++) {
    for (uint8_t y = 0; y < TRELLIS_HEIGHT; y++) {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING,  true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, buttonCallback);
    }
  }

  // say hello
  int wave = 12;
  for (int x = 0; x < TRELLIS_WIDTH; x++) {
    for (int y = 0; y < TRELLIS_HEIGHT; y++) {
      if (x > (wave - 1)) {
        trellis.setPixelColor(x - wave, y, off);
      }
      trellis.setPixelColor(x, y, map_seq(x, 0, TRELLIS_WIDTH, blues, 9));
    }
    trellis.show();
    delay(20);
  }
  for (int w = wave; w > 0; w--) {
    for (int y = 0; y < TRELLIS_HEIGHT; y++) {
      trellis.setPixelColor(TRELLIS_WIDTH - w, y, off);
    }
    trellis.show();
    delay(20);
  }
  delay(180);

  // initialize screen buffer
  int size = 7 * 16;
  for (int i = 0; i < size; i++) {
    buffer[i] = off;
  }
}

//
// main loop
//

boolean previousClock = false;

void set_color(int x, int y, uint32_t c) {
  int i = (y-1) * 16 + x;
  uint32_t old = buffer[i];
  if (c != old) {
    buffer[i] = c;
    trellis.setPixelColor(x, y, c);
  }
}

void loop() {
  // check clock and reset pins
  boolean clock = digitalRead(CLOCK);
  boolean reset = digitalRead(RESET);

  // clock rising
  if (clock && !previousClock) {
    trellis.setPixelColor(CLOCK_BUTTON, 0, selected);

    // tick
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->tick();
      if (t->get()) {
        switch (i) {
          case 0: digitalWrite(TRIGGER_1, HIGH); trigger1 = true; break;
          case 1: digitalWrite(TRIGGER_2, HIGH); trigger2 = true; break;
          case 2: digitalWrite(TRIGGER_3, HIGH); trigger3 = true; break;
          case 3: digitalWrite(TRIGGER_4, HIGH); trigger4 = true; break;
          case 4: digitalWrite(TRIGGER_5, HIGH); trigger5 = true; break;
          case 5: digitalWrite(TRIGGER_6, HIGH); trigger6 = true; break;
          case 6: digitalWrite(TRIGGER_7, HIGH); trigger7 = true; break;
          default: break;
        }
      }
    }
  }

  // clock falling
  if (!clock && previousClock) {
    trellis.setPixelColor(CLOCK_BUTTON, 0, off);

    // stop triggers
    if (trigger1) { digitalWrite(TRIGGER_1, LOW); trigger1 = false; }
    if (trigger2) { digitalWrite(TRIGGER_2, LOW); trigger2 = false; }
    if (trigger3) { digitalWrite(TRIGGER_3, LOW); trigger3 = false; }
    if (trigger4) { digitalWrite(TRIGGER_4, LOW); trigger4 = false; }
    if (trigger5) { digitalWrite(TRIGGER_5, LOW); trigger5 = false; }
    if (trigger6) { digitalWrite(TRIGGER_6, LOW); trigger6 = false; }
    if (trigger7) { digitalWrite(TRIGGER_7, LOW); trigger7 = false; }
  }
  previousClock = clock;

  // update trellis track pixels
  boolean dirty = true;
  for (int y = 1; y < TRELLIS_HEIGHT; y++) {
    Track* t = &tracks[y-1];
    boolean track_focused = focus_mode && (focus_index == (y - 1));

    for (int x = 0; x < TRELLIS_WIDTH; x++) {
      if (t->highlighted(x)) {
        set_color(x, y, track_focused ? fhighlighted : highlighted);
      }
      else if (t->selected(x)) {
        set_color(x, y, track_focused ? fselected : selected);
      }
      else if (t->pressed(x)) {
        set_color(x, y, track_focused ? fpressed : pressed);
      }
      else if (t->deselected(x)) {
        set_color(x, y, track_focused ? fdeselected : deselected);
      }
      else {
        set_color(x, y, off);
      }
    }
  }

  // update trellis LEDs if necessary
  if (dirty) {
    trellis.show();
  }

  // allow trellis to read button state
  trellis.read();

  // do reset if necessary
  if (reset) {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->reset();
    }
  }
}
