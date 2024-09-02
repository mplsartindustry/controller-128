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

import themidibus.*;
import java.awt.Color;

import java.util.Arrays;

// Simulates controller_128 software

enum PlayMode {
  SONG,
  PATTERN,
  OFF
}

class RyanSimulator2 extends Simulator {
  class Pattern {
    final color blank;
    final color active;
    
    final byte[] state = new byte[MAX_PATTERN_LEN];
    int length;
    
    Pattern(color blank, color active) {
      this.blank = blank;
      this.active = active;
      
      length = DEFAULT_PATTERN_LEN;
      
      // memset(state, 0, sizeof(state));
    }
  }
  
  final int PANEL_WIDTH = 16;
  final int PANEL_HEIGHT = 8;
  
  final int CLOCK_X = 0;
  final int CLOCK_MODE_X = 1;
  final int SETTINGS_X = 2;
  final int CLEAR_X = 3;
  final int SONG_X = 4;
  final int PATTERNS_START_X = 5;
  final int DIRECTION_X = 12;
  final int PLAY_SONG_X = 13;
  final int PLAY_PATTERN_X = 14;
  final int RESET_X = 15;
  
  final int PATTERN_COUNT = 7;
  
  final int MIN_PATTERN_LEN = 2;
  final int DEFAULT_PATTERN_LEN = 16;
  final int MAX_PATTERN_LEN = 32;
  
  final int TEMPO_STEP = 2;
  
  final color CLOCK_FORWARD = color(0, 255, 0);
  final color CLOCK_BACKWARD = color(255, 180, 0);
  final color CLOCK_OFF = color(0);
  
  final color CLOCK_MODE_SOFTWARE = color(255, 255, 0);
  final color CLOCK_MODE_HARDWARE = color(255, 180, 0);
  
  final color SONG_ACTIVE = color(255);
  final color SONG_BLANK = color(128);
  
  final color PLAY_STOP = color(128, 0, 0);
  
  final color CURSOR = color(128);
  final color OUT_OF_BOUNDS = color(0);
  
  final Pattern[] patterns = {
    new Pattern(color(128,   0,   0), color(255,   0,   0)),
    new Pattern(color(128,  90,   0), color(255, 180,   0)),
    new Pattern(color(128, 128,   0), color(255, 255,   0)),
    new Pattern(color(  0, 128,   0), color(  0, 255,   0)),
    new Pattern(color(  0, 128, 128), color(  0, 255, 255)),
    new Pattern(color(  0,   0, 128), color(  0,   0, 255)),
    new Pattern(color( 90,   0, 128), color(180,   0, 255))
  };
  final byte[] songState = new byte[MAX_PATTERN_LEN >> 1];
  int songLen = DEFAULT_PATTERN_LEN;
  
  int patternIdx = 0;
  Pattern pattern = null;
  int scroll = 0;
  
  boolean rightEncoderPressed = false;
  int direction = 1;
  
  PlayMode playMode;
  PlayMode prevPlayMode;
  
  int songCursor = 0, patternCursor = 0;
  Pattern playingPattern = null;
  int playingPatternIdx = 0;
  
  public RyanSimulator2() {
    softwareClockEnabled = true;
    direction = 1;
    
    playMode = PlayMode.OFF;
    prevPlayMode = PlayMode.PATTERN;
    
    drawInitialControlRow();
    switchToPattern(0);
    
    stopPlaying();
    
    updateTempoLCDInfo();
  }
  
  private void drawInitialControlRow() {
    setPixel(CLOCK_MODE_X, 0, CLOCK_MODE_SOFTWARE);
    setPixel(DIRECTION_X, 0, CLOCK_FORWARD);
    
    for (int x = 0; x < PATTERN_COUNT; x++) {
      setPixel(x + PATTERNS_START_X, 0, patterns[x].blank);
    }
  }
  
  private int COLUMN_TO_PATTERN(int x) {
    return x + scroll;
  }
  
  private void clampScroll(int len) {
    if (len - scroll < PANEL_WIDTH) {
      scroll = len - PANEL_WIDTH;
    }
    if (scroll < 0)
      scroll = 0;
  }
  
