/*

    mplsartindustry/controller-128
    Copyright (c) 2020-2024 held jointly by the individual authors.

    This file is part of mplsartindustry/controller-128.

    mplsartindustry/controller-128 is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    mplsartindustry/controller-128 is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mplsartindustry/controller-128.  If not, please see
    <http://www.gnu.org/licenses/>.

*/

#include "controller.h"

#define PANEL_WIDTH 16
#define PANEL_HEIGHT 8

#define CLOCK_X 0
#define CLOCK_MODE_X 1
#define SETTINGS_X 2
#define CLEAR_X 3
#define SONG_X 4
#define PATTERNS_START_X 5
#define DIRECTION_X 12
#define PLAY_SONG_X 13
#define PLAY_PATTERN_X 14
#define RESET_X 15

#define PATTERN_COUNT 7

#define MIN_PATTERN_LEN 2
#define DEFAULT_PATTERN_LEN 16
#define MAX_PATTERN_LEN 32

#define TEMPO_STEP 2
#define MAX_TEMPO_TAPS 4

// Millis
#define POPUP_PERSIST_TIME 1500

namespace Controller {
  struct Pattern {
    uint32_t blankColor, activeColor;

    uint8_t state[MAX_PATTERN_LEN];
    uint8_t length = DEFAULT_PATTERN_LEN;

    uint8_t scroll = 0;

    Pattern(uint32_t blank, uint32_t active):
        blankColor(blank), activeColor(active) {
      memset(state, 0, sizeof(state));
    }

    void copyFrom(Pattern *from) {
      memcpy(state, from->state, MAX_PATTERN_LEN);
      length = from->length;
      scroll = from->scroll;
    }

    void rotateLeft() {
      uint8_t first = state[0];
      memmove(&state[0], &state[1], length - 1);
      state[length - 1] = first;
    }

    void rotateRight() {
      uint8_t last = state[length - 1];
      memmove(&state[1], &state[0], length - 1);
      state[0] = last;
    }
  };

  struct SongPattern {
    // High bits first, then low
    uint8_t state[MAX_PATTERN_LEN >> 1];
    uint8_t length = DEFAULT_PATTERN_LEN;

    uint8_t scroll = 0;

    SongPattern() {
      memset(state, 0, sizeof(state));
    }

    uint8_t get(uint8_t index) {
      if (index & 1)
        return state[index / 2] & 0x0F;
      else
        return state[index / 2] >> 4;
    }

    void set(uint8_t index, uint8_t val) {
      if (index & 1)
        state[index / 2] = (state[index / 2] & 0xF0) | val;
      else
        state[index / 2] = (state[index / 2] & 0x0F) | (val << 4);
    }

    void rotateLeft() {
      uint8_t first = get(0);
      for (uint8_t i = 0; i < length - 1; i++) {
        set(i, get(i + 1));
      }
      set(length - 1, first);
    }

    void rotateRight() {
      uint8_t last = get(length - 1);
      for (uint8_t i = length - 1; i > 0; i--) {
        set(i, get(i - 1));
      }
      set(0, last);
    }
  };

  // --- MODEL ---
  Pattern patterns[PATTERN_COUNT] {
    Pattern(COLOR(3, 0, 0), COLOR(30,  0,  0)),
    Pattern(COLOR(3, 2, 0), COLOR(30, 15,  0)),
    Pattern(COLOR(3, 3, 0), COLOR(30, 30,  0)),
    Pattern(COLOR(0, 3, 0), COLOR( 0, 30,  0)),
    Pattern(COLOR(0, 3, 3), COLOR( 0, 30, 30)),
    Pattern(COLOR(0, 0, 3), COLOR( 0,  0, 30)),
    Pattern(COLOR(2, 0, 3), COLOR(15,  0, 30))
  };
  SongPattern songPattern;

  Pattern* playingPattern = nullptr; // Null: not playing
  bool playedSongPreviously = false;
  int8_t cursorX;
  int8_t songCursorX = -1; // -1: not playing song
  int8_t direction = 1;

