/*

    controller-128-no-clock
    Copyright (c) 2020-2024 held jointly by the individual authors.

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
  rgb(8,48,107),
  rgb(6,36,80),
  rgb(4,24,53),
  rgb(2,12,27),
  rgb(0,2,5),
  rgb(0,1,2),
  rgb(0,0,1)
};

uint32_t highlighted = blues[4];
uint32_t selected = blues[5];
uint32_t pressed = blues[6];
uint32_t deselected = blues[8];
uint32_t off = blues[14];
//uint32_t off = rgb(0, 0, 0);

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

#define SIZE 64


//
// tracks
//

enum Arp : uint8_t {
  UP = 0,
  DOWN = 1,
  UP_DOWN = 2,
  REPEAT_LAST = 3,
  RANDOM = 4,
  RANDOM_WALK = 5,
  WRAPPING_RANDOM_WALK = 6,
  WRAPPING_RANDOM_WALK_MOSTLY_RIGHT = 7
};

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
  int arp = Arp::UP;

  // arp direction
  int direction = 1;

  // bit array stored in uint64_t
  uint64_t bits;

  // advance ticks based on clock
  void tick() {
    ticks++;
    if (ticks >= duration) {
      step();
    }
  }

  // advance step based on ticks per step
  void step() {
    ticks = 0;
    int next = 0;

    switch (arp) {
      case Arp::UP:
        current++;
        if (current >= length) {
          current = start;
        }
        break;

      case Arp::DOWN:
        current--;
        if (current < start) {
          current = (length - 1);
        }
        break;

      case Arp::UP_DOWN:
        if (direction > 0) {
          current++;
          if (current >= (length - 1)) {
            direction = -1;
            current = (length - 1);
          }
        }
        else {
          current--;
          if (current <= start) {
            direction = 1;
            current = start;
          }
        }
        break;

      case Arp::REPEAT_LAST:
        if (direction > 0) {
          current++;
          if (current >= length) {
            direction = -1;
            current = (length - 1);
          }
        }
        else {
          current--;
          if (current < start) {
            direction = 1;
            current = start;
          }
        }
        break;

      case Arp::RANDOM:
        next = random(length);
        while (next == current) {
          next = random(length);
        }
        current = next;
        break;

      case Arp::RANDOM_WALK:
        if (random(2) == 1) {
          current++;
          if (current >= length) {
            current = (length - 1);
          }
        }
        else {
          current--;
          if (current <= start) {
            current = start;
          }
        }
        break;

      case Arp::WRAPPING_RANDOM_WALK:
        if (random(2) == 1) {
          current++;
          if (current >= length) {
            current = start;
          }
        }
        else {
          current--;
          if (current <= start) {
            current = (length - 1);
          }
        }
        break;

      case Arp::WRAPPING_RANDOM_WALK_MOSTLY_RIGHT:
        if (random(100) < 60) {
          current++;
          if (current >= length) {
            current = start;
          }
        }
        else {
          current--;
          if (current <= start) {
            current = (length - 1);
          }
        }
        break;

      default: break;
    }
  }

  // reset ticks, set current to phase
  void reset() {
    ticks = 0;
    current = phase;
  }

  // get bit at current
  boolean get() {
    return get(current);
  }

  // get bit at index i
  boolean get(int i) {
    int bitIdx = start + i;
    return bits & (1LL << bitIdx);
  }

  // set bit at index i to value v
  void set(int i, boolean v) {
    int bitIdx = start + i;
    uint64_t bit = 1LL << bitIdx;
    if (v) {
      bits |= bit;
    } else {
      bits &= ~bit;
    }
  }

  // toggle bit at index i
  void toggle(int i) {
    if (get(i)) {
      set(i, false);
    }
    else {
      set(i, true);
    }
  }

  // set all bits up to length to false
  void clear() {
    for (int i = 0; i < length; i++) {
      set(i, false);
    }
  }

  // check index i is within start and start + length
  boolean check(int i) {
    return (i >= start) && (i < (start + length));
  }

  // true if index i is highlighted
  boolean highlighted(int i) {
    return i == (current + start);
  }

  // true if index i is selected
  boolean selected(int i) {
    return check(i) && get(i);
  }

  // true if index i is pressed
  boolean pressed(int i) {
    return false;    
  }

  // true if index i is deselected
  boolean deselected(int i) {
    return check(i);
  }

  // increase start within length, wrapping to zero
  void increase_start() {
    start++;
    if (start >= length) {
      start = 0;
    }
  }

  // decrease start within length, wrapping to length - 1
  void decrease_start() {
    start--;
    if (start < 0) {
      start = length - 1;
    }
  }

  // increase phase within length, wrapping to zero
  void increase_phase() {
    phase++;
    if (phase >= length) {
      phase = 0;
    }
  }

  // decrease phase within length, wrapping to length - 1
  void decrease_phase() {
    phase--;
    if (phase < 0) {
      phase = length - 1;
    }
  }

  // increase length, up to size
  void increase_length() {
    length++;
    if (length > SIZE)  {
      length = SIZE;
    }
  }

  // decrease length, down to one
  void decrease_length() {
    length--;
    if (length < 1) {
      length = 1;
    }
    if (start >= length) {
      start = length - 1;
    }
    if (phase >= length) {
      phase = length - 1;
    }
  }

  // increase duration, ticks per step
  void increase_duration() {
    duration++;
  }

  // decrease duration, ticks per step, down to one
  void decrease_duration() {
    duration--;
    if (duration < 1) {
      duration = 1;
    }
  }

  // increase arp, wrapping to zero
  void increase_arp() {
    arp++;
    if (arp > 7) {
      arp = 0;
    }
  }

  // decrease arp, wrapping to seven
  void decrease_arp() {
    arp--;
    if (arp < 0) {
      arp = 7;
    }
  }

  // copy current page to next page, increasing length if necessary
  void copy(int from, int width, int to) {
    if (to + width > length) {
      length = min(to + width, SIZE);
    }
    for (int i = 0; i < width; i++) {
      if ((to + i) < length) {
        set(to + i, get(from + i));
      }
    }
  }

  void randomize() {
    int likelihood = 4;
    for (int i = start; i < length; i++) {
      if (i == 0) {
        likelihood = 48;
      }
      else if (i % 8 == 0) {
        likelihood = 64;
      }
      else if (i % 4 == 0) {
        likelihood = 16;
      }
      else if (i % 2 == 0) {
        likelihood = 8;
      }
      else {
        likelihood = 4;
      }
      if (random(100) < likelihood) {
        set(i, true);
      }
    }
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

// unused
#define DECREASE_START_BUTTON -1
#define INCREASE_START_BUTTON -2

#define DECREASE_PHASE_BUTTON 3
#define INCREASE_PHASE_BUTTON 4

#define DECREASE_LENGTH_BUTTON 5
#define INCREASE_LENGTH_BUTTON 6

#define DECREASE_ARP_BUTTON 7
#define INCREASE_ARP_BUTTON 8

// unused
#define DECREASE_DURATION_BUTTON -3
#define INCREASE_DURATION_BUTTON -4

#define FOCUS_BUTTON 9
#define DECREASE_FOCUS_BUTTON 10
#define INCREASE_FOCUS_BUTTON 11

#define RANDOMIZE_BUTTON 12

#define COPY_BUTTON 13

#define DECREASE_VIEW_PAGE_BUTTON 14
#define INCREASE_VIEW_PAGE_BUTTON 15


// focus
int focus_index = 0;
boolean focus_mode = false;

// view page
int view = 0;

int view_to_local(int x) {
  return x + view;
}

int local_to_view(int x) {
  return x - view;
}

void decrease_view_page() {
  view -= TRELLIS_WIDTH;
  if (view < 0) {
    view = 0;
  }
}

void increase_view_page() {
  view += TRELLIS_WIDTH;
  if (view >= SIZE) {
    view = SIZE - TRELLIS_WIDTH;
  }
}

// cache trigger values
boolean trigger1 = false;
boolean trigger2 = false;
boolean trigger3 = false;
boolean trigger4 = false;
boolean trigger5 = false;
boolean trigger6 = false;
boolean trigger7 = false;

boolean previousClock = false;


//
// button handling
//

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
  previousClock = true;
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

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->increase_start();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->increase_start();
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
    t->decrease_start();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->decrease_start();
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
    t->increase_phase();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->increase_phase();
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
    t->decrease_phase();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->decrease_phase();
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
    t->increase_length();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->increase_length();
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
    t->decrease_length();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->decrease_length();
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
    t->increase_duration();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->increase_duration();
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
    t->decrease_duration();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->decrease_duration();
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
  if (focus_index > 6) {
    focus_index = 0;
  }
}

void decrease_focus_pressed() {
  trellis.setPixelColor(DECREASE_FOCUS_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_focus_released() {
  trellis.setPixelColor(DECREASE_FOCUS_BUTTON, 0, off);
  trellis.show();

  focus_index--;
  if (focus_index < 0) {
    focus_index = 6;
  }
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
    t->increase_arp();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->increase_arp();
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
    t->decrease_arp();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->decrease_arp();
    }
  }
}

void increase_view_page_pressed() {
  trellis.setPixelColor(INCREASE_VIEW_PAGE_BUTTON, 0, pressed);
  trellis.show();
}

void increase_view_page_released() {
  trellis.setPixelColor(INCREASE_VIEW_PAGE_BUTTON, 0, off);
  trellis.show();

  increase_view_page();
}

void decrease_view_page_pressed() {
  trellis.setPixelColor(DECREASE_VIEW_PAGE_BUTTON, 0, pressed);
  trellis.show();
}

void decrease_view_page_released() {
  trellis.setPixelColor(DECREASE_VIEW_PAGE_BUTTON, 0, off);
  trellis.show();

  decrease_view_page();
}

void randomize_pressed() {
  trellis.setPixelColor(RANDOMIZE_BUTTON, 0, pressed);
  trellis.show();
}

void randomize_released() {
  trellis.setPixelColor(RANDOMIZE_BUTTON, 0, off);
  trellis.show();

  if (focus_mode) {
    Track* t = &tracks[focus_index];
    t->randomize();
  }
  else {
    for (int i = 0; i < 7; i++) {
      Track* t = &tracks[i];
      t->randomize();
    }
  }
}

void copy_pressed() {
  trellis.setPixelColor(COPY_BUTTON, 0, pressed);
  trellis.show();
}

void copy_released() {
  trellis.setPixelColor(COPY_BUTTON, 0, off);
  trellis.show();

  int from = view_to_local(0);
  for (int i = 0; i < 7; i++) {
    Track* t = &tracks[i];

    int width = min(t->length - from, TRELLIS_WIDTH);
    int to = from + width;

    t->copy(from, width, to);
  }
}

void toggle_pressed(int x, int y) {
  Track* t = &tracks[y-1];

  int lx = view_to_local(x);
  t->toggle(lx - t->start);

  // waiting for loop is too slow
  boolean track_focused = focus_mode && (focus_index == (y - 1));
  if (t->highlighted(lx)) {
    set_color(x, y, track_focused ? fhighlighted : highlighted);
  }
  else if (t->selected(lx)) {
    set_color(x, y, track_focused ? fselected : selected);
  }
  else if (t->pressed(lx)) {
    set_color(x, y, track_focused ? fpressed : pressed);
  }
  else if (t->deselected(lx)) {
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
        case INCREASE_VIEW_PAGE_BUTTON: increase_view_page_pressed(); break;
        case DECREASE_VIEW_PAGE_BUTTON: decrease_view_page_pressed(); break;
        case RANDOMIZE_BUTTON: randomize_pressed(); break;
        case COPY_BUTTON: copy_pressed(); break;
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
        case INCREASE_VIEW_PAGE_BUTTON: increase_view_page_released(); break;
        case DECREASE_VIEW_PAGE_BUTTON: decrease_view_page_released(); break;
        case RANDOMIZE_BUTTON: randomize_released(); break;
        case COPY_BUTTON: copy_released(); break;
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
// frame buffer
//

// frame buffer of LED colors
uint32_t buffer[7*16];

boolean set_color(int x, int y, uint32_t c) {
  int i = (y-1) * 16 + x;
  uint32_t old = buffer[i];
  if (c != old) {
    buffer[i] = c;
    trellis.setPixelColor(x, y, c);
    return true;
  }
  return false;
}


//
// setup
//

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
      trellis.setPixelColor(x, y, map_seq(x, 0, TRELLIS_WIDTH, blues, 14));
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
      int lx = view_to_local(x);
      if (t->highlighted(lx)) {
        dirty |= set_color(x, y, track_focused ? fhighlighted : highlighted);
      }
      else if (t->selected(lx)) {
        dirty |= set_color(x, y, track_focused ? fselected : selected);
      }
      else if (t->pressed(lx)) {
        dirty |= set_color(x, y, track_focused ? fpressed : pressed);
      }
      else if (t->deselected(lx)) {
        dirty |= set_color(x, y, track_focused ? fdeselected : deselected);
      }
      else {
        dirty |= set_color(x, y, off);
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
