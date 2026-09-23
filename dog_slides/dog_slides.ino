/*
  Dog Slides — T-Display S3

  BOOT button (GPIO0)  -> advances to the next slide (5 total, wraps around)
  USER button (GPIO14) -> power button: press to sleep (screen goes black,
                           chip enters deep sleep), press again to wake up.
                           NOTE: this is deep sleep, not true hardware power
                           off -- it's the closest thing possible without
                           extra circuitry, and it does drop power draw a
                           lot. Waking always restarts the sketch fresh from
                           slide 0, since deep sleep doesn't preserve RAM.

  Every slide uses the same layout: a big dog anchored bottom-left, huge
  bold text filling the right side -- matching the "HI" slide's proportions
  throughout. Each slide's text is a different shade of brown. A solid
  battery icon with the percentage printed inside it sits top-right on
  every slide (read from the board's onboard voltage divider on GPIO4) --
  green normally, red once it drops below 20%.

  Slide 0: "HI" with a BIG waving corgi.
  Slide 1: "REDBULL / UNTIL / FRIYAY" -- sarcastic, dog going from tired to
           full meltdown-crying.
  Slide 2: "SMILE! / IT'S / FRIYAY!" -- happy blinking dog.
  Slide 3: "1% BATT / 100% MAIN / ICONIC / ENERGY" -- confident running loop.
  Slide 4: "ERROR 404 / BAD VIBES / NOT FOUND / MOVING ON" -- running loop.

  A note on sizing: text and dog share the same 320px-wide screen, so there's
  a hard physical ceiling on how big both can get together. Words like "NOT
  FOUND" and "BATTERY" are already near that ceiling at the current size --
  going bigger still would mean cutting words down further or splitting them
  across more lines. Let me know if you'd rather trade dog size for text
  size (or vice versa) and I can rebalance it.
*/

#include <TFT_eSPI.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include "sprite_wave_big.h"
#include "sprite_redbull.h"
#include "sprite_smile.h"
#include "sprite_battery.h"
#include "sprite_error404.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite dogBuf = TFT_eSprite(&tft);

#define BTN_SLIDE 0   // BOOT button
#define BTN_POWER 14  // USER button
#define BATT_ADC_PIN 4 // onboard voltage-divider pin for reading battery voltage

int W, H;

uint16_t bgColor() { return tft.color565(255, 236, 214); }

struct Anim {
  const uint16_t* const* frames;
  int frameCount;
  int w, h;
  int frameDelay;
};

Anim animWave    = { wave_big_frames, WAVE_BIG_FRAMES, WAVE_BIG_W, WAVE_BIG_H, 160 };
Anim animRedbull = { redbull_frames,  REDBULL_FRAMES,  REDBULL_W,  REDBULL_H,  160 };
Anim animSmile   = { smile_frames,    SMILE_FRAMES,    SMILE_W,    SMILE_H,    220 };
Anim animBattery = { battery_frames,  BATTERY_FRAMES,  BATTERY_W,  BATTERY_H,  110 };
Anim animError   = { error404_frames, ERROR404_FRAMES, ERROR404_W, ERROR404_H, 110 };

struct Slide {
  const char* lines[5];
  int lineCount;
  Anim* anim;
  int bufSize;   // offscreen buffer size for this slide's dog
  bool bigHi;    // slide 0's extra-large single-word treatment
  uint8_t colR, colG, colB; // this slide's text color (a different brown each)
};

Slide slides[] = {
  { {"HI", "", "", "", ""},                                     1, &animWave,    160, true,  200,105, 35 },
  { {"REDBULL", "UNTIL", "FRIYAY", "", ""},                     3, &animRedbull, 130, false, 139, 69, 19 },
  { {"SMILE!", "IT'S", "FRIYAY!", "", ""},                      3, &animSmile,   130, false, 184,134, 11 },
  { {"1% BATT", "100% MAIN", "ICONIC", "ENERGY", ""},           4, &animBattery, 130, false, 101, 67, 33 },
  { {"ERROR 404", "BAD VIBES", "NOT FOUND", "MOVING ON", ""},   4, &animError,   130, false,  90, 50, 30 },
};
const int slideCount = sizeof(slides) / sizeof(slides[0]);
int slideIndex = 0;

int lastDrawnIndex = -1;
int dogFrame = 0;
unsigned long lastFrameTime = 0;
unsigned long lastBattUpdate = 0;
int battPercent = 100;

void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  pinMode(BTN_SLIDE, INPUT_PULLUP);
  pinMode(BTN_POWER, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  W = tft.width();
  H = tft.height();

  dogBuf.setColorDepth(16);
  dogBuf.createSprite(160, 160); // resized per-slide in loop() as needed
  dogBuf.setSwapBytes(true);

  tft.fillScreen(bgColor());
  drawSlideStatic();
  lastBattUpdate = 0; // force an immediate first battery read/draw
}

