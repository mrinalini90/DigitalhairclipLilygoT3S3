# Digital Hair Clip

A LilyGo T-Display S3 wired up as a wearable, battery-powered hair clip that cycles through corgi animations.

## What it does

Five slides, cycled with a button press:

1. **HI!** — waving corgi
2. **REDBULL UNTIL FRIYAY** — jumping corgi
3. **SMILE! IT'S FRIYAY!** — happy blinking loop
4. **1% BATTERY, 100% MAIN CHARACTER ENERGY** — running loop
5. **ERROR 404: BAD VIBES NOT FOUND** — corgi thinking it over

Battery percentage shown top-right on every slide (green ≥20%, red below).

### Controls

| Button | Function |
|---|---|
| **BOOT** (GPIO0) | Next slide |
| **USER** (GPIO14) | Sleep / wake |

Waking from sleep restarts at slide 1 (deep sleep doesn't preserve RAM).

---

## Building one

### What you need

- LilyGo T-Display S3 (1.9" ST7789, ESP32-S3), in its case
- 3.7V 220mAh LiPo battery, 402030-size, 2-pin JST connector
- Small screwdriver set
- Electronics-safe double-sided sticky tape
- Acrylic sheet, 2-3mm thick
- Alligator clip (metal, spring-loaded)

### Steps

1. Flash the firmware first (see [Installation](#installation)) — easier to debug before assembly.
2. Open the case and take out the bare board.
3. Connect the battery. **Check polarity before plugging in** — reversed polarity can destroy the board and/or battery.
4. Cut the acrylic sheet to roughly the board's size — this is the rigid backing.
5. Tape the alligator clip to the back of the acrylic, jaws facing outward. This is what grips the hair; the acrylic just gives it a rigid mount.
6. Tape the board and battery to the front of the acrylic.
7. Check the alligator clip's jaws still open/close freely, and the USB-C port is still reachable.

Clip it into your hair, BOOT to cycle slides, power button to sleep.

### The finished build

<p align="center">
  <img src="docs/photos/clip_side_profile.jpg" width="32%" alt="Side profile of the assembled clip" />
  <img src="docs/photos/screen_running.jpg" width="32%" alt="The clip with the HI slide running" />
  <img src="docs/photos/clip_closed_edge.jpg" width="32%" alt="Edge-on view of the clip closed" />
</p>

<p align="center">
  <video src="docs/demo_clip.mp4" controls width="480">
    <a href="docs/demo_clip.mp4">Download the clip</a>
  </video>
</p>

---

## Software notes

Single Arduino sketch (`dog_slides/dog_slides.ino`) using `TFT_eSPI`. Each animation is a small array of raw 16-bit colour frames compiled into the firmware — no SD card, no filesystem.

### Art source

The corgi spritesheet used for the "jump" and "thinking" animations came from a free asset site found online — exact source unrecorded, not original work. Frames were sliced out of it programmatically (bounding-box detection, background flattening, conversion to the display's raw colour format).

<p align="center">
  <img src="docs/photos/corgi_spritesheet_source.webp" width="45%" alt="Source corgi spritesheet" />
</p>
<p align="center"><em>Source spritesheet — not mine, found free online.</em></p>

### Battery optimizations

- **Animation frame rate kept low.** Redrawing the screen (SPI transfer) is the single biggest power cost in this firmware — bigger than the backlight or the CPU. Slower animations were chosen deliberately for battery life, not just style.
- **Backlight dimmed to ~35%** (`BRIGHTNESS_LEVEL = 90/255`).
- **CPU clocked down to 80MHz** instead of 240MHz.
- **Wi-Fi/Bluetooth powered off** at boot — unused, but can silently draw current if left on.
- **Automatic CPU light sleep** between frames/button polls — no effect on responsiveness.

---

## Battery performance: on vs. deep sleep

Measured on the physical device: roughly **16% drained per 10 minutes** switched on and animating, versus roughly **9% per 1.5 hours** in deep sleep. Projected to a full 100%→0% runtime:

![Estimated battery runtime: switched on vs switched off](docs/battery_chart.png)

Left on continuously, the battery is gone in **about an hour**. Put to sleep between wears, it stretches to roughly **16-17 hours**. The power button isn't optional here — it's the difference between an hour of wear and a full day's standby.

---

## Installation

1. Install Arduino IDE (2.x) from [arduino.cc](https://www.arduino.cc/en/software).
2. Add the ESP32 board package: File → Preferences → "Additional boards manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   Then Tools → Board → Boards Manager → search "esp32" → install.
3. Install the display library: download [LilyGo's T-Display-S3 repo](https://github.com/Xinyuan-LilyGO/T-Display-S3), copy everything in its `lib/` folder into your Arduino `libraries/` folder. Don't let the IDE auto-update it later.
4. Clone this repo (keep the `dog_slides` folder intact — `.ino` and `.h` files need to stay together).
5. Open `dog_slides/dog_slides.ino` in Arduino IDE.
6. Board settings under Tools:
   - Board: `LilyGo T3-S3` (or `ESP32S3 Dev Module`)
   - USB CDC On Boot: `Enabled`
   - Flash Size: `16MB`
   - PSRAM: `OPI PSRAM`
   - Partition Scheme: `16M Flash (3MB APP/9.9MB FATFS)`
   - Upload Mode: `UART0/Hardware CDC`
   - USB Mode: `CDC and JTAG`
7. Connect via USB-C (data cable, not charge-only), select the port, click Upload.

If upload hangs on "Connecting....": hold **BOOT**, tap **RESET** once (still holding BOOT), start the upload, release BOOT once it starts writing.

## Uninstallation

- **Use the board for something else**: upload any other sketch over this one.
- **Wipe it back to blank**: Tools → Erase Flash → "All Flash Contents", then upload any sketch.
- **Remove from your computer**: delete the project folder (and the `TFT_eSPI` library bundle, if not needed elsewhere).

## Notes

- None of the corgi artwork is original — all of it came from pixel-art assets found online, source unrecorded. Personal, non-commercial project; happy to credit or remove on request.
- Battery percentage is a voltage-based estimate, not a lab-grade fuel gauge, and is noisy over short windows.
