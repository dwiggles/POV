#include "PixelDisplay.h"
#include <sstream>

PixelDisplay::PixelDisplay(HWND hWnd, int w, int h) {

  visibleHwnd = hWnd;
  eraseBackground = false;
  width = w;
  height = h;
  frameCount = 0;

  // Create a memory DC that will receive the drawing.
  memdc = CreateCompatibleDC(GetDC(hWnd));
  HBITMAP hbitmap = CreateCompatibleBitmap(GetDC(hWnd), width, height);
  SelectObject(memdc, hbitmap);

  // Create another memory DC that will receive text from TextOut before
  // being blitted into the main memory DC memdc. This is necessary because
  // TextOut doesn't respect the SetROP2 setting so it isn't possible to XOR
  // the text on to the screen with just TextOut.
  textdc = CreateCompatibleDC(GetDC(hWnd));
  HBITMAP hbitmap2 = CreateCompatibleBitmap(GetDC(hWnd), width, height);
  SelectObject(textdc, hbitmap2);

  // Set up basic random number generation.
  srand((unsigned)memdc);

  // No timer currently running.
  timerSet = false;

  // No bouncing line currently shown.
  lineEnabled = true;
  lineRunning = false;
  start.x = start.y = 0;
  vs_x = vs_y = 0;
  end.x = end.y = 0;
  ve_x = ve_y = 0;

  // No cube currently shown.
  cubeEnabled = true;
  cubeRunning = false;
  dx = dy = dz = 0;
  rx = ry = rz = 0;

  // No text currently shown.
  textEnabled = true;
  textRunning = false;
  textPos.x = textPos.y = 0;
  textFont = CreateFont(48, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"System");
  addLine(L"My");
  addLine(L"password");
  addLine(L"is");
  addLine(L"-redacted-");
  textLineIndex = 0;

}

PixelDisplay::~PixelDisplay() {
  stopAnimation();
  DeleteObject(memdc);
}

void PixelDisplay::reset()
{
  BitBlt(memdc, 0, 0, width, height, NULL, 0, 0, BLACKNESS);
  SetROP2(memdc, R2_NOT);
  SelectObject(memdc, GetStockObject(WHITE_PEN));

  SelectObject(textdc, textFont);
  SelectObject(textdc, GetStockObject(WHITE_PEN));
  SetTextColor(textdc, RGB(255, 255, 255));
  SetTextAlign(textdc, TA_TOP | TA_CENTER);
  SetBkMode(textdc, TRANSPARENT);
}

void PixelDisplay::copyToDC(HDC hdc, HWND status)
{
  RECT rectMain, rectStatus;
  GetClientRect(visibleHwnd, &rectMain);
  GetClientRect(status, &rectStatus);
  StretchBlt(hdc, 0, 0, rectMain.right, rectMain.bottom - rectStatus.bottom, memdc, 0, 0, width, height, SRCCOPY);
  if (eraseBackground) BitBlt(memdc, 0, 0, width, height, NULL, 0, 0, BLACKNESS);
}

void PixelDisplay::addRandomLines(int n)
{
  while (n--) addOneRandomLine();
  invalidate();
}

void PixelDisplay::startAnimation()
{
  // Returns non-zero on success.
  if (SetTimer(visibleHwnd, timerId, 16, NULL)) timerSet = true;
  if (lineEnabled) {
    if (start.x == 0 && start.y == 0) resetLine();
    lineRunning = true;
  }
  if (cubeEnabled) {
    if (dx == 0 && dy == 0) resetCube();
    cubeRunning = true;
  }
  if (textEnabled) {
    if (textPos.x == 0 && textPos.y == 0) resetText();
    textRunning = true;
  }
}

void PixelDisplay::stopAnimation()
{
  if (timerSet) KillTimer(visibleHwnd, timerId);
  timerSet = false;
  lineRunning = false;
  cubeRunning = false;
}

void PixelDisplay::toggleAnimation()
{
  if (timerSet) {
    stopAnimation();
  } else {
    startAnimation();
  }
}

void PixelDisplay::nextFrame()
{
  if (lineEnabled && lineRunning) {
    moveLine();
    drawLine();
  }
  if (cubeEnabled && cubeRunning) {
    moveCube();
    drawCube();
  }
  if (textEnabled && textRunning) {
    moveText();
    drawText();
  }
  frameCount++;
  invalidate();
}

