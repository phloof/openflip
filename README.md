# openflip
Open source mechanical switch actuator built with ESP32 and a servo motor, with native Matter support for Google Home, Apple Home, and Alexa.

## Hardware Requirements
* **Microcontroller:** ESP32-S3 SuperMini (most likely compatible with other ESP32 might have to add support for non BLE boards)
* **Actuator:** Standard or Micro Servo (GPIO 4)
* **Status Light:** Built-in WS2812 RGB LED (GPIO 48)
* **Manual Control:** Built-in BOOT button (GPIO 0)

---

## Config

Before flashing, choose your switch behavior inside the code (`constexpr SwitchMode SWITCH_MODE`):

| Mode | Behavior | Ideal For |
| :--- | :--- | :--- |
| `MODE_DIRECT` | Servo sweeps to `POS_ON` / `POS_OFF` and holds position. | Rocker switches, latching push buttons. |
| `MODE_TOGGLE` | Servo nudges to `POS_ON` and returns to `POS_REST` immediately. | Momentary buttons, toggle/flip switches. |

---

## LED Status Reference

* **Blinking Blue:** Ready to pair (Commissioning mode)
* **Solid Red:** Switch is **OFF**
* **Solid Green:** Switch is **ON**
* **Off (Black):** Servo is currently moving (minimizes power spikes)
* **Purple Flash:** Factory reset triggered

---

## First-Time Setup & Pairing

1. **Flash the Firmware:** Upload the code to your ESP32-S3.
2. **Open Serial Monitor:** Set your baud rate to `115200`.
3. **Get the Link:** Copy the onboarding QR URL printed in the console logs and open it in a web browser.
4. **Pair Device:**
   * Open your favorite Smart Home App (e.g., **Google Home**).
   * Tap **Add Device (`+`)** $\rightarrow$ **Set up device** $\rightarrow$ **Matter device**.
   * Scan the QR code displayed in your browser.
   * *Note: Setup handles Wi-Fi over Bluetooth (BLE). No hardcoded credentials required.*

---

## Manual Toggle & Factory Reset

* **Manual Press:** Give the **BOOT** button a single short press to manually toggle the switch state.
* **Factory Reset:** Hold the **BOOT** button for **$\ge$ 5 seconds**. The LED will flash purple, clear all Matter pairing credentials, and reboot back into setup mode.

---

## Future Changes

* **Expanded Hardware Compatibility:** Add support for non-BLE (Bluetooth Low Energy) microcontrollers and development boards.
* **Advanced Power Optimisation:**
  * Implement **Modem Sleep** to significantly reduce Wi-Fi power consumption during idle periods.
  * Dynamically lower **CPU Frequency** to conserve battery life.
  * Add a feature to turn the **Idle LED Completely Off** instead of keeping it red or green, eliminating phantom power draw.

---

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. See the official GNU documentation for details.
