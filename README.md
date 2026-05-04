# CSS-Helmet
Firmware for an **Arduino Nano ESP32** mounted in a smart cycling helmet. Receives BLE commands from a Raspberry Pi 5 and drives haptic + audio alerts in real time.

---

## Overview

The ESP32 advertises a BLE service and exposes a single writable characteristic. A host device (Raspberry Pi 5 running `main.py`) writes short command strings to trigger left/right buzzers and vibration motors — giving the rider directional audio and haptic warnings without needing to look at a screen.

---

## Hardware

| Pin | Component |
|-----|-----------|
| D4  | Left passive buzzer |
| D6  | Right passive buzzer |
| D2  | Left vibration motor |
| D9  | Right vibration motor |

---

## Alert Levels

| Level   | Tone     | On    | Off   | Pattern |
|---------|----------|-------|-------|---------|
| `LOW`   | 800 Hz   | 100ms | 500ms | Slow pulse |
| `HIGH`  | 2500 Hz  | 60ms  | 150ms | Fast pulse |
| `CRASH` | 3500 Hz  | 50ms  | 50ms  | Rapid alarm, both sides |

All pulsing is **non-blocking** — the loop uses `millis()`-based timing so the device stays responsive to incoming BLE commands while an alert is active.

---

## BLE Commands

| Command      | Effect |
|--------------|--------|
| `LEFT_LOW`   | Left side — slow pulse, 800 Hz |
| `LEFT_HIGH`  | Left side — fast pulse, 2500 Hz |
| `LEFT_OFF`   | Left side off |
| `RIGHT_LOW`  | Right side — slow pulse, 800 Hz |
| `RIGHT_HIGH` | Right side — fast pulse, 2500 Hz |
| `RIGHT_OFF`  | Right side off |
| `ALL_OFF`    | All outputs off |
| `CRASH`      | Both sides — rapid alarm, takes full priority |

Left and right sides are managed independently, so asymmetric alerts are supported (e.g. left-only warning for a left-lane hazard).

---

## BLE Configuration

| Field | Value |
|-------|-------|
| Device name | `HelmetESP32` |
| Service UUID | `12345678-1234-5678-1234-56789abcdef0` |
| Characteristic UUID | `12345678-1234-5678-1234-56789abcdef1` |
| Properties | Write / Write Without Response |
| Max value length | 20 bytes |

> UUIDs must match those defined in the Pi's `main.py`.

---

## Dependencies

- [ArduinoBLE](https://www.arduino.cc/reference/en/libraries/arduinoble/) — install via Arduino Library Manager

---

## Getting Started

1. Install **ArduinoBLE** via the Arduino Library Manager.
2. Open `CSS_Helmet.ino` in the Arduino IDE.
3. Select **Arduino Nano ESP32** as the target board.
4. Upload — the device plays a startup beep and begins advertising immediately.
5. On connection, two beeps confirm pairing; one beep on disconnect.

---

## License

MIT