  private void drawPatternColumn(int x, boolean showCursor) {
    int col = COLUMN_TO_PATTERN(x);
    if (col < pattern.length) {
      boolean cursor = showCursor && playMode != PlayMode.OFF && playingPatternIdx == patternIdx && patternCursor == col;
      byte state = pattern.state[col];
      for (int y = 1; y < PANEL_HEIGHT; y++) {
        color c = (state & (1 << y)) != 0 ? pattern.active : (cursor ? CURSOR : pattern.blank);
        setPixel(x, y, c);
      }
    } else {
      for (int y = 1; y < PANEL_HEIGHT; y++)
        setPixel(x, y, OUT_OF_BOUNDS);
    }
  }
  
  private void switchToPattern(int index) {
    if (pattern == null)
      setPixel(SONG_X, 0, SONG_BLANK);
    else {
      if (patternIdx == index)
        return; // Already in this pattern
      
      setPixel(PATTERNS_START_X + patternIdx, 0, pattern.blank);
    }
    
    pattern = patterns[index];
    patternIdx = index;
    
    setPixel(PATTERNS_START_X + index, 0, pattern.active);
    if (playMode == PlayMode.OFF)
      setPixel(PLAY_PATTERN_X, 0, pattern.active);
    
    clampScroll(pattern.length);
    for (int x = 0; x < PANEL_WIDTH; x++) {
      drawPatternColumn(x, true);
    }
  }
  
  private void drawSongColumn(int x, boolean showCursor) {
    int col = COLUMN_TO_PATTERN(x);
    if (col < songLen) {
      boolean cursor = showCursor && playMode != PlayMode.OFF && pattern == null && songCursor == col;

      for (int y = 1; y < PANEL_HEIGHT; y++) {
        int state;
        if ((col & 1) == 0)
          state = songState[col] & 0xF;
        else
          state = songState[col] >>> 4;
          
        Pattern rowPattern = patterns[y - 1];
        color c = (state == y - 1) ? rowPattern.active : (cursor ? CURSOR : rowPattern.blank);
        
        setPixel(x, y, c);
      }
    } else {
      for (int y = 1; y < PANEL_HEIGHT; y++)
        setPixel(x, y, OUT_OF_BOUNDS);
    }
  }
  
  private void switchToSong() {
    if (pattern == null)
      return; // Already in song mode
    else
      setPixel(PATTERNS_START_X + patternIdx, 0, pattern.blank);
      
    if (playMode == PlayMode.OFF) {
      setPixel(PLAY_PATTERN_X, 0, SONG_ACTIVE);
    }
      
    pattern = null;
    setPixel(SONG_X, 0, SONG_ACTIVE);
    
    clampScroll(songLen);
    for (int x = 0; x < PANEL_WIDTH; x++) {
      drawSongColumn(x, true);
    }
  }
  
  private /*inline*/ void toggleClockMode() {
    softwareClockEnabled = !softwareClockEnabled;
    setPixel(CLOCK_MODE_X, 0, softwareClockEnabled ? CLOCK_MODE_SOFTWARE : CLOCK_MODE_HARDWARE);
  }
  
  private /*inline*/ void toggleClockDirection() {
    direction = -direction;
    setPixel(DIRECTION_X, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);
  }
  
  private /*inline*/ void clearCurrent() {
    if (pattern == null) {
      Arrays.fill(songState, (byte) 0);
      for (int x = 0; x < PANEL_WIDTH; x++) {
        drawSongColumn(x, true);
      }
    } else {
      Arrays.fill(pattern.state, (byte) 0);
      for (int x = 0; x < PANEL_WIDTH; x++) {
        drawPatternColumn(x, true);
      }
    }
  }
  
  private void updateSongLCDInfo() {
    lcd.setCursor(0, 0);
    lcd.print("Song: Pattern " + (playingPatternIdx + 1) + " ");
  }
  
  private void updateTempoLCDInfo() {
    lcd.setCursor(0, 1);
    lcd.print("Tempo: " + tempoBPM + " BPM   ");
  }
  