void PixelDisplay::resetLine()
{
  start.x = 5;
  start.y = 0;
  vs_x = getRandom(5) + 1;
  vs_y = getRandom(5) + 1;

  end.x = 0;
  end.y = 5;
  ve_x = getRandom(5) + 1;
  ve_y = getRandom(5) + 1;

  dx = dy = 0;
  dz = 0;
}

void PixelDisplay::moveLine()
{
  // Update the position of each endpoint based on their speeds.
  start.x += vs_x;
  start.y += vs_y;
  end.x += ve_x;
  end.y += ve_y;

  // Bounce the endpoints if they reach the edge of the window.
  if (start.x < 0 || start.x > width) vs_x = -vs_x;
  if (start.y < 0 || start.y > height) vs_y = -vs_y;
  if (end.x < 0 || end.x > width) ve_x = -ve_x;
  if (end.y < 0 || end.y > height) ve_y = -ve_y;

}

void PixelDisplay::drawLine()
{
  MoveToEx(memdc, start.x, start.y, NULL);
  LineTo(memdc, end.x, end.y);
}

void PixelDisplay::resetCube()
{
  dx = dy = 0;
  dz = 800;
  rx = ry = rz = 0;
}

void PixelDisplay::moveCube()
{
  rx += 0.04;
  ry += 0.03;
  rz += 0.02;
}

void PixelDisplay::drawCube()
{
  Cube t(cube);
  t.rotate(rx, ry, rz);
  t.translate(dx, dy, dz);
  t.draw(memdc, 100.0, width, height);
}

void PixelDisplay::translateCube(double _x, double _y, double _z)
{
  dx += _x;
  dy += _y;
  dz += _z;
}

size_t PixelDisplay::addLine(std::wstring line)
{
  lines.push_back(line);
  return lines.size();
}

void PixelDisplay::resetText()
{
  textPos.x = 20;
  textPos.y = height;
  textVelocity.x = 0;
  textVelocity.y = -2;
}

void PixelDisplay::moveText()
{
  // Temporarily removed this - having the text static seems easier to read.
  //// Update the position of the text based on its speed.
  //textPos.x += textVelocity.x;
  //textPos.y += textVelocity.y;

  //// When the text gets to the top, move it back to the bottom and move on to the next line.
  //if (textPos.y < -20) {
  //  textPos.y = height;
  //  textLineIndex++;
  //  if (textLineIndex == lines.size()) textLineIndex = 0;
  //}
}

void PixelDisplay::nextLine() {
  textLineIndex++;
  if (textLineIndex == lines.size()) textLineIndex = 0;
}

void PixelDisplay::drawText()
{
  // Draw a few frames of text in the middle of the screen every 60 frames (ie every second).
  auto mod = frameCount % 60;
  if (mod > 0 && mod < 5) {
    BitBlt(textdc, 0, 0, width, height, NULL, 0, 0, BLACKNESS);
    auto line = lines[textLineIndex].c_str();
    TextOut(textdc, width / 2, height / 2, line, wcslen(line));
    BitBlt(memdc, 0, 0, width, height, textdc, 0, 0, SRCINVERT);
  } else if (mod == 0) nextLine();
  
}

std::wstring PixelDisplay::getStatusMessage()
{
  std::wstringstream message;
  message << L"Animation: ";
  message << (timerSet ? L"Yes" : L"No");
  message << L", Background erase?: ";
  message << (eraseBackground ? L"Yes" : L"No");
  message << L", Line enabled: ";
  message << (lineEnabled ? L"Yes" : L"No");
  message << L", Cube enabled: ";
  message << (cubeEnabled ? L"Yes" : L"No");
  message << L", Text enabled: ";
  message << (textEnabled ? L"Yes" : L"No");
  return message.str();
}


void PixelDisplay::addOneRandomLine() {
  // Draw a line from somewhere on the left of the screen to somewhere on the right,
  // not entirely randomly.
  int margin = width / 5;
  MoveToEx(memdc, getRandom(margin), getRandom(height), NULL);
  LineTo(memdc, width - getRandom(margin), getRandom(height));
}
