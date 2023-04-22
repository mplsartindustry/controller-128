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

namespace Controller {
  enum PlayMode : uint8_t {
    OFF = 0,
    SONG = 1,
    PATTERN = 2
  };

  struct Pattern {
    uint32_t blank;
    uint32_t active;

    uint8_t state[MAX_PATTERN_LEN];
    uint8_t length = DEFAULT_PATTERN_LEN;

    Pattern(uint32_t blank_, uint32_t active_):
        blank(blank_), active(active_) {
      memset(state, 0, sizeof(state));
    }
  };

  static const uint32_t CLOCK_FORWARD = COLOR(0, 30, 0);
  static const uint32_t CLOCK_BACKWARD = COLOR(30, 15, 0);
  static const uint32_t CLOCK_OFF = COLOR(0, 0, 0);

  static const uint32_t CLOCK_MODE_SOFTWARE = COLOR(30, 30, 0);
  static const uint32_t CLOCK_MODE_HARDWARE = COLOR(30, 15, 0);

  static const uint32_t SONG_ACTIVE = COLOR(30, 30, 30);
  static const uint32_t SONG_BLANK = COLOR(3, 3, 3);

  static const uint32_t PLAY_STOP = COLOR(15, 0, 0);

  static const uint32_t CURSOR = COLOR(15, 15, 15);
  static const uint32_t OUT_OF_BOUNDS = COLOR(0, 0, 0);

  Pattern patterns[PATTERN_COUNT] {
    Pattern(COLOR(3, 0, 0), COLOR(30,  0,  0)),
    Pattern(COLOR(3, 2, 0), COLOR(30, 15,  0)),
    Pattern(COLOR(3, 3, 0), COLOR(30, 30,  0)),
    Pattern(COLOR(0, 3, 0), COLOR( 0, 30,  0)),
    Pattern(COLOR(0, 3, 3), COLOR( 0, 30, 30)),
    Pattern(COLOR(0, 0, 3), COLOR( 0,  0, 30)),
    Pattern(COLOR(2, 0, 3), COLOR(15,  0, 30))
  };
  uint8_t songState[MAX_PATTERN_LEN >> 1];
  uint8_t songLen = DEFAULT_PATTERN_LEN;

  uint8_t patternIdx = 0;
  Pattern* pattern = nullptr;
  int8_t scroll = 0;

  bool rightEncoderPressed = false;
  int8_t direction = 1;

  PlayMode playMode;
  PlayMode prevPlayMode;

  int8_t songCursor = 0, patternCursor = 0;
  Pattern* playingPattern = nullptr;
  uint8_t playingPatternIdx = 0;

  #define COLUMN_TO_PATTERN(x) (x + scroll)

  void drawInitialControlRow() {
    Hardware::setPixel(CLOCK_MODE_X, 0, CLOCK_MODE_SOFTWARE);
    Hardware::setPixel(DIRECTION_X, 0, CLOCK_FORWARD);

    for (uint8_t x = 0; x < PATTERN_COUNT; x++) {
      Hardware::setPixel(x + PATTERNS_START_X, 0, patterns[x].blank);
    }
  }

  void clampScroll(uint8_t len) {
    if (len - scroll < PANEL_WIDTH)
      scroll = len - PANEL_WIDTH;
    if (scroll < 0)
      scroll = 0;
  }

  void updateTempoLCDInfo() {
    Hardware::lcd.setCursor(0, 1);
    Hardware::lcd.print("Tempo: ");
    Hardware::lcd.print(Hardware::getClockBPM());
    Hardware::lcd.print(" BPM   ");
  }

  void updateSongLCDInfo() {
    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Song: Pattern ");
    Hardware::lcd.print(playingPatternIdx + 1);
    Hardware::lcd.print(" ");    
  }  

  void drawPatternColumn(uint8_t x, bool showCursor) {
    uint8_t col = COLUMN_TO_PATTERN(x);
    if (col < pattern->length) {
      bool cursor = showCursor
          && playMode != PlayMode::OFF
          && playingPatternIdx == patternIdx
          && patternCursor == col;

      uint8_t state = pattern->state[col];
      for (uint8_t y = 1; y < PANEL_HEIGHT; y++) {
        uint32_t c = (state & (1 << y)) ? pattern->active : (cursor ? CURSOR : pattern->blank);
        Hardware::setPixel(x, y, c);
      }      
    } else {
      for (uint8_t y = 1; y < PANEL_HEIGHT; y++) {
        Hardware::setPixel(x, y, OUT_OF_BOUNDS);
      }
    }
  }