  private void playSong() {
    setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);
    
    playMode = PlayMode.SONG;
    prevPlayMode = PlayMode.SONG;
    
    playingPatternIdx = getSongState(0);
    playingPattern = patterns[playingPatternIdx];
    
    songCursor = 0;
    patternCursor = 0;
    
    updateSongLCDInfo();
    updateColumn(0, true);
    updateSongColumn(0, true);
  }
  
  private void playPattern() {
    if (pattern == null) {
      playSong();
      return;
    }
    
    setPixel(PLAY_SONG_X, 0, PLAY_STOP);
    setPixel(PLAY_PATTERN_X, 0, PLAY_STOP);
    
    playMode = PlayMode.PATTERN;
    prevPlayMode = PlayMode.PATTERN;
    
    lcd.setCursor(0, 0);
    lcd.print("Pattern " + (patternIdx + 1) + "       ");
    
    playingPattern = pattern;
    playingPatternIdx = patternIdx;
    
    patternCursor = 0;
    
    updateColumn(0, true);
  }
  
  private color currentPatternActive() {
    if (pattern != null)
      return pattern.active;
    else
      return SONG_ACTIVE;
  }
  
  private void stopPlaying() {
    setPixel(PLAY_SONG_X, 0, SONG_ACTIVE);
    setPixel(PLAY_PATTERN_X, 0, currentPatternActive());
    
    playMode = PlayMode.OFF;
    
    lcd.setCursor(0, 0);
    lcd.print("Idle            ");
    
    updateColumn(patternCursor, false);
    updateSongColumn(songCursor, false);
  }
  
  private /*inline*/ void reset() {
    if (playMode == PlayMode.OFF)
      return;
  }
  
  private /*inline*/ void controlRow(int x) {
    switch (x) {
      case CLOCK_X:        if (!softwareClockEnabled) onClockRising(); break;
      case CLOCK_MODE_X:   toggleClockMode(); break;
      case SETTINGS_X:     break;
      case CLEAR_X:        clearCurrent(); break;
      case SONG_X:         switchToSong(); break;
      case DIRECTION_X:    toggleClockDirection(); break;
      case PLAY_SONG_X:    if (playMode != PlayMode.OFF) stopPlaying(); else playSong(); break;
      case PLAY_PATTERN_X: if (playMode != PlayMode.OFF) stopPlaying(); else playPattern(); break;
      case RESET_X:        reset(); break;
      default:             switchToPattern(x - PATTERNS_START_X); break;
    }
  }
  
  private int getSongState(int col) {
    int state;
    if ((col & 1) == 0)
      state = songState[col] & 0xF;
    else
      state = songState[col] >>> 4;
    return state;
  }
  
  private /*inline*/ void songSet(int x, int y) {
    int col = COLUMN_TO_PATTERN(x);
    if (col >= songLen)
      return;
      
    int state = getSongState(col);
      
    setPixel(x, state + 1, patterns[state].blank);
    setPixel(x, y, patterns[y - 1].active);
    
    if ((col & 1) == 0)
      songState[col] = (byte) ((songState[col] & 0xF0) | (y - 1));
    else
      songState[col] = (byte) ((songState[col] & 0x0F) | ((y - 1) << 4));
  }
  
  private /*inline*/ void patternSet(int x, int y) {
    int col = COLUMN_TO_PATTERN(x);
    if (col >= pattern.length)
      return;
      
    int mask = 1 << y;
    pattern.state[col] ^= mask;
    
    setPixel(x, y, ((pattern.state[col] & mask) != 0) ? pattern.active : pattern.blank);
  }
  
  @Override
  public void onButtonDown(int x, int y) {
    if (y == 0) {
      controlRow(x);
    } else if (pattern == null) {
      songSet(x, y);
    } else {
      patternSet(x, y);
    }
  }
  
  @Override
  public void onButtonUp(int x, int y) {
    if (y == 0 && x == CLOCK_X && !softwareClockEnabled)
      onClockFalling();
  }
  
  private void updateColumn(int idx, boolean cursor) {
    if (pattern != null && playingPatternIdx == patternIdx) {
      int col = idx - scroll;
      
      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, cursor);
    }
  }
  
  private void updateSongColumn(int idx, boolean cursor) {
    if (pattern == null) {
      int col = idx - scroll;
      if (col >= 0 && col < songLen)
        drawSongColumn(col, cursor);
    }
  }
  
  @Override
  public void onClockRising() {
    setPixel(CLOCK_X, 0, direction < 0 ? CLOCK_BACKWARD : CLOCK_FORWARD);
    
    if (playMode == PlayMode.OFF) return;
    
    updateColumn(patternCursor, false);
    patternCursor += direction;
    if (patternCursor >= playingPattern.length) {
      patternCursor = 0;
      if (playMode == PlayMode.SONG) {
        updateSongColumn(songCursor, false);
        songCursor++;
        if (songCursor >= songLen)
          songCursor = 0;
        
        playingPatternIdx = getSongState(songCursor);
        playingPattern = patterns[playingPatternIdx];
        updateSongColumn(songCursor, true);
        updateSongLCDInfo();
      }
    } else if (patternCursor < 0) {
      patternCursor = playingPattern.length - 1;
      if (playMode == PlayMode.SONG) {
        updateSongColumn(songCursor, false);
        songCursor--;
        if (songCursor < 0)
          songCursor = songLen - 1;
          
        playingPatternIdx = getSongState(songCursor);
        playingPattern = patterns[playingPatternIdx];
        updateSongColumn(songCursor, true);
        updateSongLCDInfo();
      }
    }

    System.out.println("Triggers: " + Integer.toBinaryString((playingPattern.state[patternCursor] & 0xFF) + 0x100).substring(1));
    
    updateColumn(patternCursor, true);
  }
  
  @Override
  public void onClockFalling() {
    setPixel(CLOCK_X, 0, CLOCK_OFF);
  }
  
  private void shortenPattern() {
    if (pattern == null) {
      if (songLen <= MIN_PATTERN_LEN)
        return;
      
      songLen--;
      int col = songLen - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawSongColumn(col, true);
    } else {
      if (pattern.length <= MIN_PATTERN_LEN)
        return;
      
      pattern.length--;
      int col = pattern.length - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, true);
    }
  }
  
  private void lengthenPattern() {
    if (pattern == null) {
      if (songLen >= MAX_PATTERN_LEN)
        return;
      
      songLen++;
      int col = songLen - 1 - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawSongColumn(col, true);
    } else {
      if (pattern.length >= MAX_PATTERN_LEN)
        return;
      
      pattern.length++;
      int col = pattern.length - 1 - scroll;
      if (col >= 0 && col < PANEL_WIDTH)
        drawPatternColumn(col, true);
    }
  }
  
  private void scroll(int dir) {
    scroll += dir;
   
    if (pattern == null) {
      clampScroll(songLen);
      for (int x = 0; x < PANEL_WIDTH; x++) {
        drawSongColumn(x, true);
      }
    } else {
      clampScroll(pattern.length);
      for (int x = 0; x < PANEL_WIDTH; x++) {
        drawPatternColumn(x, true);
      }
    }
  }
  
  @Override
  public void onEncoderChange(int encoder, int movement) {
    if (encoder == 0) {
      tempoBPM += movement * TEMPO_STEP;
      updateTempoLCDInfo();
    } else {
      if (rightEncoderPressed) {
        if (movement > 0)
          lengthenPattern();
        else
          shortenPattern();
      } else {
        scroll(movement);
      }
    }
  }
  
  @Override
  public void onEncoderPress(int encoder) {
    if (encoder == 0) {
      if (playMode == PlayMode.OFF) {
        if (prevPlayMode == PlayMode.SONG)
          playSong();
        else
          playPattern();
      } else
        stopPlaying();
    }
    if (encoder == 1) rightEncoderPressed = true;
  }
  
  @Override
  public void onEncoderRelease(int encoder) {
    if (encoder == 1) rightEncoderPressed = false;
  }
}