  uint64_t prevTapTime;
  uint64_t tapDurations[MAX_TEMPO_TAPS];
  int8_t tapCount = -1;

  uint8_t currentOutputs = 0x00;
  uint8_t gateMask = 0x00; // If bit is set, the channel is a gate, otherwise it's a trigger

  bool clockOn = false;

  // --- VIEW ---
  #define MAX_COLUMN_UPDATES_PER_LOOP 2

  static const uint32_t CURSOR        = COLOR(15, 15, 15);
  static const uint32_t OUT_OF_BOUNDS = COLOR( 0,  0,  0);

  static const uint32_t CLOCK_FORWARD = COLOR(0, 30, 0);
  static const uint32_t CLOCK_BACKWARD = COLOR(30, 15, 0);
  static const uint32_t CLOCK_OFF = COLOR(0, 0, 0);

  static const uint32_t SONG_ACTIVE = COLOR(30, 30, 30);
  static const uint32_t SONG_BLANK = COLOR(3, 3, 3);

  static const uint32_t PLAY_STOP = COLOR(15, 0, 0);

  static const uint32_t CLOCK_MODE_SOFTWARE = COLOR(30, 30, 0);
  static const uint32_t CLOCK_MODE_HARDWARE = COLOR(30, 15, 0);

  static const uint32_t SETTINGS_TRIGGER = COLOR(8, 8, 0);
  static const uint32_t SETTINGS_GATE = COLOR(0, 8, 0);

  Pattern* viewedPattern = nullptr; // If null, song pattern
  uint8_t viewedPatternIdx = 0;
  uint16_t dirtyColumns = 0xFFFF;
  bool rightEncoderPressed = false;
  uint64_t popupTime = 0; // 0 = no popup
  bool settingsMenuOpen = false;
  uint8_t heldPatterns = 0;
  bool songHeld = false;

  #define PIXEL_TO_PATTERN(x) ((x) + getCurrentScroll())
  #define PATTERN_TO_PIXEL(x) ((x) - getCurrentScroll())

  uint8_t getCurrentScroll() {
    return viewedPattern ? viewedPattern->scroll : songPattern.scroll;
  }

  void redrawColumn(uint8_t patternX) {
    int8_t pixelX = PATTERN_TO_PIXEL(patternX);
    if (pixelX >= 0 || pixelX < PANEL_WIDTH) {
      dirtyColumns |= (1 << pixelX);
    }
  }

  inline void updatePatternColumn(uint8_t pixelX) {
    uint8_t patternX = PIXEL_TO_PATTERN(pixelX);
    if (patternX >= viewedPattern->length) {
      for (uint8_t y = 1; y < PANEL_HEIGHT; y++)
        Hardware::setPixel(pixelX, y, OUT_OF_BOUNDS);
      return;
    }

    uint32_t unset = (viewedPattern == playingPattern && patternX == cursorX)
      ? CURSOR : viewedPattern->blankColor;
    uint8_t state = viewedPattern->state[patternX];

    for (uint8_t y = 1; y < PANEL_HEIGHT; y++)
      Hardware::setPixel(pixelX, y, state & (1 << y) ? viewedPattern->activeColor : unset);
  }

  uint8_t getSongState(uint8_t col) {
    uint8_t idx = col >> 1;
    if (col & 1)
      return songPattern.state[idx] & 0xF;
    else
      return songPattern.state[idx] >> 4;
  }

  inline void updateSongColumn(uint8_t pixelX) {
    uint8_t patternX = PIXEL_TO_PATTERN(pixelX);
    if (patternX >= songPattern.length) {
      for (uint8_t y = 1; y < PANEL_HEIGHT; y++)
        Hardware::setPixel(pixelX, y, OUT_OF_BOUNDS);
      return;
    }

    // Will only be true when playing, since patternX can never be -1
    bool isCursor = patternX == songCursorX;

    uint8_t patternIdx = getSongState(patternX);

    for (uint8_t y = 1; y < PANEL_HEIGHT; y++) {
      Pattern* rowPattern = &patterns[y - 1];

      uint32_t color;
      if (y - 1 == patternIdx)
        color = rowPattern->activeColor;
      else if (isCursor)
        color = CURSOR;
      else
        color = rowPattern->blankColor;

      Hardware::setPixel(pixelX, y, color);
    }
  }