  void updateColumn(uint8_t idx, bool cursor) {
    if (pattern && playingPatternIdx == patternIdx) {
      int8_t col = idx - scroll;

      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, cursor);      
    }
  }

  void switchToPattern(uint8_t index) {
    if (!pattern)
      Hardware::setPixel(SONG_X, 0, SONG_BLANK);
    else {
      if (patternIdx == index)
        return;
      
      Hardware::setPixel(PATTERNS_START_X + patternIdx, 0, pattern->blank);
    }

    pattern = &patterns[index];
    patternIdx = index;

    Hardware::setPixel(PATTERNS_START_X + index, 0, pattern->active);
    if (playMode == PlayMode::OFF)
      Hardware::setPixel(PLAY_PATTERN_X, 0, pattern->active);

    clampScroll(pattern->length);
    for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
      drawPatternColumn(x, true);
    }
  }

  void drawSongColumn(uint8_t x, bool showCursor) {
    uint8_t col = COLUMN_TO_PATTERN(x);
    if (col < songLen) {
      bool cursor = showCursor
          && playMode != PlayMode::OFF
          && !pattern
          && songCursor == col;

      for (uint8_t y = 1; y < PANEL_HEIGHT; y++) {
        uint8_t state;
        if (col & 1)
          state = songState[col] & 0xF;
        else
          state = songState[col] >> 4;
        
        Pattern* rowPattern = &patterns[y - 1];
        uint32_t c = (state == y - 1) ? rowPattern->active : (cursor ? CURSOR : rowPattern->blank);

        Hardware::setPixel(x, y, c);
      }
    } else {
      for (uint8_t y = 1; y < PANEL_HEIGHT; y++)
        Hardware::setPixel(x, y, OUT_OF_BOUNDS);
    }
  }

  void updateSongColumn(uint8_t idx, bool cursor) {
    if (!pattern) {
      int8_t col = idx - scroll;
      if (col >= 0 && col < songLen)
        drawSongColumn(col, cursor);
    }
  }

  void switchToSong() {
    if (!pattern) return;
    Hardware::setPixel(PATTERNS_START_X + patternIdx, 0, pattern->blank);

    if (playMode == PlayMode::OFF)
      Hardware::setPixel(PLAY_PATTERN_X, 0, SONG_ACTIVE);

    pattern = nullptr;
    Hardware::setPixel(SONG_X, 0, SONG_ACTIVE);

    clampScroll(songLen);
    for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
      drawSongColumn(x, true);
    }
  }

  uint8_t getSongState(uint8_t col) {
    if (col & 1)
      return songState[col] & 0xF;
    else
      return songState[col] >> 4;
  }

  void playSong() {
    Hardware::setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    Hardware::setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);

    playMode = PlayMode::SONG;
    prevPlayMode = PlayMode::SONG;

    playingPatternIdx = getSongState(0);
    playingPattern = &patterns[playingPatternIdx];

    songCursor = 0;
    patternCursor = 0;

    updateSongLCDInfo();
    updateColumn(0, true);
    updateSongColumn(0, true);    
  }

  void playPattern() {
    if (!pattern) {
      playSong();
      return;
    }

    Hardware::setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    Hardware::setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);

    playMode = PlayMode::PATTERN;
    prevPlayMode = PlayMode::PATTERN;

    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Pattern ");
    Hardware::lcd.print(patternIdx + 1);
    Hardware::lcd.print("      ");

    playingPattern = pattern;
    playingPatternIdx = patternIdx;

    patternCursor = 0;

    updateColumn(0, true);
  }

  inline uint32_t currentPatternActive() {
    if (pattern)
      return pattern->active;
    else
      return SONG_ACTIVE;    
  }

  void stopPlaying() {
    Hardware::setPixel(PLAY_SONG_X, 0, SONG_ACTIVE);
    Hardware::setPixel(PLAY_PATTERN_X, 0, currentPatternActive());

    playMode = PlayMode::OFF;

    Hardware::lcd.setCursor(0, 0);
    Hardware::lcd.print("Idle            ");

    updateColumn(patternCursor, false);
    updateSongColumn(songCursor, false);    
  }

  void init() {
    playMode = PlayMode::OFF;
    prevPlayMode = PlayMode::PATTERN;

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

  inline void toggleClockDirection() {
    direction = -direction;
    Hardware::setPixel(DIRECTION_X, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);
  }

  inline void clearCurrent() {
    if (pattern) {
      memset(pattern->state, 0, sizeof(pattern->state));
      for (uint8_t x = 0; x < PANEL_WIDTH; x++)
        drawPatternColumn(x, true);
    } else {
      memset(songState, 0, sizeof(songState));
      for (uint8_t x = 0; x < PANEL_WIDTH; x++)
        drawSongColumn(x, true);
    }
  }

  inline void reset() {
    // TODO
  }

  inline void controlRow(uint8_t x) {
    switch (x) {
      case CLOCK_X:        if (!Hardware::isSoftwareClockEnabled()) onClockRising(); break;
      case CLOCK_MODE_X:   toggleClockMode(); break;
      case SETTINGS_X:     /* TODO */ break;
      case CLEAR_X:        clearCurrent(); break;
      case SONG_X:         switchToSong(); break;
      case DIRECTION_X:    toggleClockDirection(); break;
      case PLAY_SONG_X:    if (playMode != PlayMode::OFF) stopPlaying(); else playSong(); break;
      case PLAY_PATTERN_X: if (playMode != PlayMode::OFF) stopPlaying(); else playPattern(); break;
      case RESET_X:        reset(); break;
      default:             switchToPattern(x - PATTERNS_START_X); break;
    }
  }

  inline void songSet(uint8_t x, uint8_t y) {
    uint8_t col = COLUMN_TO_PATTERN(x);
    if (col >= songLen)
      return;

    uint8_t state = getSongState(col);

    Hardware::setPixel(x, state + 1, patterns[state].blank);
    Hardware::setPixel(x, y, patterns[y - 1].active);

    if (col & 1)
      songState[col] = (songState[col] & 0xF0) | (y - 1);
    else
      songState[col] = (songState[col] & 0x0F) | ((y - 1) << 4);    
  }

  inline void patternSet(uint8_t x, uint8_t y) {
    uint8_t col = COLUMN_TO_PATTERN(x);
    if (col >= pattern->length)
      return;

    uint8_t mask = 1 << y;
    pattern->state[col] ^= mask;

    Hardware::setPixel(x, y, (pattern->state[col] & mask) ? pattern->active : pattern->blank);
  }

  void onButtonPress(uint8_t x, uint8_t y) {
    if (y == 0) {
      controlRow(x);
    } else if (!pattern) {
      songSet(x, y);
    } else {
      patternSet(x, y);
    }
    Hardware::updateTrellis();
  }

  void onButtonRelease(uint8_t x, uint8_t y) {
    if (y == 0 && x == CLOCK_X && !Hardware::isSoftwareClockEnabled())
      onClockFalling();
  }

  void onClockRising() {
    Hardware::setPixel(CLOCK_X, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);

    if (playMode != PlayMode::OFF) {
      updateColumn(patternCursor, false);
      patternCursor += direction;
      if (patternCursor >= playingPattern->length) {
        patternCursor = 0;
        if (playMode == PlayMode::SONG) {
          updateSongColumn(songCursor, false);
          songCursor++;
          if (songCursor >= songLen)
            songCursor = 0;

          playingPatternIdx = getSongState(songCursor);
          playingPattern = &patterns[playingPatternIdx];
          updateSongColumn(songCursor, true);
          updateSongLCDInfo();
        }
      } else if (patternCursor < 0) {
        patternCursor = playingPattern->length - 1;
        if (playMode == PlayMode::SONG) {
          updateSongColumn(songCursor, false);
          songCursor--;
          if (songCursor < 0)
            songCursor = songLen - 1;

          playingPatternIdx = getSongState(songCursor);
          playingPattern = &patterns[playingPatternIdx];
          updateSongColumn(songCursor, true);
          updateSongLCDInfo();
        }
      }
      updateColumn(patternCursor, true);
    }

    Hardware::outputTriggers(playingPattern->state[patternCursor] | 1); // Add 1 bit for clock out
    Hardware::updateTrellis();
  }

  void onClockFalling() {
    Hardware::setPixel(CLOCK_X, 0, CLOCK_OFF);
    Hardware::updateTrellis();

    Hardware::outputTriggers(0);
  }

  void onReset() {
    reset();
  }

  void shortenPattern() {
    if (!pattern) {
      if (songLen <= MIN_PATTERN_LEN)
        return;
      
      songLen--;
      int8_t col = songLen - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawSongColumn(col, true);
    } else {
      if (pattern->length <= MIN_PATTERN_LEN)
        return;
      
      pattern->length--;
      int8_t col = pattern->length - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, true);
    }
  }
  
  void lengthenPattern() {
    if (!pattern) {
      if (songLen >= MAX_PATTERN_LEN)
        return;
      
      songLen++;
      int8_t col = songLen - 1 - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawSongColumn(col, true);
    } else {
      if (pattern->length >= MAX_PATTERN_LEN)
        return;
      
      pattern->length++;
      int8_t col = pattern->length - 1 - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, true);
    }
  }

  void doScroll(uint8_t dir) {
    scroll += dir;
   
    if (!pattern) {
      clampScroll(songLen);
      for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
        drawSongColumn(x, true);
      }
    } else {
      clampScroll(pattern->length);
      for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
        drawPatternColumn(x, true);
      }
    }
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
      } else {
        doScroll(movement);
      }
    }
    Hardware::updateTrellis();
  }

  void onEncoderPress(Hardware::Encoder encoder) {
    if (encoder == Hardware::Encoder::LEFT) {
      if (playMode == PlayMode::OFF) {
        if (prevPlayMode == PlayMode::SONG)
          playSong();
        else
          playPattern();
      } else {
        stopPlaying();
      }
    } else {
      rightEncoderPressed = true;
    }
    Hardware::updateTrellis();
  }

  void onEncoderRelease(Hardware::Encoder encoder) {
    if (encoder == Hardware::Encoder::RIGHT) {
      rightEncoderPressed = false;
    }
  }
}