//enum State {
//  TRIGGER(#FF9E00, 16, 0, "Trigger         "),
//  CV     (#0CED0A, 16, 8, "Control Voltage ");
  
//  private color c;
//  private int scrollXLimit;
//  private int scrollYLimit;
//  private String name;
//  private State(color c, int scrollXLimit, int scrollYLimit, String name) {
//    this.c = c;
//    this.scrollXLimit = scrollXLimit;
//    this.scrollYLimit = scrollYLimit;
//    this.name = name;
//  }
//  color getColor() {
//    return c;
//  }
//  int getSXLimit() {
//    return scrollXLimit;
//  }
//  int getSYLimit() {
//    return scrollYLimit;
//  }
//  String getName() {
//    return name;
//  }
//}

//class RyanSimulator extends Simulator {
//  //private MidiBus midi;

//  // Colors
//  private color blank = color(0, 0, 0);
//  private color active; // Set by initState()
//  private color cursor = color(175, 175, 175);
//  private color forward = #0CED0A;
//  private color backward = #FF9E00;
//  private color outOfBounds = #6B0000;
//  private color[] rainbow = new color[20]; //TODO: Make intro animation with these
  
//  private State state;
//  private boolean[][] displayState = new boolean[DISPLAY_WIDTH * 2][DISPLAY_HEIGHT * 2];
  
