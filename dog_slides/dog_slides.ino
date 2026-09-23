/*
  Dog Slides — T-Display S3 — SMOOTH ANIMATION + CRISP VECTOR FONTS
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

// BACKLIGHT CONFIGURATION (Using safe hardware PWM dimming only)
#define BACKLIGHT_PIN 15
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8 

// Set to 90 (35% brightness) to safely cut battery drain without glitching your code loops
const int BRIGHTNESS_LEVEL = 90; 

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
  int bufSize;   
  bool bigHi;    
  uint8_t colR, colG, colB; 
};

Slide slides[] = {
  { {"HI!", "", "", "", ""},                                     1, &animWave,    160, true,  200,105, 35 },
  { {"REDBULL", "UNTIL", "FRIYAY.", "", ""},                     3, &animRedbull, 130, false, 139, 69, 19 },
  { {"SMILE!", "IT'S", "FRIYAY!", "", ""},                      3, &animSmile,   130, false, 184,134, 11 },
  { {"1% BATTERY!", "100% MAIN", "CHARACTER", "ENERGY!", ""},    4, &animBattery, 130, false, 101, 67, 33 },
  { {"ERROR 404:", "BAD VIBES", "NOT FOUND!", "MOVING ON!", ""}, 4, &animError,   130, false,  90, 50, 30 },
};

const int slideCount = sizeof(slides) / sizeof(slides[0]);
int slideIndex = 0;

int lastDrawnIndex = -1;
int dogFrame = 0;
unsigned long lastFrameTime = 0;
unsigned long lastBattUpdate = 0;
int battPercent = 100;

void drawSlideStatic();
void handleButtons();
bool drawAnimatedDog(unsigned long now);
void drawBatteryIcon(unsigned long now);
int readBatteryPercent();
void enterSleep();

void setup() {
  // Safe energy speed
  setCpuFrequencyMhz(80);

  // Clean PWM dimming initialization
  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(BACKLIGHT_PIN, BRIGHTNESS_LEVEL); 

  pinMode(BTN_SLIDE, INPUT_PULLUP);
  pinMode(BTN_POWER, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  W = tft.width();
  H = tft.height();

  dogBuf.setColorDepth(16);
  dogBuf.createSprite(160, 160); 
  dogBuf.setSwapBytes(true);

  tft.fillScreen(bgColor());
  drawSlideStatic();
  lastBattUpdate = 0; 
}

void loop() {
  handleButtons();
  unsigned long now = millis();

  if (slideIndex != lastDrawnIndex) {
    tft.fillScreen(bgColor());
    drawSlideStatic();
    Slide& s = slides[slideIndex];
    dogBuf.deleteSprite();
    dogBuf.createSprite(s.bufSize, s.bufSize); 
    dogBuf.setSwapBytes(true);
    dogFrame = 0;
    lastFrameTime = now;
    lastDrawnIndex = slideIndex;
    lastBattUpdate = 0; 
  }

  // Micro-pacing render handler
  bool frameChanged = drawAnimatedDog(now);
  if (frameChanged) {
    drawBatteryIcon(now);
  }
  
  // Dropped to 5ms to allow ultra-smooth sub-frame step timing without loop latency
  delay(5); 
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
  ledcWrite(BACKLIGHT_PIN, 0); 
  tft.fillScreen(TFT_BLACK);
  
  while (digitalRead(BTN_POWER) == LOW) delay(10);
  delay(50);

  rtc_gpio_pullup_en((gpio_num_t)BTN_POWER);
  rtc_gpio_pulldown_dis((gpio_num_t)BTN_POWER);
  esp_sleep_enable_ext1_wakeup(1ULL << BTN_POWER, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start(); 
}

void drawSlideStatic() {
  Slide& s = slides[slideIndex];
  int dogRight = 4 + s.bufSize;
  int textWidth = W - dogRight;
  int textCenter = dogRight + textWidth / 2;

  uint16_t color = tft.color565(s.colR, s.colG, s.colB);
  int lineH = s.bigHi ? 78 : 34; 
  int blockH = s.lineCount * lineH;
  int startY = ((H - blockH) / 2) + 20;

  tft.setTextColor(color, bgColor());

  for (int i = 0; i < s.lineCount; i++) {
    int y = startY + i * lineH;
    
    if (s.bigHi) {
      tft.setTextDatum(TC_DATUM); 
      tft.setFreeFont(NULL); 
      tft.setTextSize(3);
      tft.drawString(s.lines[i], textCenter, y - 20, 4); 
      tft.drawString(s.lines[i], textCenter + 1, y - 20, 4);
      tft.drawString(s.lines[i], textCenter + 2, y - 20, 4);
      tft.setTextSize(1);
    } else {
      tft.setFreeFont(&FreeSansBold12pt7b); 
      tft.setTextSize(1); 
      
      tft.setTextDatum(TL_DATUM); 
      int strW = tft.textWidth(s.lines[i]);
      int calculatedX = textCenter - (strW / 2);
      
      tft.drawString(s.lines[i], calculatedX, y); 
    }
  }
  tft.setFreeFont(NULL);
}

// OPTIMIZED: Uses non-blocking differential delta matching to cleanly step frames smoothly
bool drawAnimatedDog(unsigned long now) {
  Slide& s = slides[slideIndex];
  Anim* a = s.anim;

  if (now - lastFrameTime >= a->frameDelay) {
    dogFrame = (dogFrame + 1) % a->frameCount;
    // Advance tracking using exact mathematical intervals to prevent jitter
    lastFrameTime += a->frameDelay; 

    int buf = s.bufSize;
    int dogX = 4;
    int dogY = H - buf - 2;

    dogBuf.fillSprite(bgColor());
    int ix = (buf - a->w) / 2;
    int iy = buf - a->h; 
    dogBuf.pushImage(ix, iy, a->w, a->h, (uint16_t*)a->frames[dogFrame]);
    dogBuf.pushSprite(dogX, dogY);
    
    return true;
  }
  return false;
}

int readBatteryPercent() {
  const int BATT_MIN_MV = 3300; 
  const int BATT_MAX_MV = 4200; 
  long sum = 0;
  const int SAMPLES = 8;
  for (int i = 0; i < SAMPLES; i++) sum += analogReadMilliVolts(BATT_ADC_PIN);
  int mv = (sum / SAMPLES) * 2; 
  int pct = (int)(100.0 * (mv - BATT_MIN_MV) / (BATT_MAX_MV - BATT_MIN_MV));
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

void drawBatteryIcon(unsigned long now) {
  if (lastBattUpdate != 0 && now - lastBattUpdate < 3000) return; 
  lastBattUpdate = now;
  battPercent = readBatteryPercent();

  int x = W - 50, y = 4; 
  int bw = 42, bh = 16;  
  int nub = 3;

  tft.fillRect(x - 2, y - 2, bw + nub + 4, bh + 4, bgColor()); 

  bool low = battPercent < 20;
  uint16_t fillColor = low ? TFT_RED : tft.color565(60, 170, 90);
  uint16_t outline = tft.color565(90, 70, 55);

  tft.fillRoundRect(x, y, bw, bh, 3, fillColor);
  tft.drawRoundRect(x, y, bw, bh, 3, outline);
  tft.fillRect(x + bw, y + bh / 2 - 3, nub, 6, outline); 

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, fillColor);
  tft.drawString(String(battPercent) + "%", x + bw / 2, y + bh / 2 + 1, 1);
}