// #define STATE_WIDTH (TRELLIS_WIDTH*2)
// #define STATE_HEIGHT TRELLIS_HEIGHT

// #define CLOCK_ADJUST_STEP 1
// #define MAX_BPM 100

// namespace Controller {
//   static const uint32_t blank  = COLOR( 0,  0,  0);
//   static const uint32_t active = COLOR(30, 15,  0);
//   static const uint32_t cursor = COLOR(30, 30, 30);
//   static const uint32_t outOfBounds = COLOR(10, 0, 0);

//   uint8_t displayState[STATE_WIDTH];

//   bool playing = false;
//   int cursorX = 0;

//   int scrollX = 0;
//   int scrollY = 0;
//   int scrollXLimit = 16;
//   int scrollYLimit = 0;
//   int patternLength = 16;

//   bool getState(uint8_t x, uint8_t y) {
//     uint8_t mask = 1 << y;
//     return (displayState[x] & mask) != 0;
//   }

//   void setState(uint8_t x, uint8_t y, bool b) {
//     uint8_t mask = 1 << y;

//     if (b) {
//       displayState[x] |= mask;
//     } else {
//       displayState[x] &= ~mask;
//     }
//   }

//   void clearDisplay() {
//     // Clear state
//     memset(displayState, 0, sizeof(displayState));

