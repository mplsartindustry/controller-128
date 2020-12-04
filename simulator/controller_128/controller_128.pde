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

import org.dishevelled.matrix.BitMatrix1D;

import org.dishevelled.processing.executor.Executor;

import themidibus.*;

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

// patterns
static final int P = H - 1;
static final Pattern PATTERNS[] = new Pattern[P];

// default colors, monome greyscale 128 style
//final int frame = color(24, 27, 30);
//final int steel = color(185, 193, 196);
//final int ledOff = color(129, 137, 137);
//final int ledOn = color(253, 252, 252);
//final int highlight = color(0, 0, 80);

// alternate colors, monome 512 style
final int frame = color(56, 36, 29);
final int steel = color(126, 122, 121);
final int ledOff = color(100, 94, 98);
final int ledOn = color(245, 124, 77);
final int highlight = color(80, 0, 80);

// cursor
int cursorX = 0;
int cursorY = 0;

// true if running
boolean running = false;

// clock rate, in BPM, 4.0 to 240.0
float rate = 120.0;

// swing amount, 50.0 to 100.0, default 50.0
float swing = 50.0;

// humanize amount, -1.0 to 1.0, default 0.0
float humanize = 0.0;

// true if odd beat
boolean odd = false;

// internal clock rate, when not running (in ms)
static final long INTERNAL_CLOCK_RATE = 10L;

Executor executor;
MidiBus midi;

void setup()
{
  //size(SKETCH_WIDTH, SKETCH_HEIGHT);
  //println(SKETCH_WIDTH + " " + SKETCH_HEIGHT);
  size(788, 484);
  executor = new Executor(this, 4);

  MidiBus.list();
  midi = new MidiBus(this, 0, 0);

  // initialize patterns
  for (int i = 0; i < P; i++)
  {
    PATTERNS[i] = new Pattern(W);
  }

  smooth();
  background(0);
  clear(false);

  Thread loop = new Thread(new Runnable()
  {
    public void run()
    {
      while (true)
      {
        if (running)
        {
          // calculate delay
          long ms = odd ? oddDelay() : evenDelay();

          // rising edge
          try { Thread.currentThread().sleep(ms/2); } catch (InterruptedException e) {}
          button(W - 1, 0, true);
          for (int i = 0; i < P; i++)
          {
            button(PATTERNS[i].peek(), i + 1, true);
            if (PATTERNS[i].get()) midi.sendNoteOn(0, 32 + i, 127);
          }

          // falling edge
          try { Thread.currentThread().sleep(ms/2); } catch (InterruptedException e) {}
          button(W - 1, 0, false);
          for (int i = 0; i < P; i++)
          {
            if (!PATTERNS[i].get(PATTERNS[i].peek())) button(PATTERNS[i].peek(), i + 1, false);
            PATTERNS[i].step();
          }

          odd = !odd;
        }
        try { Thread.currentThread().sleep(INTERNAL_CLOCK_RATE); } catch (InterruptedException e) {}
      }
    }
  });

  loop.start();
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

void play()
{
  if (running)
  {
    pause();
  }
  else
  {
    restart();
  }
}

void pause()
{
  println("pause");
  running = false;
  button(0, 0, false);
}

void restart()
{
  println("restart");
  running = true;
  button(0, 0, true);
}

void step()
{
  println("step");
  for (int i = 0; i < P; i++)
  {
    PATTERNS[i].step();
  }
}

void reset()
{
  println("reset");
  for (int i = 0; i < P; i++)
  {
    PATTERNS[i].reset();
  }
  button(1, 0, true);
  executor.later("unblinkReset", 50);
}

void unblinkReset()
{
  button(1, 0, false);
}

void clockUp()
{
  rate = min(rate + 0.5, 240.0);
  println("clockUp " + rate);
  button(W - 2, 0, true);
  executor.later("unblinkClockUp", 50);
}

void unblinkClockUp()
{
  button(W - 2, 0, false);
}

void clockDown()
{
  rate = max(rate - 0.5, 4.0);
  println("clockDown " + rate);
  button(W - 3, 0, true);
  executor.later("unblinkClockDown", 50);
}

void unblinkClockDown()
{
  button(W - 3, 0, false);
}

void buttonPressed(final int x, final int y)
{
  if (y == 0)
  {
    switch (x) {
      case 0: play(); break;
      case 1: reset(); break;
      case (W - 3): clockDown(); break;
      case (W - 2): clockUp(); break;
    }
  }
  else
  {
    toggle(x, y);
    PATTERNS[y - 1].toggle(x);
  }
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

float humanizeDur(float dur) {
  if (humanize > 0.0 || humanize < 0.0) {
    return dur + (dur * humanize * 0.001 * random(100));
  }
  return dur;
}

long oddDelay() {
  float q = 60000.0/rate;
  float e = q / 4.0;
  float s1 = e * (swing/100.0);
  return (long) humanizeDur(s1);
}

long evenDelay() {
  float q = 60000.0/rate;
  float e = q / 4.0;
  float s1 = e * (swing/100.0);
  float s2 = e - s1;
  return (long) humanizeDur(s2);
}

static class Pattern
{
  private int step;
  private int start;
  private int length;
  private final BitMatrix1D bits = new BitMatrix1D(1024);

  Pattern(final int length) {
    this.length = length;
  }

  public boolean get() { return bits.get(start + step); }
  public int peek() { return step; }
  public void step() { step++; if (step >= length) { step = 0; }};
  public void reset() { step = 0; }

  public void start(final int start) { this.start = start; }
  public int start() { return start; }
  public void length(final int length) { this.length = length; }
  public int length() { return length; }
  public void set(final int index, final boolean value) { bits.set(start + index, value); }
  public void set(final int i0, final int i1, final boolean value) { bits.set(start + i0, start + i1, value); }
  public void toggle(final int index) { bits.flip(start + index); }
  public void toggle(final int i0, final int i1) { bits.flip(start + i0 , start + i1); }
  public boolean get(final int index) { return bits.get(start + index); }
  public void clear() { bits.clear(); }
}
