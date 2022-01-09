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

import themidibus.*;
import java.awt.Color;

// Simulates controller_128 software

enum State {
  TRIGGER(#FF9E00, 16, 0, "Trigger         "),
  CV     (#0CED0A, 16, 8, "Control Voltage ");
  
  private color c;
  private int scrollXLimit;
  private int scrollYLimit;
  private String name;
  private State(color c, int scrollXLimit, int scrollYLimit, String name) {
    this.c = c;
    this.scrollXLimit = scrollXLimit;
    this.scrollYLimit = scrollYLimit;
    this.name = name;
  }
  color getColor() {
    return c;
  }
  int getSXLimit() {
    return scrollXLimit;
  }
  int getSYLimit() {
    return scrollYLimit;
  }
  String getName() {
    return name;
  }
}

class RyanSimulator extends Simulator {
  private MidiBus midi;

  // Colors
  private color blank = color(0, 0, 0);
  private color active; // Set by initState()
  private color cursor = color(175, 175, 175);
  private color forward = #0CED0A;
  private color backward = #FF9E00;
  private color outOfBounds = #6B0000;
  private color[] rainbow = new color[20]; //TODO: Make intro animation with these
  
  private State state;
  private boolean[][] displayState = new boolean[DISPLAY_WIDTH * 2][DISPLAY_HEIGHT * 2];
  
  private boolean playing = false;
  private int cursorX = 0;
  private int direction = 1;
  
  private int scrollX = 0;
  private int scrollY = 0;
  private int scrollXLimit = 16;
  private int scrollYLimit = 8;
  private int patternLength = 16;
  
  private final int STEPS_PER_BEAT = 8;
  private final int SECONDS_PER_MINUTE = 60;
  private final int BPM_PER_CLOCK_ADJUST = 1;
  
  private boolean[] triggers = new boolean[DISPLAY_HEIGHT - 1];
  
  public RyanSimulator(final MidiBus midi) {
    this.midi = midi;
    clearDisplay();
    
    // TODO: Is this necessary?
    for (int i = 0; i < triggers.length; i++) {
      triggers[i] = false;
    }
    
    initState(State.TRIGGER);
    
    initColors();
    
    updateBPM();
  }
  
  @Override
  void onButtonDown(int x, int y) {
    if (y == 0) {
      // This pixel is in the control bar
      switch (x) {
        case 0: // Play/Stop
          if (playing) {
            playing = false;
            super.setPixel(0, 0, blank);
          } else {
            playing = true;
            cursorX = direction == 1 ? 0 : DISPLAY_WIDTH - 1;
            super.setPixel(0, 0, forward);
          }
          break;
        case 1: // Reset
          if (playing) {
            cursorX = direction == 1 ? 0 : DISPLAY_WIDTH - 1;
          }
          super.setPixel(1, 0, forward);
          break;
        case 2: // Clear pattern
          clearDisplay();
          if (state == State.CV) autoFillUntilDifferentColumn(0, DISPLAY_HEIGHT - 1, -1);
          super.setPixel(2, 0, forward);
          break;
        case 3: // Toggle state
          switch (state) {
            case TRIGGER:
              initState(State.CV);
              break;
            case CV:
              initState(State.TRIGGER);
              break;
          }
          break;
        case 4: // Scroll down
          if (scrollY < scrollYLimit) scrollY++;
          break;
        case 5: // Scroll up
          if (scrollY > 0) scrollY--;
          break;
        case 6: // Scroll left
          if (scrollX > 0) scrollX--;
          break;
        case 7: // Scroll right
          if (scrollX < scrollXLimit) scrollX++;
          break; 
        case 8: // Decrease length
          if (patternLength > 1) patternLength--;
          break;
        case 9: // Increase length
          if (patternLength < DISPLAY_WIDTH * 2 - 1) patternLength++;
          break;
        case 11: // Swing down
          if (swingFactor > 0.04f) swingFactor -= 0.05f;
          if (swingFactor < 0) swingFactor = 0;
          updateBPM();
          break;
        case 12: // Swing up
          if (swingFactor < 0.96f) swingFactor += 0.05f;
          if (swingFactor > 1) swingFactor = 1;
          updateBPM();
          break;
        case 13: // Clock speed down
          if (clocksPerSecond <= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST + 0.0001) direction = -1;
          clocksPerSecond -= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST * direction;
          updateBPM();
          break;
        case 14: // Clock speed up
          if (clocksPerSecond <= (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST + 0.0001) direction = 1;
          clocksPerSecond += (float) STEPS_PER_BEAT / (float) SECONDS_PER_MINUTE * BPM_PER_CLOCK_ADJUST * direction;
          updateBPM();
          break;
      }
    } else {
      if (state == State.TRIGGER) {
        if (x + scrollX < patternLength) togglePixel(x + scrollX, y + scrollY);
      } else if (state == State.CV && x + scrollX < patternLength) {
        // Autofill is complicated
        int pos = -1;
        for (int i = 0; i < displayState[0].length - 1; i++) {
          if (displayState[x + scrollX][displayState[0].length - i - 1]) {
            pos = i;
            break;
          }
        }
        clearColumnExcept(x + scrollX, y + scrollY);
        displayState[x + scrollX][y + scrollY] = true;
        if (x + scrollX != patternLength - 1) autoFillUntilDifferentColumn(x + scrollX + 1, y + scrollY, pos);
      }
    }
    updateDisplay();
  }
  
  private void updateBPM() {
    int bpm = (int) (clocksPerSecond * (float) SECONDS_PER_MINUTE / (float) STEPS_PER_BEAT);
    String bpmString = String.format("%03d", bpm * direction);
    lcd.setCursor(0, 1);
    lcd.print(bpmString + "bpm, " + (int) Math.round(swingFactor * 100) + "% sw     ");
}
  
  @Override
  void onButtonUp(int x, int y) {
    if (y == 0) {
      switch (x) {
        case 1:
          super.setPixel(1, 0, blank);
          break;
        case 2:
          super.setPixel(2, 0, blank);
          break;
      }
    }
  }
  
  @Override
  void onClockRising() {
    // Move cursor
    if (playing) {
      cursorX += direction;
      if (cursorX >= patternLength) cursorX = 0;
      if (cursorX == -1) cursorX = patternLength - 1;
    }
    
    // Check triggers
    if (playing && state == State.TRIGGER) {
      for (int i = 0; i < displayState[0].length - 1; i++) {
        if (displayState[cursorX][i + 1]) {
          onTriggerDown(i);
          triggers[i] = true;
        }
      }
    }
    
    // Check CV 
    if (playing && state == State.CV) {
      boolean found = false;
      for (int i = 0; i < DISPLAY_HEIGHT - 1; i++) {
        if (super.getPixel(cursorX, DISPLAY_HEIGHT - i - 1) == active) {
          setCV(i);
          found = true;
          break;
        }
      }
      if (!found) setCV(-1);
    }
    
    // Clock blinky on
    super.setPixel(15, 0, direction == 1 ? forward : backward);
    
    updateDisplay();
  }
  
  @Override
  void onClockFalling() {
    // Triggers up
    for (int i = 0; i < DISPLAY_HEIGHT - 1; i++) {
      if (triggers[i]) {
        onTriggerUp(i);
        triggers[i] = false;
      }
    }
    
    // Clock blinky off
    super.setPixel(15, 0, blank);
  }
  
  void onTriggerDown(int trigger) {
    println("Trigger " + trigger + " down");
    midi.sendNoteOn(0, 32 + trigger, 127);
  }
  
  void onTriggerUp(int trigger) {
    println("Trigger " + trigger + " up");
    midi.sendNoteOff(0, 32 + trigger, 127);
  }
  
  // Sets the CV out level
  // Range: 0 - 7, if -1 is passed, set output to floating
  void setCV(int level) {
    println("CV out level " + level);
  }
  
  private void updateDisplay() {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      for (int y = 1; y < DISPLAY_HEIGHT; y++) {
        super.setPixel(x, y, x + scrollX < patternLength ? (displayState[x + scrollX][y + scrollY] ? active : (x + scrollX) == cursorX && playing ? cursor : blank) : outOfBounds);
      }
    }
  }
  
  private void initState(State state) {
    // Stop
    playing = false;
    super.setPixel(0, 0, blank);
    
    // Clear
    clearDisplay();
    
    active = state.getColor();
    super.setPixel(3, 0, active);
    this.state = state;
    
    scrollXLimit = state.getSXLimit();
    scrollYLimit = state.getSYLimit();
    
    if (state == State.CV) {
      autoFillUntilDifferentColumn(0, DISPLAY_HEIGHT - 1, -1);
    }
    
    lcd.setCursor(0, 0);
    lcd.print(state.getName());
  }
  
  // Toggles whether a pixel is selected
  private void togglePixel(int x, int y) {
    displayState[x][y] = !displayState[x][y];
  }
  
  // Clears the display
  private void clearDisplay() {
    
    for (int x = 0; x < displayState.length; x++) {
      for (int y = 0; y < displayState[0].length; y++) {
        displayState[x][y] = false;
      }
    }
  }
  
  private void autoFillUntilDifferentColumn(int start, int y, int currentPos) {
    for (int x = start; x < patternLength; x++) {
      int pos = -1;
      for (int i = 0; i < displayState[0].length - 1; i++) {
        if (displayState[x][displayState[0].length - i - 1]) {
          pos = i;
          break;
        }
      }
      if (pos != currentPos) return;
      clearColumn(x);
      displayState[x][y] = true;
    }
  }
  
  // Clears a row
  private void clearRow(int y) {
    for (int x = 0; x < displayState.length; x++) {
      displayState[x][y] = false;
    }
  }
  
  // Clears a column
  private void clearColumn(int x) {
    for (int y = 1; y < displayState[0].length; y++) {
      displayState[x][y] = false;
    }
  }
  
  private void clearColumnExcept(int x, int excludedY) {
    for (int y = 1; y < displayState[0].length; y++) {
      if (y == excludedY) continue;
      displayState[x][y] = false;
    }
  }
  
  private void initColors() {
    for (int i = 0; i < 20; i++) {
      float hue = i / 20.0f;
      Color c = new Color(Color.HSBtoRGB(hue, 1, 1));
      rainbow[i] = color(c.getRed(), c.getGreen(), c.getBlue());
    }
  }
}

abstract class Simulator {
  abstract void onButtonDown(int x, int y);
  abstract void onButtonUp(int x, int y);
  abstract void onClockRising();
  abstract void onClockFalling();
  protected void setPixel(int x, int y, color c) {
    display[x][y] = c;
  }
  protected color getPixel(int x, int y) {
    return display[x][y];
  }
}