//     // Clear display
//     for (uint8_t x = 0; x < TRELLIS_WIDTH; x++) {
//       for (uint8_t y = 1; y < TRELLIS_HEIGHT; y++) {
//         Hardware::setPixel(x, y, playing && x == cursorX - scrollX ? cursor : blank);
//       }
//     }
//   }

//   void drawOutOfBounds() {
//     int oobBegin = patternLength - scrollX;
//     for (int x = oobBegin; x < TRELLIS_WIDTH; x++) {
//       for (int y = 1; y < TRELLIS_HEIGHT; y++) {
//         Hardware::setPixel(x, y, outOfBounds);
//       }
//     }
//   }

//   void eraseCursor() {
//     int x = cursorX - scrollX;
//     for (int y = 1; y < TRELLIS_HEIGHT; y++) {
//       Hardware::setPixel(x, y, getState(cursorX, y + scrollY) ? active : blank);
//     }
//   }

//   void drawCursor() {
//     int x = cursorX - scrollX;
//     for (int y = 1; y < TRELLIS_HEIGHT; y++) {
//       Hardware::setPixel(x, y, getState(cursorX, y + scrollY) ? active : cursor);
//     }
//   }

//   void showBPM() {
//     uint16_t bpm = Hardware::getClockBPM();
//     Hardware::lcd.setCursor(0, 1);
//     Hardware::lcd.print(bpm);
//     Hardware::lcd.print(" bpm, ");
//     Hardware::lcd.print("__% sw     ");
//   }

