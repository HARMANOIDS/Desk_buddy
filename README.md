# Desk Buddy

A tiny desk companion built from just an **ESP32** and a **128x64 SH1106 OLED**. It shows an animated face, a clock, or a custom message, and you control it from your phone over its own Wi-Fi network. No internet, no cloud, no AI chat: a local control panel only.

## Features

- **Animated face**: eyes blink and glance left/right on a 6-second loop
- **Fixed expressions**: eyes closed, look left, look right
- **Desk clock**: time is synced from your phone or computer when you tap "Clock"
- **Message mode**: show up to 80 characters of text
- **Brightness slider**
- **Self-hosted Wi-Fi**: the ESP creates its own access point, so it works anywhere

## Parts

| Part | Notes |
|------|-------|
| ESP32 dev board | Any common ESP32 board |
| 1.3" 128x64 OLED (SH1106, I2C) | Address 0x3C or 0x3D (auto-detected) |
| 4 jumper wires | |
| USB cable | Power and programming |

## Wiring

| OLED | ESP32 |
|------|-------|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

## Software setup

1. Install the Arduino IDE and add ESP32 board support.
2. Install libraries (Library Manager):
   - **Adafruit GFX Library**
   - **Adafruit SH110X**
3. Select your ESP32 board and port, then upload `DeskBuddy.ino`.
4. Open the Serial Monitor at **115200** baud to see status messages.

## Using it

1. On your phone, join Wi-Fi **DeskBuddy**, password **buddy1234**.
2. Open **http://192.168.4.1** in a browser (the Serial Monitor prints the exact address).
3. Use the buttons:
   - **Face**: animated face
   - **Clock**: sets the time from your device and shows the clock
   - **Message**: shows the text you typed (press "Show message")
   - **Expression** dropdown: animated or a fixed look
   - **Brightness** slider

## How it works

- **Display**: `Adafruit_SH1106G` draws over I2C at 100 kHz. Setup scans addresses 0x3C and 0x3D and uses whichever responds.
- **Web server**: `WebServer` on port 80 serves one HTML page (stored in flash with `PROGMEM`) at `/`, and accepts commands at `POST /control`.
- **State**: three variables drive everything: `mode` (0 face, 1 clock, 2 message), `expression` (0-3) and `message`.
- **Redraw only when needed**: a `dirty` flag, plus a pose or clock-second change, triggers a redraw, so the screen isn't redrawn constantly.
- **Clock**: the browser sends Unix time plus its timezone offset. The ESP sets its system clock to that and prints it with `gmtime_r`.

### Control API

`POST /control` with form fields (any combination):

| Field | Values |
|-------|--------|
| `mode` | 0 face, 1 clock, 2 message |
| `expression` | 0 animated, 1 eyes closed, 2 left, 3 right |
| `text` | up to 80 chars (non-printable characters become `?`) |
| `epoch`, `offset` | Unix seconds and timezone offset in seconds |
| `brightness` | 0-255 |

## Known limits and ideas

- No battery-backed clock: the time resets on power loss, so tap **Clock** again after each boot.
- Change the Wi-Fi password in `setup()` (`WiFi.softAP("DeskBuddy","buddy1234")`) if you want it private.
- Ideas: more expressions (happy, sleepy), a touch or button input, a timer or Pomodoro mode, scrolling text, saving the last message to flash.

## Files

```
DeskBuddy/
├── DeskBuddy.ino   (the code)
└── README.md       (this file)
```
