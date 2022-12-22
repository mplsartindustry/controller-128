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

import java.awt.event.KeyEvent;

//import themidibus.*;

// Simulates controller_128 hardware

//float clocksPerSecond = 10.0f;
boolean clockEdge = true;
float swingFactor = 0;
boolean evenClock = false;

final int DISPLAY_WIDTH = 16;
final int DISPLAY_HEIGHT = 8;
color[][] display = new color[DISPLAY_WIDTH][DISPLAY_HEIGHT];

final int CLOCK_PER_BEAT = 4;

final int buttonSize = 36;

// Style colors
final int frame = color(56, 36, 29);
final int steel = color(126, 122, 121);
final int highlight = color(80, 0, 80);

// Cursor
int cursorX = 0;
int cursorY = 0;

long lastTime = System.currentTimeMillis();
double unprocessedTime = 0;

// The software simulator
Simulator simulator;

//MidiBus midi;

LCD lcd;
Encoder leftEncoder, rightEncoder;

boolean softwareClockEnabled;
int tempoBPM;

final char LEFT_ENC_CCW = 'a';
final char LEFT_ENC_S = 's';
final char LEFT_ENC_CW = 'd';

final char RIGHT_ENC_CCW = 'j';
final char RIGHT_ENC_S = 'k';
final char RIGHT_ENC_CW = 'l';

void settings() {
  //size(788, 484);
  size(1020, 484);
  
  noSmooth();
  
  tempoBPM = 60;
}

void setup() {
  // list all MIDI ports
  //MidiBus.list();
  
  lcd = new LCD();
  
  leftEncoder = new Encoder(725, 200, 75, LEFT_ENC_CCW, LEFT_ENC_S, LEFT_ENC_CW);
  rightEncoder = new Encoder(875, 200, 75, RIGHT_ENC_CCW, RIGHT_ENC_S, RIGHT_ENC_CW);

  // this, MIDI in port, MIDI out port
  //midi = new MidiBus(this, 0, 1);
  simulator = new RyanSimulator2(/*midi*/);
}

// Using draw() as an update method, the actual rendering is in render()
void draw() {
  if (softwareClockEnabled) {
    // Run the clock
    long currentTime = System.currentTimeMillis();
    long passedTime = currentTime - lastTime;
    lastTime = currentTime;
    unprocessedTime += passedTime / 1000.0;
    float secondsPerClock = 0.5f / (tempoBPM * CLOCK_PER_BEAT / 60f);
    while (unprocessedTime > secondsPerClock + secondsPerClock * (evenClock ? swingFactor : -swingFactor)) {
      if (clockEdge) {
        simulator.onClockRising();
        evenClock = !evenClock;
      } else {
        simulator.onClockFalling();
      }
      clockEdge = !clockEdge;
      unprocessedTime -= secondsPerClock;
    }
  } else if (!clockEdge) {
    simulator.onClockFalling();
    clockEdge = true;
  }
  
  if (!softwareClockEnabled) {
    unprocessedTime = 0;
    lastTime = System.currentTimeMillis();
  }

  render();
}

void render() {
  fill(0);
  noStroke();
  rect(0, 0, width, height);

  fill(steel);
  stroke(frame);
  strokeWeight(buttonSize/2.0f);
  roundRect(buttonSize, buttonSize, width - 2*buttonSize, height - 2*buttonSize, buttonSize*2);

  noStroke();
  for (int x = 0; x < DISPLAY_WIDTH; x++)
  {
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
      fill(display[x][y]);
      roundRect(2.5*buttonSize + x * (buttonSize + 2), 2.5*buttonSize + y * (buttonSize + 2), buttonSize, buttonSize, 2);
    }
  }

  noFill();
  stroke(highlight);
  strokeWeight(1);
  roundRect(2.5*buttonSize + cursorX * (buttonSize + 2), 2.5*buttonSize + cursorY * (buttonSize + 2), buttonSize, buttonSize, 2);

  // LCD
  stroke(0);
  fill(color(0, 0, 255));
  float lcdX = 2.5 * buttonSize + (DISPLAY_WIDTH + 1) * (buttonSize + 2);
  float lcdY = 2.5 * buttonSize;
  //rect(lcdX, lcdY, 205, 50);
  
  lcd.render(lcdX, lcdY, 196, 44);
  
  leftEncoder.draw();
  rightEncoder.draw();
}

