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

// size
static final int W = 16;
static final int H = 8;

// button size
static final int S = 36;

// sketch size
static final int SKETCH_WIDTH = W * S + 5*S + 2*W;
static final int SKETCH_HEIGHT = H * S + 5*S + 2*H;

// button state
static final boolean STATE[][] = new boolean[W][H];

// default colors, monome greyscale 128 style
final int frame = color(24, 27, 30);
final int steel = color(185, 193, 196);
final int ledOff = color(129, 137, 137);
final int ledOn = color(253, 252, 252);
final int highlight = color(0, 0, 80);

// alternate colors, monome 512 style
//final int frame = color(56, 36, 29);
//final int steel = color(126, 122, 121);
//final int ledOff = color(100, 94, 98);
//final int ledOn = color(245, 124, 77);
//final int highlight = color(80, 0, 80);

// cursor
int cursorX = 0;
int cursorY = 0;

void setup()
{
  //size(SKETCH_WIDTH, SKETCH_HEIGHT);
  //println(SKETCH_WIDTH + " " + SKETCH_HEIGHT);
  size(788, 484);

  smooth();
  background(0);
  clear(false);
}

void draw()
{
  fill(0);
  noStroke();
  rect(0, 0, width, height);

  fill(steel);
  stroke(frame);
  strokeWeight(S/2.0f);
  roundRect(S, S, width - 2*S, height - 2*S, S*2);

  noStroke();
  for (int x = 0; x < W; x++)
  {
    for (int y = 0; y < H; y++)
    {
      fill(STATE[x][y] ? ledOn : ledOff);
      roundRect(2.5*S + x * (S + 2), 2.5*S + y * (S + 2), S, S, 2);
    }
  }

  noFill();
  stroke(highlight);
  strokeWeight(1);
  roundRect(2.5*S + cursorX * (S + 2), 2.5*S + cursorY * (S + 2), S, S, 2);
}

void clear(final boolean state)
{
  rect(0, 0, W, H, state);
}

boolean button(final int x, final int y)
{
  return STATE[x][y];
}

void button(final int x, final int y, final boolean state)
{
  STATE[x][y] = state;
}

void toggle(final int x, final int y)
{
  button(x, y, !button(x, y));
}

void row(final int row, final boolean state)
{
  for (int x = 0; x < W; x++)
  {
    button(x, row, state);
  }
}

void column(final int column, final boolean state)
{
  for (int y = 0; y < H; y++)
  {
    button(column, y, state);
  }
}

void rect(final int x1, final int y1, final int x2, final int y2, final boolean state)
{
  for (int x = x1; x < x2; x++)
  {
    for (int y = y1; y < y2; y++)
    {
      button(x, y, state);
    }
  }
}

void buttonPressed(final int x, final int y)
{
  toggle(x, y);
}

void buttonReleased(final int x, final int y)
{
  // empty
}

void keyPressed()
{
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
      buttonPressed(cursorX, cursorY);
      buttonReleased(cursorX, cursorY);
      break;
    default:
      break;
  }
  cursorX = constrain(cursorX, 0, W - 1);
  cursorY = constrain(cursorY, 0, H - 1);
}

void mouseMoved()
{
  int i = 0;
  float x = 2.5*S;
  while (x < mouseX)
  {
    x += S + 2; 
    i++;
  }
  int j = 0;
  float y = 2.5*S;
  while (y < mouseY)
  {
    y += S + 2;
    j++;
  }
  cursorX = constrain(i - 1, 0, W - 1);
  cursorY = constrain(j - 1, 0, H - 1);
}

void mousePressed()
{
  buttonPressed(cursorX, cursorY);
}

void mouseReleased()
{
  buttonReleased(cursorX, cursorY);
}

private float prevX;
private float prevY;

private void roundRect(final int x, final int y, final int w, final int h, final int r)
{
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

private void quadraticBezierVertex(final int cpx, final int cpy, final int x, final int y)
{
  float cp1x = prevX + 2.0/3.0*(cpx - prevX);
  float cp1y = prevY + 2.0/3.0*(cpy - prevY);
  float cp2x = cp1x + (x - prevX)/3.0;
  float cp2y = cp1y + (y - prevY)/3.0;
  bezierVertex(cp1x, cp1y, cp2x, cp2y, x, y);
}

private void roundRect(final float x, final float y, final float w, final float h, final float r)
{
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

private void quadraticBezierVertex(final float cpx, final float cpy, final float x, final float y)
{
  float cp1x = prevX + 2.0/3.0*(cpx - prevX);
  float cp1y = prevY + 2.0/3.0*(cpy - prevY);
  float cp2x = cp1x + (x - prevX)/3.0;
  float cp2y = cp1y + (y - prevY)/3.0;
  bezierVertex(cp1x, cp1y, cp2x, cp2y, x, y);
}
