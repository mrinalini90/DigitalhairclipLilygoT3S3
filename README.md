# Digital Hairclip — LilyGo T-Display S3

A tiny animated slideshow that turns a LilyGo T-Display S3 into a wearable digital hair clip. Built around a corgi spritesheet, it runs standalone off USB power or a small LiPo battery.

## What it is

Five slides, cycled with a button press:

1. **HI** — a big waving corgi with a huge bold greeting
2. **REDBULL UNTIL FRIYAY** — sarcastic, tired-to-meltdown-crying animation
3. **SMILE! IT'S FRIYAY!** — happy blinking loop
4. **1% BATTERY, 100% ICONIC ENERGY** — confident running loop
5. **ERROR 404: BAD VIBES NOT FOUND, MOVING ON** — running loop

Every slide shows a live battery percentage in the top-right corner, read from the board's onboard GPIO4 voltage divider — green at 20% or above, red below.

### Controls

| Button | Function |
|---|---|
| **BOOT** (GPIO0) | Advance to the next slide (wraps around after slide 5) |
| **USER** (GPIO14) | Power button — press to sleep (screen goes black, board enters deep sleep to save power), press again to wake up |

Waking from sleep always restarts from slide 1, since deep sleep doesn't preserve RAM. There's no true hardware power-off on this board without extra circuitry, so deep sleep is the closest practical equivalent — it drops power draw substantially compared to staying fully awake.

## Hardware needed

- LilyGo T-Display S3 (170×320 ST7789 LCD, ESP32-S3)
- A single-cell 3.7V LiPo battery with a 2-pin JST-GH 1.25mm connector (optional, for wearing it untethered from USB)

## Installation

1. **Install Arduino IDE** (2.x) from [arduino.cc](https://www.arduino.cc/en/software).
2. **Add the ESP32 board package**: File -> Preferences -> paste this into "Additional boards manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   Then Tools -> Board -> Boards Manager -> search "esp32" -> install the Espressif package.
3. **Install the display library**: download [LilyGo's T-Display-S3 repo](https://github.com/Xinyuan-LilyGO/T-Display-S3), and copy everything inside its `lib/` folder into your Arduino `libraries/` folder (e.g. `Documents/Arduino/libraries`). This bundle includes a pre-configured `TFT_eSPI` with this board's correct pin mapping -- don't let the IDE auto-update it later, or it'll overwrite that config.
4. **Clone this repo** (or download it as a ZIP and extract it), keeping the `dog_slides` folder intact -- Arduino needs the `.ino` file and its `.h` sprite files together in one folder.
5. **Open `dog_slides/dog_slides.ino`** in Arduino IDE.
6. **Set the board options** under Tools:
   - Board: `ESP32S3 Dev Module`
   - USB CDC On Boot: `Enabled`
   - Flash Size: `16MB`
   - PSRAM: `OPI PSRAM`
   - Partition Scheme: `16M Flash (3MB APP/9.9MB FATFS)`
   - Upload Mode: `UART0/Hardware CDC`
   - USB Mode: `CDC and JTAG`
7. **Connect the board** via USB-C (a cable with data lines, not a charge-only one), select its port under Tools -> Port, and click **Upload**.

The board will boot straight into slide 1 once flashing finishes.

## Uninstallation

There's no separate "uninstaller" -- removing this project just means putting different firmware on the board, or removing the code from your machine.

- **To stop the board running this and use it for something else**: open any other sketch (even the bundled "Blink" example) in Arduino IDE with the board connected and click Upload. That fully overwrites this program.
- **To wipe the board back to a blank slate**: Tools -> Erase Flash (set to "All Flash Contents"), then upload any sketch. This erases everything, including this program.
- **To remove the code from your computer**: delete the cloned/extracted project folder. If you installed the LilyGo `TFT_eSPI` library bundle only for this project and don't need it elsewhere, you can also remove it from your Arduino `libraries` folder -- but note other T-Display S3 sketches will likely need it again.

## Notes

- The corgi artwork is the project owner's own uploaded asset, used here for this personal project.
- Battery percentage is a simple voltage-based estimate (not a lab-grade fuel gauge), calibrated for a typical single-cell LiPo (3.3V-4.2V range).