  inline void updateSettingsColumn(uint8_t pixelX) {
    for (uint8_t y = 1; y < PANEL_HEIGHT; y++) {
      uint32_t color = gateMask & (1 << y) ? SETTINGS_GATE : SETTINGS_TRIGGER;
      Hardware::setPixel(pixelX, y, color);
    }
  }

  inline void updatePixels() {
    if (!dirtyColumns)
      return;

    uint8_t updates = 0;
    for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
      uint16_t cond = dirtyColumns & (1 << x);
      if (!cond) continue;
      dirtyColumns ^= cond; // Clear this bit

      if (settingsMenuOpen)
        updateSettingsColumn(x);
      else if (viewedPattern)
        updatePatternColumn(x);
      else
        updateSongColumn(x);

      if (++updates >= MAX_COLUMN_UPDATES_PER_LOOP)
        break;
    }

    if (updates)
      Hardware::updateTrellis();
  }

  void drawInitialControlRow() {
    Hardware::setPixel(CLOCK_MODE_X, 0, CLOCK_MODE_SOFTWARE);
    Hardware::setPixel(DIRECTION_X, 0, CLOCK_FORWARD);

    for (uint8_t x = 0; x < PATTERN_COUNT; x++) {
      Hardware::setPixel(x + PATTERNS_START_X, 0, patterns[x].blankColor);
    }
  }

  void switchToPattern(uint8_t index) {
    if (!viewedPattern)
      Hardware::setPixel(SONG_X, 0, SONG_BLANK);
    else {
      if (viewedPatternIdx == index)
        return;
      
      Hardware::setPixel(PATTERNS_START_X + viewedPatternIdx, 0, viewedPattern->blankColor);
    }

    viewedPattern = &patterns[index];
    viewedPatternIdx = index;
    dirtyColumns = 0xFFFF;

    Hardware::setPixel(PATTERNS_START_X + index, 0, viewedPattern->activeColor);
    if (!playingPattern)
      Hardware::setPixel(PLAY_PATTERN_X, 0, viewedPattern->activeColor);
  }

  void switchToSong() {
    if (!viewedPattern) return;
    Hardware::setPixel(PATTERNS_START_X + viewedPatternIdx, 0, viewedPattern->blankColor);

    if (!playingPattern)
      Hardware::setPixel(PLAY_PATTERN_X, 0, SONG_ACTIVE);

    viewedPattern = nullptr;
    dirtyColumns = 0xFFFF;
    Hardware::setPixel(SONG_X, 0, SONG_ACTIVE);
  }

  inline uint32_t currentPatternActive() {
    if (viewedPattern)
      return viewedPattern->activeColor;
    else
      return SONG_ACTIVE;    
  }

  inline void stopPlaying() {
    if (playingPattern && playingPattern == viewedPattern)
      redrawColumn(cursorX);
    if (songCursorX >= 0 && !viewedPattern)
      redrawColumn(songCursorX);

    playingPattern = nullptr;
    songCursorX = -1;

    Hardware::setPixel(PLAY_SONG_X, 0, SONG_ACTIVE);
    Hardware::setPixel(PLAY_PATTERN_X, 0, currentPatternActive());

    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Idle            ");

    Hardware::outputTriggers(0x00); // No outputs
    currentOutputs = 0x00;
  }

  // Prevents the current popup from ending, the lcd should be updated after
  inline void cancelPopup() {
    popupTime = 0;
  }

  void beginNewLengthPopup(uint8_t len) {
    Hardware::lcd.setCursor(0, 1);
    Hardware::lcd.print("New length: ");
    Hardware::lcd.print(len);
    Hardware::lcd.print("   ");
    popupTime = millis();
  }

  void updateTempoLCDInfo() {
    cancelPopup();
    Hardware::lcd.setCursor(0, 1);
    Hardware::lcd.print("Tempo: ");
    Hardware::lcd.print(Hardware::getClockBPM());
    Hardware::lcd.print(" BPM   ");
  }

  void init() {
    cursorX = 0;
    drawInitialControlRow();
    switchToPattern(0);
    stopPlaying();
    updateTempoLCDInfo();
    Hardware::updateTrellis();
  }

  inline void toggleClockMode() {
    bool enabled = !Hardware::isSoftwareClockEnabled();
    Hardware::setSoftwareClockEnabled(enabled);
    Hardware::setPixel(CLOCK_MODE_X, 0, enabled ? CLOCK_MODE_SOFTWARE : CLOCK_MODE_HARDWARE);
  }

  inline void clearCurrent() {
    if (viewedPattern) {
      memset(viewedPattern->state, 0, sizeof(viewedPattern->state));
    } else {
      memset(songPattern.state, 0, sizeof(songPattern.state));
    }
    dirtyColumns = 0xFFFF;
  }

  inline void toggleClockDirection() {
    direction = -direction;
    Hardware::setPixel(DIRECTION_X, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);
  }

  void updateSongLCDInfo() {
    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Song: Pattern ");
    Hardware::lcd.print(getSongState(songCursorX) + 1);
    Hardware::lcd.print(" ");
  }

  inline void playSong() {
    playedSongPreviously = true;
    Hardware::setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    Hardware::setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);

    songCursorX = direction < 0 ? songPattern.length - 1 : 0;
    playingPattern = &patterns[getSongState(songCursorX)];
    cursorX = direction < 0 ? playingPattern->length - 1 : 0;

    if (!viewedPattern)
      redrawColumn(songCursorX);

    updateSongLCDInfo();
  }

  inline void playPattern() {
    if (!viewedPattern) {
      playSong();
      return;
    }

    playedSongPreviously = false;
    Hardware::setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    Hardware::setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);

    playingPattern = viewedPattern;
    cursorX = direction < 0 ? playingPattern->length - 1 : 0;
    redrawColumn(cursorX);

    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Pattern ");
    Hardware::lcd.print(viewedPatternIdx + 1);
    Hardware::lcd.print("      ");
  }

  inline void beginTapTempo() {
    tapCount = 0;
  }

  inline void clockPress() {
    // Serial.println("clockPress()");
    if (!Hardware::isSoftwareClockEnabled()) {
      onClockRising();
      return;
    }

    if (tapCount < 0) {
      // Serial.println("tapCount < 0");
      return;
    }

    uint64_t time = millis();
    if (tapCount == 0) {
      prevTapTime = time;
      tapCount++;
      // Serial.print("tapCount == 0: ");
      // Serial.print(prevTapTime);
      // Serial.print(" ");
      // Serial.println(tapCount);
      return;
    }

    uint64_t dur = time - prevTapTime;
    prevTapTime = time;
    // Serial.print("Duration: ");
    // Serial.print(dur);
    // Serial.print(" ");
    // Serial.print(time);
    // Serial.print(" ");
    // Serial.print(prevTapTime);
    // Serial.print(" ");
    // Serial.println(tapCount);

    if (tapCount >= 1 && tapCount <= MAX_TEMPO_TAPS) {
      tapDurations[tapCount - 1] = dur;
      // Serial.print("Put into ");
      // Serial.println(tapCount - 1);
    } else {
      // Move all the durations back one spot
      memmove(&tapDurations[0], &tapDurations[1], (MAX_TEMPO_TAPS - 1) * sizeof(uint64_t));
      tapDurations[MAX_TEMPO_TAPS - 1] = dur;

      // Serial.print("Moved back, put into ");
      // Serial.println(MAX_TEMPO_TAPS - 1);
    }
    tapCount++;

    // Serial.print("tapDurations = ");
    // for (uint8_t i = 0; i < MAX_TEMPO_TAPS; i++) {
    //   Serial.print(tapDurations[i]);
    //   Serial.print(" ");
    // }
    // Serial.println();

    uint64_t avg = 0;
    uint8_t count = min(tapCount - 1, MAX_TEMPO_TAPS);
    // Serial.print("Count: ");
    // Serial.println(count);
    for (uint8_t i = 0; i < count; i++)
      avg += tapDurations[i];
    // Serial.print("Total: ");
    // Serial.println(avg);
    avg /= count;
    // Serial.print("Avg: ");
    // Serial.println(avg);

    Hardware::setClockInterval(avg);
    updateTempoLCDInfo();
  }

  inline void endTapTempo() {
    if (tapCount == 0) {
      toggleClockMode();
    }
    tapCount = -1;
  }

  inline void switchToPatternButton(int index) {
    heldPatterns |= (1 << index);

    uint8_t heldIndex;
    for (heldIndex = 0; heldIndex < PATTERN_COUNT; heldIndex++) {
      if (heldIndex == index)
        continue;
      if (heldPatterns & (1 << heldIndex))
        break;
    }

    if (heldIndex < PATTERN_COUNT) {
      // Copy from held pattern to new pattern
      Pattern *from = &patterns[heldIndex];
      patterns[index].copyFrom(from);
    }

    switchToPattern(index);
  }

  inline void controlRow(uint8_t x) {
    switch (x) {
      case CLOCK_X:        clockPress(); break;
      case CLOCK_MODE_X:   beginTapTempo(); break;
      case SETTINGS_X:     settingsMenuOpen = true; dirtyColumns = 0xFFFF; break;
      case CLEAR_X:        clearCurrent(); break;
      case SONG_X:         switchToSong(); songHeld = true; break;
      case DIRECTION_X:    toggleClockDirection(); break;
      case PLAY_SONG_X:    if (playingPattern) stopPlaying(); else playSong(); break;
      case PLAY_PATTERN_X: if (playingPattern) stopPlaying(); else playPattern(); break;
      case RESET_X:        onReset(); break;
      default:             switchToPatternButton(x - PATTERNS_START_X); break;
    }
    Hardware::updateTrellis();
  }

  uint8_t whichPattern = 0;
  void onButtonPress(uint8_t x, uint8_t y) {
    uint8_t patternX = PIXEL_TO_PATTERN(x);
    if (y == 0) {
      controlRow(x);
      return;
    } else if (settingsMenuOpen) {
      gateMask ^= 1 << y;
      dirtyColumns = 0xFFFF;
    } else if (!viewedPattern) {
      if (patternX < songPattern.length) {
        uint8_t col = patternX >> 1;
        if (patternX & 1)
          songPattern.state[col] = (songPattern.state[col] & 0xF0) | (y - 1);
        else
          songPattern.state[col] = (songPattern.state[col] & 0x0F) | ((y - 1) << 4); 
        dirtyColumns |= 1 << x;
      }
    } else {
      if (patternX < viewedPattern->length) {
        viewedPattern->state[patternX] ^= 1 << y;
        dirtyColumns |= 1 << x;
      }
    }
    Hardware::updateTrellis();
  }

  void onButtonRelease(uint8_t x, uint8_t y) {
    if (y == 0) {
      if (x == SETTINGS_X) {
        settingsMenuOpen = false;
        dirtyColumns = 0xFFFF;
      }
      if (x == CLOCK_X && !Hardware::isSoftwareClockEnabled())
        onClockFalling();
      if (x == CLOCK_MODE_X)
        endTapTempo();
      if (x == SONG_X)
        songHeld = false;
    }
    Hardware::updateTrellis();

    int8_t pattern = (int8_t) x - PATTERNS_START_X;
    if (pattern >= 0 && pattern < PATTERN_COUNT) {
      heldPatterns &= ~(1 << pattern);
      // Serial.print("Held ");
      // Serial.println(heldPatterns, BIN);
    }
  }


  void tick() {
    updatePixels();

    if (popupTime && (millis() - popupTime) >= POPUP_PERSIST_TIME) {
      updateTempoLCDInfo(); // Will also cancel the popup
    }
  }


  void writeOutputs() {
    if (clockOn) {
      currentOutputs = playingPattern->state[cursorX];
      Hardware::outputTriggers(currentOutputs | 1); // Always output trigger on output 1 for clock out
    } else {
      Hardware::outputTriggers(currentOutputs & gateMask);
    }
  }

  void onClockRising() {
    Hardware::setPixel(0, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);
    Hardware::updateTrellis();

    if (!playingPattern)
      return;

    // Advance cursor and pattern position
    if (viewedPattern == playingPattern)
      redrawColumn(cursorX);
    cursorX += direction;
    if (cursorX < 0) {
      if (songCursorX >= 0) {
        if (!viewedPattern)
          redrawColumn(songCursorX);
        if (--songCursorX < 0)
          songCursorX = songPattern.length - 1;
        playingPattern = &patterns[getSongState(songCursorX)];
        if (!viewedPattern)
          redrawColumn(songCursorX);
        updateSongLCDInfo();
      }
      cursorX = playingPattern->length - 1;
    }
    if (cursorX >= playingPattern->length) {
      if (songCursorX >= 0) {
        if (!viewedPattern)
          redrawColumn(songCursorX);
        if (++songCursorX >= songPattern.length)
          songCursorX = 0;
        playingPattern = &patterns[getSongState(songCursorX)];
        if (!viewedPattern)
          redrawColumn(songCursorX);
        updateSongLCDInfo();
      }
      cursorX = 0;
    }
    if (viewedPattern == playingPattern)
      redrawColumn(cursorX);

    clockOn = true;
    writeOutputs();
  }

  void onClockFalling() {
    clockOn = false;
    writeOutputs();

    Hardware::setPixel(0, 0, CLOCK_OFF);
    Hardware::updateTrellis();
  }

  void onReset() {
    if (!playingPattern)
      return;

    if (songCursorX > 0) {
      if (!viewedPattern)
        redrawColumn(songCursorX);
      if (viewedPattern == playingPattern)
        redrawColumn(cursorX);
      songCursorX = direction < 0 ? songPattern.length - 1 : 0;
      playingPattern = &patterns[getSongState(songCursorX)];
      cursorX = direction < 0 ? playingPattern->length - 1 : 0;
      if (!viewedPattern)
        redrawColumn(songCursorX);
      if (viewedPattern == playingPattern)
        redrawColumn(cursorX);
    } else if (playingPattern) {
      if (playingPattern == viewedPattern)
        redrawColumn(cursorX);
      cursorX = direction < 0 ? playingPattern->length - 1 : 0;
      if (playingPattern == viewedPattern)
        redrawColumn(cursorX);
    }

    writeOutputs();
  }

  void lengthenPattern() {
    if (viewedPattern) {
      if (viewedPattern->length < MAX_PATTERN_LEN) {
        uint8_t col = viewedPattern->length++;
        redrawColumn(col);
        beginNewLengthPopup(col + 1);
      }
    } else {
      if (songPattern.length < MAX_PATTERN_LEN) {
        uint8_t col = songPattern.length++;
        redrawColumn(col);
        beginNewLengthPopup(col + 1);
      }
    }
  }

  uint8_t calcMaxScroll(uint8_t patternLen) {
    return max(patternLen, PANEL_WIDTH) - PANEL_WIDTH;
  }

  void shortenPattern() {
    if (viewedPattern) {
      if (viewedPattern->length > MIN_PATTERN_LEN) {
        uint8_t col = --viewedPattern->length;
        beginNewLengthPopup(col);

        uint8_t maxScroll = calcMaxScroll(col);
        if (getCurrentScroll() >= maxScroll) {
          viewedPattern->scroll = maxScroll;
          dirtyColumns = 0xFFFF;
        } else {
          redrawColumn(col);
        }
      }
    } else {
      if (songPattern.length > MIN_PATTERN_LEN) {
        uint8_t col = --songPattern.length;
        beginNewLengthPopup(col);

        uint8_t maxScroll = calcMaxScroll(col);
        if (getCurrentScroll() >= maxScroll) {
          songPattern.scroll = maxScroll;
          dirtyColumns = 0xFFFF;
        } else {
          redrawColumn(col);
        }
      }
    }
  }

  void doScroll(int16_t amount) {
    uint8_t scroll = getCurrentScroll();
    
    if (amount < 0 && scroll == 0)
      return;
    uint8_t len = viewedPattern ? viewedPattern->length : songPattern.length;
    uint8_t maxScroll = calcMaxScroll(len);
    if (amount > 0 && scroll == maxScroll)
      return;

    dirtyColumns = 0xFFFF;

    if (amount < 0 && scroll < -amount)
      scroll = 0;
    else
      scroll += amount;
    
    if (scroll > maxScroll)
      scroll = maxScroll;

    if (viewedPattern)
      viewedPattern->scroll = scroll;
    else
      songPattern.scroll = scroll;

    // if (amount < 0 && scroll == 0)
    //   return; // Can't scroll past beginning of pattern

    // uint8_t currentLen;
    // if (viewedPattern)
    //   currentLen = viewedPattern->length;
    // else
    //   currentLen = songPattern.length;

    // uint8_t maxScroll = min(currentLen, PANEL_WIDTH) - PANEL_WIDTH;
    // if (amount > 0 && scroll == maxScroll)
    //   return; // Can't scroll past end of pattern

    // // Redraw everything since the view is changing
    // dirtyColumns = 0xFFFF;

    // // Move the scroll by amount, while limiting to keep pattern on screen
    // int16_t unclamped = (int16_t) scroll + amount;
    // uint8_t clamped = (uint8_t) constrain(unclamped, 0, maxScroll);
    // scroll = clamped;
  }

  void onEncoderTurn(Hardware::Encoder encoder, int16_t movement) {
    if (encoder == Hardware::Encoder::LEFT) {
      uint16_t tempo = Hardware::getClockBPM();
      tempo += movement * TEMPO_STEP;
      Hardware::setClockBPM(tempo);
      updateTempoLCDInfo();
    } else {
      if (rightEncoderPressed) {
        while (movement != 0) {
          if (movement > 0) {
            lengthenPattern();
            movement--;            
          } else {
            shortenPattern();
            movement++;
          }
        }
      } else if (viewedPattern && (heldPatterns & (1 << viewedPatternIdx))) {
        while (movement != 0) {
          if (movement > 0) {
            viewedPattern->rotateRight();
            movement--;
          } else {
            viewedPattern->rotateLeft();
            movement++;
          }
        }
        dirtyColumns = 0xFFFF;
      } else if (!viewedPattern && songHeld) {
        while (movement != 0) {
          if (movement > 0) {
            songPattern.rotateRight();
            movement--;
          } else {
            songPattern.rotateLeft();
            movement++;
          }
          dirtyColumns = 0xFFFF;
        }
      } else {
        doScroll(-movement);
      }
    }
  }

  void onEncoderPress(Hardware::Encoder encoder) {
    if (encoder == Hardware::Encoder::LEFT) {
      if (!playingPattern) {
        if (playedSongPreviously)
          playSong();
        else
          playPattern();
      } else {
        stopPlaying();
      }
    } else {
      rightEncoderPressed = true;
    }
  }

  void onEncoderRelease(Hardware::Encoder encoder) {
    if (encoder == Hardware::Encoder::RIGHT) {
      rightEncoderPressed = false;
    }
  }
}