//   void playPause() {
//     if (playing) {
//       playing = false;
//       Hardware::setPixel(0, 0, blank);
//       eraseCursor();
//       Hardware::updateTrellis();
//     } else {
//       playing = true;
//       cursorX = 0;
//       Hardware::setPixel(0, 0, active);
//       drawCursor();
//       Hardware::updateTrellis();
//     }
//   }

//   void resetCursor() {
//     if (playing) {
//       eraseCursor();
//       cursorX = 0;
//       drawCursor();
//       Hardware::updateTrellis();
//     }
//   }

//   void clearState() {
//     clearDisplay();
//     drawOutOfBounds();
//     Hardware::updateTrellis();
//   }

//   void clockUp() {
//     uint16_t bpm = Hardware::getClockBPM() + CLOCK_ADJUST_STEP;
//     if (bpm > MAX_BPM) bpm = MAX_BPM;
//     Hardware::setClockBPM(bpm);
//     showBPM();
//   }

//   void clockDown() {
//     uint16_t bpm = Hardware::getClockBPM() - CLOCK_ADJUST_STEP;
//     if (bpm > MAX_BPM) bpm = 0; // Wrapped around to negative
//     Hardware::setClockBPM(bpm);
//     showBPM();
//   }
  
//   void init() {
//     clearDisplay();
//     drawOutOfBounds();
//     showBPM();

//     Hardware::updateTrellis();
//     Hardware::lcd.setCursor(0, 0);
//     Hardware::lcd.print("Running         ");
//   }

//   void onButtonPress(uint8_t x, uint8_t y) {
//     if (y == 0) {
//       switch (x) {
//         case 0:  playPause();   break;
//         case 1:  resetCursor(); break;
//         case 2:  clearState();  break;
//         case 13: clockDown();   break;
//         case 14: clockUp();     break;
//       }
//     } else {
//       if (x + scrollX < patternLength) {
//         bool state = !getState(x + scrollX, y + scrollY);
//         setState(x + scrollX, y + scrollY, state);
//         Hardware::setPixel(x, y, state ? active : blank);
//       }
//     }
//     Hardware::updateTrellis();
//   }

//   void onButtonRelease(uint8_t x, uint8_t y) {
    
//   }

//   void onClockRising() {
//     if (playing) {
//       eraseCursor();
//       cursorX++;
//       if (cursorX >= patternLength) cursorX = 0;
//       drawCursor();

//       uint8_t triggers = displayState[cursorX];
//       Hardware::outputTriggers(triggers);
//     }

//     Hardware::setPixel(15, 0, active);

//     Hardware::updateTrellis();
//   }

//   void onClockFalling() {
//     Hardware::outputTriggers(0);

//     Hardware::setPixel(15, 0, blank);

//     Hardware::updateTrellis();
//   }

//   void onReset() {
//     resetCursor();
//   }

//   void onEncoderTurn(Hardware::Encoder encoder, int16_t movement) {
//     while (movement != 0) {
//       if (movement > 0) {
//         clockUp();
//         movement--;
//       }
//       if (movement < 0) {
//         clockDown();
//         movement++;
//       }
//     }
//   }

//   void onEncoderPress(Hardware::Encoder encoder) {
//     playPause();
//   }

//   void onEncoderRelease(Hardware::Encoder encoder) {
    
//   }
// }