void keyReleased() {
  switch (key) {
    case LEFT_ENC_S: simulator.onEncoderRelease(0); break;
    case RIGHT_ENC_S: simulator.onEncoderRelease(1); break;
  }
}

void keyPressed()
{
  if (key != CODED) {
    switch (key) {
      case LEFT_ENC_CCW:  simulator.onEncoderChange(0, -1); break;
      case LEFT_ENC_CW:   simulator.onEncoderChange(0, 1); break;
      case RIGHT_ENC_CCW: simulator.onEncoderChange(1, -1); break;
      case RIGHT_ENC_CW:  simulator.onEncoderChange(1, 1); break;
      case LEFT_ENC_S:    simulator.onEncoderPress(0); break;
      case RIGHT_ENC_S:   simulator.onEncoderPress(1); break;
    }
  } else {
    switch (keyCode)
    {
      case UP:
        cursorY--;
        break;
      case DOWN:
        cursorY++;
        break;
      case LEFT:
        cursorX--;
        break;
      case RIGHT:
        cursorX++;
        break;
      case CONTROL:
      case KeyEvent.VK_SPACE:
        simulator.onButtonDown(cursorX, cursorY);
        simulator.onButtonUp(cursorX, cursorY);
        break;
      default:
        break;
    }
    cursorX = constrain(cursorX, 0, DISPLAY_WIDTH - 1);
    cursorY = constrain(cursorY, 0, DISPLAY_HEIGHT - 1);
  }
}

void mouseDragged() {
  mouseMoved();
}

void mouseMoved()
{
  int i = 0;
  float x = 2.5*buttonSize;
  while (x < mouseX)
  {
    x += buttonSize + 2; 
    i++;
  }
  int j = 0;
  float y = 2.5*buttonSize;
  while (y < mouseY)
  {
    y += buttonSize + 2;
    j++;
  }
  cursorX = constrain(i - 1, 0, DISPLAY_WIDTH - 1);
  cursorY = constrain(j - 1, 0, DISPLAY_HEIGHT - 1);
}

void mousePressed()
{
  simulator.onButtonDown(cursorX, cursorY);
}

void mouseReleased()
{
  simulator.onButtonUp(cursorX, cursorY);
}

private float prevX, prevY;

private void roundRect(final float x, final float y, final float w, final float h, final float r) {
  beginShape();
  vertex(x+r, y);

  vertex(x+w-r, y);
  prevX = x+w-r;
  prevY = y;
  quadraticBezierVertex(x+w, y, x+w, y+r);

  vertex(x+w, y+h-r);
  prevX = x+w;
  prevY = y+h-r;
  quadraticBezierVertex(x+w, y+h, x+w-r, y+h);

  vertex(x+r, y+h);
  prevX = x+r;
  prevY = y+h;
  quadraticBezierVertex(x, y+h, x, y+h-r);

  vertex(x, y+r);
  prevX = x;
  prevY = y+r;
  quadraticBezierVertex(x, y, x+r, y);

  endShape();
}

private void quadraticBezierVertex(final float cpx, final float cpy, final float x, final float y) {
  float cp1x = prevX + 2.0/3.0*(cpx - prevX);
  float cp1y = prevY + 2.0/3.0*(cpy - prevY);
  float cp2x = cp1x + (x - prevX)/3.0;
  float cp2y = cp1y + (y - prevY)/3.0;
  bezierVertex(cp1x, cp1y, cp2x, cp2y, x, y);
}