//  private boolean playing = false;
//  private int cursorX = 0;
//  private int direction = 1;
  
//  private int scrollX = 0;
//  private int scrollY = 0;
//  private int scrollXLimit = 16;
//  private int scrollYLimit = 8;
//  private int patternLength = 16;
  
//  private final int STEPS_PER_BEAT = 8;
//  private final int SECONDS_PER_MINUTE = 60;
//  private final int BPM_PER_CLOCK_ADJUST = 1;
  
//  private boolean[] triggers = new boolean[DISPLAY_HEIGHT - 1];
  
//  public RyanSimulator(/*final MidiBus midi*/) {
//    //this.midi = midi;
//    clearDisplay();
    
//    // TODO: Is this necessary?
//    for (int i = 0; i < triggers.length; i++) {
//      triggers[i] = false;
//    }
    
//    initState(State.TRIGGER);
    
//    initColors();
    
//    updateBPM();
//  }
  
//  @Override
//  public void onButtonDown(int x, int y) {
//    if (y == 0) {
//      // This pixel is in the control bar
//      switch (x) {
//        case 0: // Play/Stop
//          if (playing) {
//            playing = false;
//            super.setPixel(0, 0, blank);
//          } else {
//            playing = true;
//            cursorX = direction == 1 ? 0 : DISPLAY_WIDTH - 1;
//            super.setPixel(0, 0, forward);
//          }
//          break;
//        case 1: // Reset
//          if (playing) {
//            cursorX = direction == 1 ? 0 : DISPLAY_WIDTH - 1;
//          }
//          super.setPixel(1, 0, forward);
//          break;
//        case 2: // Clear pattern
//          clearDisplay();
//          if (state == State.CV) autoFillUntilDifferentColumn(0, DISPLAY_HEIGHT - 1, -1);
//          super.setPixel(2, 0, forward);
//          break;
//        case 3: // Toggle state
//          switch (state) {
//            case TRIGGER:
//              initState(State.CV);
//              break;
//            case CV:
//              initState(State.TRIGGER);
//              break;
//          }
//          break;
//        case 4: // Scroll down
//          if (scrollY < scrollYLimit) scrollY++;
//          break;
//        case 5: // Scroll up
//          if (scrollY > 0) scrollY--;
//          break;
//        case 6: // Scroll left
//          if (scrollX > 0) scrollX--;
//          break;
//        case 7: // Scroll right
//          if (scrollX < scrollXLimit) scrollX++;
//          break; 
//        case 8: // Decrease length
//          if (patternLength > 1) patternLength--;
//          break;
//        case 9: // Increase length
//          if (patternLength < DISPLAY_WIDTH * 2 - 1) patternLength++;
//          break;
//        case 11: // Swing down
//          if (swingFactor > 0.04f) swingFactor -= 0.05f;
//          if (swingFactor < 0) swingFactor = 0;
//          updateBPM();
//          break;
//        case 12: // Swing up
//          if (swingFactor < 0.96f) swingFactor += 0.05f;
//          if (swingFactor > 1) swingFactor = 1;
//          updateBPM();
//          break;
//        case 13: // Clock speed down
//          if (clocksPerSecond <= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST + 0.0001) direction = -1;
//          clocksPerSecond -= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST * direction;
//          updateBPM();
//          break;
//        case 14: // Clock speed up
//          if (clocksPerSecond <= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST + 0.0001) direction = 1;
//          clocksPerSecond += (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST * direction;
//          updateBPM();
//          break;
//      }
//    } else {
//      if (state == State.TRIGGER) {
//        if (x + scrollX < patternLength) togglePixel(x + scrollX, y + scrollY);
//      } else if (state == State.CV && x + scrollX < patternLength) {
//        // Autofill is complicated
//        int pos = -1;
//        for (int i = 0; i < displayState[0].length - 1; i++) {
//          if (displayState[x + scrollX][displayState[0].length - i - 1]) {
//            pos = i;
//            break;
//          }
//        }
//        clearColumnExcept(x + scrollX, y + scrollY);
//        displayState[x + scrollX][y + scrollY] = true;
//        if (x + scrollX != patternLength - 1) autoFillUntilDifferentColumn(x + scrollX + 1, y + scrollY, pos);
//      }
//    }
//    updateDisplay();
//  }
  
