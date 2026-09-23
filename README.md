# The Digital Hair Clip

> ⚠️ **A quick word before you build this:** the battery pins sit exposed
> on the board's edge, and this thing is about to live in someone's hair —
> so give those pins a small piece of tape before it's worn, or a stray
> strand will happily bridge them for you. And get the polarity right
> every time: backwards is the kind of mistake that takes the board and
> the battery down together, not just one quiet blink of nothing happening.

A LilyGo T-Display S3 turned into a wearable, battery-powered hair clip that
cycles through a little corgi's daily moods. It started as a "can I put a
tiny animated screen on a hair clip" idea, and turned into a small project
in embedded power budgeting: every design decision here — the sprite art,
the frame rate, the brightness, the sleep behaviour — exists because a
220mAh battery is a genuinely tiny amount of energy, and a full-colour LCD
is a genuinely hungry thing to run off it.

This document is both the story of how it got built and the reference for
building your own.

<details>
<summary><strong>What it actually does</strong></summary>

Five slides, cycled with a button press, each with its own little looping
corgi animation and its own colour:

1. **HI!** — a big wave, warm orange text
2. **REDBULL UNTIL FRIYAY** — the corgi jumping for joy
3. **SMILE! IT'S FRIYAY!** — a happy blinking loop
4. **1% BATTERY, 100% MAIN CHARACTER ENERGY** — a confident running loop
5. **ERROR 404: BAD VIBES NOT FOUND** — the corgi thinking it over, unconvinced

Every slide shows a live battery percentage in the top-right corner, read
straight off the board's onboard GPIO4 voltage divider — green at 20% or
above, red below.

### Controls

| Button | Function |
|---|---|
| **BOOT** (GPIO0) | Advance to the next slide (wraps around after slide 5) |
| **USER** (GPIO14) | Power button — press to sleep (screen goes black, board enters deep sleep), press again to wake up |

Waking from sleep always restarts from slide 1, since deep sleep doesn't
preserve RAM. There's no true hardware power-off on this board without
extra circuitry, so deep sleep is the closest practical equivalent to
"off." How much that actually saves is covered honestly, with real
numbers, further down — the short version is: less than you'd hope.

</details>

---

<details>
<summary><strong>Building one</strong></summary>

### What you need

