#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>

#define PIN_CS   15
#define PIN_DC   2
#define PIN_RST  4
Adafruit_ILI9341 display(PIN_CS, PIN_DC, PIN_RST);

#define PIN_XP 27
#define PIN_XM 15
#define PIN_YP 4
#define PIN_YM 14

#define PRESS_MIN 10
#define PRESS_MAX 1000

TouchScreen touch(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 300);
int tx, ty;

#define TOUCH_LEFT 380
#define TOUCH_RIGHT 3200
#define TOUCH_TOP 2500
#define TOUCH_BOTTOM 400

enum UIState {
  SPLASH,
  DASHBOARD,
  COLOR_RED,
  COLOR_ORANGE,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_MAGENTA,
  OPEN_ANIM,
  CLOSE_ANIM
};

UIState screenState = SPLASH;
UIState nextScreen = COLOR_RED;

bool dashboardDrawn = false;
bool appDrawn = false;
int frameCounter = 0;
#define TOTAL_FRAMES 12

#define ICON_SIZE 60
#define NAV_HEIGHT 40
#define NAV_TOP 280

struct Icon {
  int x, y;
  uint16_t color;
  UIState state;
  const char* name;
};

Icon icons[] = {
  {25, 60, ILI9341_RED, COLOR_RED, "RED"},
  {135, 60, ILI9341_ORANGE, COLOR_ORANGE, "ORA"},
  {25, 140, ILI9341_YELLOW, COLOR_YELLOW, "YEL"},
  {135, 140, ILI9341_GREEN, COLOR_GREEN, "GRE"},
  {25, 220, ILI9341_BLUE, COLOR_BLUE, "BLU"},
  {135, 220, ILI9341_MAGENTA, COLOR_MAGENTA, "MAG"}
};

#define ICON_COUNT (sizeof(icons)/sizeof(Icon))

bool readTouch() {
  TSPoint p = touch.getPoint();
  pinMode(PIN_YP, OUTPUT);
  pinMode(PIN_XM, OUTPUT);
  if (p.z > PRESS_MIN && p.z < PRESS_MAX) {
    tx = map(p.x, TOUCH_LEFT, TOUCH_RIGHT, 0, 240);
    ty = map(p.y, TOUCH_TOP, TOUCH_BOTTOM, 0, 320);
    return true;
  }
  return false;
}

uint16_t gradientColor(int y) {
  int blue = map(y, 0, 320, 10, 31);
  return display.color565(0, 0, blue*8);
}

void drawNavigation(uint16_t bg, uint16_t fg) {
  display.fillRect(0, NAV_TOP, 240, NAV_HEIGHT, bg);
  display.drawCircle(120, NAV_TOP + 20, 10, fg); // Home button
  display.drawTriangle(40, NAV_TOP + 20, 50, NAV_TOP + 10, 50, NAV_TOP + 30, fg); // Back
}

void showSplash() {
  display.fillScreen(ILI9341_BLACK);
  display.setTextColor(ILI9341_WHITE);
  display.setTextSize(4);
  display.setCursor(40, 140);
  display.print("AmyOS");
}

void showDashboard() {
  if (dashboardDrawn) return;

  for (int y=0; y<320; y+=4)
    display.fillRect(0, y, 240, 4, gradientColor(y));

  for (int i=0; i<ICON_COUNT; i++) {
    display.fillRoundRect(icons[i].x, icons[i].y, ICON_SIZE, ICON_SIZE, 8, icons[i].color);
    display.setTextColor(ILI9341_WHITE);
    display.setTextSize(2);
    display.setCursor(icons[i].x+12, icons[i].y+22);
    display.print(icons[i].name);
  }

  display.setTextSize(3);
  display.setCursor(10, 10);
  display.print("AmyOS");

  drawNavigation(gradientColor(300), ILI9341_WHITE);
  dashboardDrawn = true;
}

void showAppScreen(uint16_t color, const char* name) {
  if (appDrawn) return;
  display.fillScreen(color);
  display.setTextColor(ILI9341_WHITE);
  display.setTextSize(4);
  display.setCursor(50, 140);
  display.print(name);
  drawNavigation(ILI9341_BLACK, ILI9341_WHITE);
  appDrawn = true;
}

void openAnimation() {
  float progress = (float)frameCounter / TOTAL_FRAMES;
  int size = progress * 320;
  int idx = nextScreen - COLOR_RED;
  display.fillRect(120 - size/2, 160 - size/2, size, size, icons[idx].color);
  frameCounter++;
  if (frameCounter > TOTAL_FRAMES) {
    screenState = nextScreen;
    frameCounter = 0;
    appDrawn = false;
  }
}

void closeAnimation() {
  display.fillScreen(ILI9341_BLACK);
  screenState = DASHBOARD;
  dashboardDrawn = false;
}

void dashboardTouch() {
  if (!readTouch()) return;
  if (ty > NAV_TOP && abs(tx-120)<15) return;
  for (int i=0; i<ICON_COUNT; i++) {
    if (tx > icons[i].x && tx < icons[i].x+ICON_SIZE &&
        ty > icons[i].y && ty < icons[i].y+ICON_SIZE) {
      nextScreen = icons[i].state;
      screenState = OPEN_ANIM;
      frameCounter = 0;
    }
  }
}

void appTouch() {
  if (!readTouch()) return;
  if (ty > NAV_TOP && abs(tx-120)<15) screenState = CLOSE_ANIM;
}

void setup() {
  display.begin();
  display.setRotation(0);
  showSplash();
}

void loop() {
  switch(screenState) {
    case SPLASH:
      delay(2000);
      screenState = DASHBOARD;
      break;

    case DASHBOARD:
      showDashboard();
      dashboardTouch();
      break;

    case OPEN_ANIM:
      openAnimation();
      break;

    case CLOSE_ANIM:
      closeAnimation();
      break;

    default:
      if (screenState >= COLOR_RED && screenState <= COLOR_MAGENTA) {
        int idx = screenState - COLOR_RED;
        showAppScreen(icons[idx].color, icons[idx].name);
        appTouch();
      }
      break;
  }
}