//  private void updateBPM() {
//    int bpm = (int) (clocksPerSecond * (float) SECONDS_PER_MINUTE / (float) STEPS_PER_BEAT);
//    String bpmString = String.format("%03d", bpm * direction);
//    lcd.setCursor(0, 1);
//    lcd.print(bpmString + "bpm, " + (int) Math.round(swingFactor * 100) + "% sw     ");
//}
  
//  @Override
//  public void onButtonUp(int x, int y) {
//    if (y == 0) {
//      switch (x) {
//        case 1:
//          super.setPixel(1, 0, blank);
//          break;
//        case 2:
//          super.setPixel(2, 0, blank);
//          break;
//      }
//    }
//  }
  
//  @Override
//  public void onClockRising() {
//    // Move cursor
//    if (playing) {
//      cursorX += direction;
//      if (cursorX >= patternLength) cursorX = 0;
//      if (cursorX == -1) cursorX = patternLength - 1;
//    }
    
//    // Check triggers
//    if (playing && state == State.TRIGGER) {
//      for (int i = 0; i < displayState[0].length - 1; i++) {
//        if (displayState[cursorX][i + 1]) {
//          onTriggerDown(i);
//          triggers[i] = true;
//        }
//      }
//    }
    
//    // Check CV 
//    if (playing && state == State.CV) {
//      boolean found = false;
//      for (int i = 0; i < DISPLAY_HEIGHT - 1; i++) {
//        if (super.getPixel(cursorX, DISPLAY_HEIGHT - i - 1) == active) {
//          setCV(i);
//          found = true;
//          break;
//        }
//      }
//      if (!found) setCV(-1);
//    }
    
//    // Clock blinky on
//    super.setPixel(15, 0, direction == 1 ? forward : backward);
    
//    updateDisplay();
//  }
  
//  @Override
//  public void onClockFalling() {
//    // Triggers up
//    for (int i = 0; i < DISPLAY_HEIGHT - 1; i++) {
//      if (triggers[i]) {
//        onTriggerUp(i);
//        triggers[i] = false;
//      }
//    }
    
//    // Clock blinky off
//    super.setPixel(15, 0, blank);
//  }
  
//  @Override
//  public void onEncoderChange(int encoder, int movement) {
//    System.out.println("Encoder change: " + encoder + ": " + movement);
//  }
  
//  @Override
//  public void onEncoderPress(int encoder) {
//    System.out.println("Encoder press: " + encoder);
//  }
  