void loop() {
  handleButtons();
  unsigned long now = millis();

  if (slideIndex != lastDrawnIndex) {
    tft.fillScreen(bgColor());
    drawSlideStatic();
    Slide& s = slides[slideIndex];
    dogBuf.deleteSprite();
    dogBuf.createSprite(s.bufSize, s.bufSize); // exact fit -- no leftover padding to overlap text
    dogBuf.setSwapBytes(true);
    dogFrame = 0;
    lastFrameTime = now;
    lastDrawnIndex = slideIndex;
    lastBattUpdate = 0; // screen was just wiped, redraw the icon immediately
  }

  drawAnimatedDog(now);
  drawBatteryIcon(now);
  delay(20);
}

void handleButtons() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress < 300) return;

  if (digitalRead(BTN_SLIDE) == LOW) {
    slideIndex = (slideIndex + 1) % slideCount;
    lastPress = millis();
  } else if (digitalRead(BTN_POWER) == LOW) {
    lastPress = millis();
    enterSleep();
  }
}

void enterSleep() {
  tft.fillScreen(TFT_BLACK);

  // wait for release so we don't instantly wake back up
  while (digitalRead(BTN_POWER) == LOW) delay(10);
  delay(50);

  rtc_gpio_pullup_en((gpio_num_t)BTN_POWER);
  rtc_gpio_pulldown_dis((gpio_num_t)BTN_POWER);
  esp_sleep_enable_ext1_wakeup(1ULL << BTN_POWER, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start(); // resets and re-runs setup() on wake
}

// ---- static per-slide content: same dog-left / big-text-right layout as
// the HI slide, for every slide ----
void drawSlideStatic() {
  Slide& s = slides[slideIndex];
  int dogRight = 4 + s.bufSize;
  int textX = dogRight + (W - dogRight) / 2;

  uint16_t color = tft.color565(s.colR, s.colG, s.colB);
  int lineH = s.bigHi ? 78 : 30; // bigHi renders at 3x font-4 scale (~78px tall)
  int blockH = s.lineCount * lineH;
  int startY = (H - blockH) / 2;

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color, bgColor());
  if (s.bigHi) tft.setTextSize(3);
  for (int i = 0; i < s.lineCount; i++) {
    int y = startY + i * lineH;
    // heavier bold: three overlapping passes instead of one offset copy
    tft.drawString(s.lines[i], textX, y, 4);
    tft.drawString(s.lines[i], textX + 1, y, 4);
    tft.drawString(s.lines[i], textX + (s.bigHi ? 2 : 2), y, 4);
  }
  if (s.bigHi) tft.setTextSize(1);
}

// ---- animated dog, bottom-left corner, flicker-free via offscreen buffer ----
void drawAnimatedDog(unsigned long now) {
  Slide& s = slides[slideIndex];
  Anim* a = s.anim;

  if (now - lastFrameTime > a->frameDelay) {
    dogFrame = (dogFrame + 1) % a->frameCount;
    lastFrameTime = now;
  }

  int buf = s.bufSize;
  int dogX = 4;
  int dogY = H - buf - 2;

  dogBuf.fillSprite(bgColor());
  int ix = (buf - a->w) / 2;
  int iy = buf - a->h; // bottom-align within its own buffer
  dogBuf.pushImage(ix, iy, a->w, a->h, (uint16_t*)a->frames[dogFrame]);
  dogBuf.pushSprite(dogX, dogY);
}

// ---- battery icon, top-right, on every slide ----
int readBatteryPercent() {
  // T-Display S3 exposes battery voltage on GPIO4 through an onboard 2:1
  // divider. This is a simple voltage->percent estimate, not a lab-grade
  // fuel gauge -- LiPo voltage sag under load can throw it off a bit, and
  // the min/max below may need tweaking for your specific cell.
  const int BATT_MIN_MV = 3300; // ~0%
  const int BATT_MAX_MV = 4200; // ~100%
  long sum = 0;
  const int SAMPLES = 8;
  for (int i = 0; i < SAMPLES; i++) sum += analogReadMilliVolts(BATT_ADC_PIN);
  int mv = (sum / SAMPLES) * 2; // account for the onboard divider
  int pct = (int)(100.0 * (mv - BATT_MIN_MV) / (BATT_MAX_MV - BATT_MIN_MV));
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

void drawBatteryIcon(unsigned long now) {
  if (lastBattUpdate != 0 && now - lastBattUpdate < 3000) return; // refresh every 3s
  lastBattUpdate = now;
  battPercent = readBatteryPercent();

  int x = W - 50, y = 4; // top-right corner
  int bw = 42, bh = 16;  // battery body size (wide enough for "100%" inside)
  int nub = 3;

  tft.fillRect(x - 2, y - 2, bw + nub + 4, bh + 4, bgColor()); // clear just this corner

  bool low = battPercent < 20;
  uint16_t fillColor = low ? TFT_RED : tft.color565(60, 170, 90);
  uint16_t outline = tft.color565(90, 70, 55);

  tft.fillRoundRect(x, y, bw, bh, 3, fillColor);
  tft.drawRoundRect(x, y, bw, bh, 3, outline);
  tft.fillRect(x + bw, y + bh / 2 - 3, nub, 6, outline); // terminal nub

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, fillColor);
  tft.drawString(String(battPercent) + "%", x + bw / 2, y + bh / 2 + 1, 1);
}