- **LilyGo T-Display S3** (1.9" ST7789 LCD, ESP32-S3), in its normal black
  housing
- **A 3.7V 220mAh LiPo battery**, the small 402030-size cell with a
  2-pin JST connector and a built-in protection circuit (the kind sold for
  small DIY electronics — it'll usually have a tiny status LED on the
  board itself)
- **A small screwdriver set** — you'll need it to open the board's case
- **Electronics-safe double-sided sticky tape** (a foam mounting tape
  works well; you want something that won't leave residue or short
  anything out)
- **A sheet of acrylic, 2-3mm thick** — this becomes the rigid backing
  the whole thing is built on
- **An alligator clip** (the metal spring-loaded kind, like a large
  bulldog/crocodile clip) — this is what actually grips your hair

### Step by step

1. **Flash the firmware first, while everything is still easy to reach.**
   Connect the bare board to your PC over USB-C and follow
   [Installing the software](#installing-the-software) just below before
   doing anything else. It's much easier to debug upload issues, iterate
   on colours/animations, and test button behaviour *before* everything
   is glued into a fixed physical assembly. (This is also where an AI
   coding assistant genuinely earns its keep — most of the real
   engineering here was compiling, flashing, diagnosing a
   corrupted-sprite bug, and tuning frame timing over dozens of
   iterations, which is a lot less painful with something driving the
   toolchain for you.)

2. **Open the case.** Use the small screwdriver set to carefully take the
   board out of its plastic housing. You just need the bare PCB — the
   housing isn't used in the final assembly.

3. **Connect the battery — carefully.** Check polarity (red-to-red,
   black-to-black) before you seat the connector — see the safety note
   above for why this matters. Once it's connected, **cover the exposed
   battery pins on the side of the board with a small piece of insulating
   tape or a thin plastic shroud**, so nothing (hair included) can bridge
   them once this is being worn.

4. **Cut the acrylic sheet to size.** Trace the bare board's outline (or
   just eyeball it slightly larger) onto the acrylic and cut it down —
   this becomes the rigid plate that everything else mounts to. Acrylic
   is chosen here specifically because it's rigid, thin, and light: a
   hair clip needs to not flex when clipped in, but also can't add much
   weight.

5. **This is the part that actually makes it a "clip": mount the
   alligator clip to the acrylic.** Use the electronics-safe sticky tape
   to fix the alligator clip's flat handle/base to the back of the
   acrylic sheet, with its spring-loaded jaws facing outward past one
   edge. The alligator clip is doing the actual mechanical job a hair
   clip needs to do — clamping onto a section of hair and holding its
   own weight — while the acrylic gives it a flat, rigid surface to be
   mounted to instead of trying to stick electronics directly onto a
   springy metal clip.

6. **Stick the board to the front of the acrylic**, on the opposite face
   from the clip, using the same sticky tape. Keep the battery tucked
   against the acrylic too, ideally with its own small strip of tape so
   it isn't just dangling on its wires.

7. **Check clearances.** Make sure the alligator clip's jaws can still
   open and close freely without catching on the board or battery wires,
   and that the USB-C port is still reachable for future re-flashing
   without having to pull anything apart.

That's it — clip it into a section of hair, press BOOT to cycle slides,
and press the power button when you're done wearing it.

### Installing the software

This is what step 1 above is pointing at — do this part first, before
any of the physical assembly:

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
   - Board: `LilyGo T3-S3` (or `ESP32S3 Dev Module` if that specific entry isn't available)
   - USB CDC On Boot: `Enabled`
   - Flash Size: `16MB`
   - PSRAM: `OPI PSRAM`
   - Partition Scheme: `16M Flash (3MB APP/9.9MB FATFS)`
   - Upload Mode: `UART0/Hardware CDC`
   - USB Mode: `CDC and JTAG`
7. **Connect the board** via USB-C (a cable with data lines, not a charge-only one), select its port under Tools -> Port, and click **Upload**.

If the upload hangs on "Connecting...." with no response, hold the
**BOOT** button, tap **RESET** once while still holding BOOT, then start
the upload and release BOOT once it starts writing — this manually forces
the chip into its bootloader instead of relying on the auto-reset circuit.

The board will boot straight into slide 1 once flashing finishes — now
go back and do the physical assembly steps above.

### The finished build

<p align="center">
  <img src="docs/photos/clip_side_profile.jpg" width="32%" alt="Side profile of the assembled clip, showing the acrylic backing, alligator clip jaws, and battery" />
  <img src="docs/photos/screen_running.jpg" width="32%" alt="The clip held up with the HI slide running on screen" />
  <img src="docs/photos/clip_closed_edge.jpg" width="32%" alt="Edge-on view of the clip closed" />
</p>

<p align="center">
  <video src="docs/photos/device_demo.mp4" controls width="480">
    Your browser doesn't support inline video — <a href="docs/photos/device_demo.mp4">download the clip</a> instead.
  </video>
</p>

### Uninstalling / resetting the board

There's no separate "uninstaller" -- removing this project just means putting different firmware on the board, or removing the code from your machine.

- **To stop the board running this and use it for something else**: open any other sketch (even the bundled "Blink" example) in Arduino IDE with the board connected and click Upload. That fully overwrites this program.
- **To wipe the board back to a blank slate**: Tools -> Erase Flash (set to "All Flash Contents"), then upload any sketch. This erases everything, including this program.
- **To remove the code from your computer**: delete the cloned/extracted project folder. If you installed the LilyGo `TFT_eSPI` library bundle only for this project and don't need it elsewhere, you can also remove it from your Arduino `libraries` folder -- but note other T-Display S3 sketches will likely need it again.

</details>

---

<details>
<summary><strong>The software journey — optional read, for anyone curious how this actually came together</strong></summary>

The goal at the start was simple: put a small animated character on a
screen, clip it into hair, done. What actually happened was a crash
course in how little energy 220mAh really is once a full-colour LCD is
involved.

The firmware itself ended up as one Arduino sketch
(`dog_slides/dog_slides.ino`), with each corgi animation baked directly
into flash as raw colour frames — no SD card, nothing loaded at runtime.
The art came from a free pixel-art spritesheet found online (the "jump"
and "thinking" animations), sliced apart programmatically into
individual frames and recoloured to match the UI — a shortcut that let
five different moods get tried and swapped in minutes instead of
hand-drawn one at a time. The original sheet lives in `docs/assets/` if
you're curious.

That's where the simple part ended. Once the slides were running, the
first real battery test came back rough — the display was noticeably
hungrier than expected, and it turned out **redrawing the screen is the
single most expensive thing this firmware ever does**, well above the
CPU or even the backlight. Every animation frame is a fresh wave of pixel
data over SPI, and a faster loop just pays that cost more often for
motion most people don't consciously register. So every slide's frame
rate got dialed back deliberately, the on-screen sprite was kept small
instead of filling the display, and only the two slides that genuinely
needed some bounce (jump, battery) got sped back up individually. The
animation's personality ended up being a battery decision first, a style
decision second.

Alongside that, a handful of other things got tuned once it was clear
battery life needed real attention:

- **Backlight dimmed to ~35% brightness** (`BRIGHTNESS_LEVEL = 90/255`) —
  right alongside screen redraws, one of the two biggest power draws here.
- **CPU clocked down to 80MHz** instead of 240MHz — nothing this firmware
  does needs the extra speed.
- **Wi-Fi and Bluetooth powered off at boot** — unused, but can silently
  draw current if left on.
- **Automatic CPU light sleep** between frames and button polls — no
  effect on responsiveness, since wake time is sub-millisecond.

Then came the part that was genuinely surprising: a real timed test on
the physical device, comparing the screen left on and animating against
the board put to deep sleep via the power button. Two consistent rates
came out of it — roughly **16% drained per 10 minutes switched on**,
versus roughly **9% per 1.5 hours in deep sleep**. Projected out to a
full 100%→0% runtime, that gap is stark:

![Estimated battery runtime: switched on vs switched off](docs/battery_performance_graph.png)

Left on continuously, this 220mAh cell is gone in **about an hour**. Put
to sleep between wears, it stretches to roughly **16-17 hours**. That's
not a subtle difference — the power button turned out to be the entire
reason this is wearable for more than a single outing, not just a nice
extra. It comes back to the same root cause as the frame-rate tuning:
deep sleep switches off the backlight, the CPU, and all that SPI traffic
at once, which is the only way to actually stop paying that cost.

**Where it landed:** treat the power button as essential. Left running,
expect well under two hours before it's flat. Put to sleep whenever it's
not being looked at, expect closer to a full day's standby.

</details>

---

<details>
<summary><strong>Notes</strong></summary>

- None of the corgi artwork here is original work — all of it (the wave/smile/battery-slide frames and the jump/thinking-it-over animations) came from pixel-art assets found online, sourced from unknown/unrecorded origins. This project is personal and non-commercial; if you recognise the art and want it credited or removed, that's a completely fair ask.
- Battery percentage is a simple voltage-based estimate (not a lab-grade fuel gauge), calibrated for a typical single-cell LiPo (3.3V-4.2V range), and is noticeably noisy over short windows — treat single readings a few minutes apart with some skepticism; the numbers in the battery section above come from a longer, deliberately spaced-out test for that reason.

</details>