//  @Override
//  public void onEncoderRelease(int encoder) {
//    System.out.println("Encoder release: " + encoder);
//  }
  
//  void onTriggerDown(int trigger) {
//    println("Trigger " + trigger + " down");
//    //midi.sendNoteOn(0, 32 + trigger, 127);
//  }
  
//  void onTriggerUp(int trigger) {
//    println("Trigger " + trigger + " up");
//    //midi.sendNoteOff(0, 32 + trigger, 127);
//  }
  
//  // Sets the CV out level
//  // Range: 0 - 7, if -1 is passed, set output to floating
//  void setCV(int level) {
//    println("CV out level " + level);
//  }
  
//  private void updateDisplay() {
//    for (int x = 0; x < DISPLAY_WIDTH; x++) {
//      for (int y = 1; y < DISPLAY_HEIGHT; y++) {
//        super.setPixel(x, y, x + scrollX < patternLength ? (displayState[x + scrollX][y + scrollY] ? active : (x + scrollX) == cursorX && playing ? cursor : blank) : outOfBounds);
//      }
//    }
//  }
  
//  private void initState(State state) {
//    // Stop
//    playing = false;
//    super.setPixel(0, 0, blank);
    
//    // Clear
//    clearDisplay();
    
//    active = state.getColor();
//    super.setPixel(3, 0, active);
//    this.state = state;
    
//    scrollXLimit = state.getSXLimit();
//    scrollYLimit = state.getSYLimit();
    
//    if (state == State.CV) {
//      autoFillUntilDifferentColumn(0, DISPLAY_HEIGHT - 1, -1);
//    }
    
//    lcd.setCursor(0, 0);
//    lcd.print(state.getName());
//  }
  
//  // Toggles whether a pixel is selected
//  private void togglePixel(int x, int y) {
//    displayState[x][y] = !displayState[x][y];
//  }
  
//  // Clears the display
//  private void clearDisplay() {
    
//    for (int x = 0; x < displayState.length; x++) {
//      for (int y = 0; y < displayState[0].length; y++) {
//        displayState[x][y] = false;
//      }
//    }
//  }
  
//  private void autoFillUntilDifferentColumn(int start, int y, int currentPos) {
//    for (int x = start; x < patternLength; x++) {
//      int pos = -1;
//      for (int i = 0; i < displayState[0].length - 1; i++) {
//        if (displayState[x][displayState[0].length - i - 1]) {
//          pos = i;
//          break;
//        }
//      }
//      if (pos != currentPos) return;
//      clearColumn(x);
//      displayState[x][y] = true;
//    }
//  }
  
//  // Clears a row
//  private void clearRow(int y) {
//    for (int x = 0; x < displayState.length; x++) {
//      displayState[x][y] = false;
//    }
//  }
  
//  // Clears a column
//  private void clearColumn(int x) {
//    for (int y = 1; y < displayState[0].length; y++) {
//      displayState[x][y] = false;
//    }
//  }
  
//  private void clearColumnExcept(int x, int excludedY) {
//    for (int y = 1; y < displayState[0].length; y++) {
//      if (y == excludedY) continue;
//      displayState[x][y] = false;
//    }
//  }
  
//  private void initColors() {
//    for (int i = 0; i < 20; i++) {
//      float hue = i / 20.0f;
//      Color c = new Color(Color.HSBtoRGB(hue, 1, 1));
//      rainbow[i] = color(c.getRed(), c.getGreen(), c.getBlue());
//    }
//  }
//}

abstract class Simulator {
  abstract void onButtonDown(int x, int y);
  abstract void onButtonUp(int x, int y);
  abstract void onClockRising();
  abstract void onClockFalling();
  abstract void onEncoderChange(int encoder, int movement);
  abstract void onEncoderPress(int encoder);
  abstract void onEncoderRelease(int encoder);
  
  protected void setPixel(int x, int y, color c) {
    delay(1);
    display[x][y] = c;
  }
  protected color getPixel(int x, int y) {
    return display[x][y];
  }
}
