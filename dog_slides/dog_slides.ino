/*
  Dog Slides — T-Display S3 — POWER SAVING + BACKLIGHT DIMMING + CRISP VECTOR FONT
*/

#include <TFT_eSPI.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "esp_wifi.h"
#include "esp_bt.h"

#include "sprite_wave_big.h"
#include "sprite_smile.h"
#include "sprite_battery.h"
#include "sprite_jump.h"
#include "sprite_thinking.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite dogBuf = TFT_eSprite(&tft);

#define BTN_SLIDE 0   // BOOT button
#define BTN_POWER 14  // USER button
#define BATT_ADC_PIN 4 // onboard voltage-divider pin for reading battery voltage

// BACKLIGHT CONFIGURATION
#define BACKLIGHT_PIN 15
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8 // 8-bit means values range from 0 to 255

// Set brightness here: 0 (Off) to 255 (Full Brightness)
// 90 is roughly 35% brightness -- dimmer than 115, better for battery life
const int BRIGHTNESS_LEVEL = 90;

int W, H;

uint16_t bgColor() { return tft.color565(255, 236, 214); }

struct Anim {
  const uint16_t* const* frames;
  int frameCount;
  int w, h;
  int frameDelay;
};

// Frame delays slowed down further across the board for a calmer animation pace
Anim animWave     = { wave_big_frames, WAVE_BIG_FRAMES, WAVE_BIG_W, WAVE_BIG_H, 270 };
Anim animJump     = { jump_frames,     JUMP_FRAMES,     JUMP_W,     JUMP_H,     130 };
Anim animSmile    = { smile_frames,    SMILE_FRAMES,    SMILE_W,    SMILE_H,    360 };
Anim animBattery  = { battery_frames,  BATTERY_FRAMES,  BATTERY_W,  BATTERY_H,  90 };
Anim animThinking = { thinking_frames, THINKING_FRAMES, THINKING_W, THINKING_H, 310 };

struct Slide {
  const char* lines[5]; // Restored multi-line array brackets
  int lineCount;
  Anim* anim;
  int bufSize;   
  bool bigHi;    
  uint8_t colR, colG, colB; 
};

// Text colours step through an orange -> dark brown gradient from the first slide to the last
Slide slides[] = {
  { {"HI!", "", "", "", ""},                                     1, &animWave,     160, true,  214,106, 30 },
  { {"REDBULL", "UNTIL", "FRIYAY.", "", ""},                     3, &animJump,     130, false, 179, 90, 28 },
  { {"SMILE!", "IT'S", "FRIYAY!", "", ""},                      3, &animSmile,    130, false, 143, 73, 25 },
  { {"1% BATTERY!", "100% MAIN", "CHARACTER", "ENERGY!", ""},    4, &animBattery,  130, false, 108, 57, 23 },
  { {"ERROR 404:", "BAD VIBES", "NOT FOUND!", "MOVING ON!", ""}, 4, &animThinking, 130, false,  72, 40, 20 },
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
  setCpuFrequencyMhz(80);

  // Radios are never used here but can silently draw current if left enabled — kill them
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Let the CPU drop into light sleep during idle ticks (between frames, between button polls)
  // instead of spinning at full clock. No effect on animation timing or button latency.
  esp_pm_config_esp32s3_t pm_config = {
    .max_freq_mhz = 80,
    .min_freq_mhz = 10,
    .light_sleep_enable = true
  };
  esp_pm_configure(&pm_config);

  // Native ESP32 Core v3.0+ hardware PWM system dimming syntax
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

  bool frameChanged = drawAnimatedDog(now);
  if (frameChanged) {
    drawBatteryIcon(now);
  }
  
  delay(10); 
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
  
  // Adjusted spacing for standard font vs vector text structures
  int lineH = s.bigHi ? 78 : 34; 
  int blockH = s.lineCount * lineH;
  
  // Shifted starting Y position down globally by adding 20px to move text under the battery icon
  int startY = ((H - blockH) / 2) + 20;

  tft.setTextColor(color, bgColor());

  for (int i = 0; i < s.lineCount; i++) {
    int y = startY + i * lineH;
    
    if (s.bigHi) {
      // Slide 0: Crisp original Font 4 behavior
      tft.setTextDatum(TC_DATUM); 
      tft.setFreeFont(NULL); 
      tft.setTextSize(3);
      tft.drawString(s.lines[i], textCenter, y - 20, 4); // Standard center placement
      tft.drawString(s.lines[i], textCenter + 1, y - 20, 4);
      tft.drawString(s.lines[i], textCenter + 2, y - 20, 4);
      tft.setTextSize(1);
    } else {
      // Slides 1-4: Clean compilation structure for the beautiful GFX vector bold text
      tft.setFreeFont(&FreeSansBold12pt7b); 
      tft.setTextSize(1); 
      
      // Calculate layout text width using left alignment constraints
      tft.setTextDatum(TL_DATUM); 
      int strW = tft.textWidth(s.lines[i]);
      int calculatedX = textCenter - (strW / 2);
      
      tft.drawString(s.lines[i], calculatedX, y); 
    }
  }
  
  // Clean up: Reset back to system standard so it won't distort the battery icon font
  tft.setFreeFont(NULL);
}

bool drawAnimatedDog(unsigned long now) {
  Slide& s = slides[slideIndex];
  Anim* a = s.anim;

  if (now - lastFrameTime > a->frameDelay) {
    dogFrame = (dogFrame + 1) % a->frameCount;
    lastFrameTime = now;

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
