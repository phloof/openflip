# openflip
Open source mechanical switch actuator built with ESP32 and a servo motor, with native Matter support for Google Home, Apple Home, and Alexa.

## Hardware Requirements
* **Microcontroller:** ESP32-S3 SuperMini (most likely compatible with other ESP32 might have to add support for non BLE boards)
* **Actuator:** Standard or Micro Servo (GPIO 4)
* **Status Light:** Built-in WS2812 RGB LED (GPIO 48)
* **Manual Control:** Built-in BOOT button (GPIO 0)

---

## Config

You can customise the behavior, physical limits, and hardware parameters of your switch presser by modifying the constants at the top of the code.

| Configuration Constant | Default Value | Description / Purpose |
| :--- | :--- | :--- |
| **`SWITCH_MODE`** | `MODE_TOGGLE` | **`MODE_DIRECT`**: Servo holds position (rocker switches). <br>**`MODE_TOGGLE`**: Servo presses & releases instantly (momentary buttons). |
| **`POS_OFF`** | `15` (degrees) | Physical angle when the switch is turned **OFF** (Direct mode). |
| **`POS_ON`** | `35` (degrees) | Physical angle when pressing the switch **ON** (Both modes). |
| **`POS_REST`** | `15` (degrees) | Neutral parked position where the servo rests when idle (Toggle mode). |
| **`SWEEP_MS`** | `20` (ms) | Time delay per degree of rotation. Higher values smooth the sweep and reduce current spikes. |
| **`PRESS_HOLD_MS`** | `150` (ms) | How long the servo pauses at full press (`POS_ON`) before releasing (Toggle mode). |
| **`SERVO_SETTLE`** | `100` (ms) | Buffer time allowed for the servo to reach its destination before turning off PWM. |
| **`LED_PIN`** | `48` | GPIO pin connected to the built-in WS2812 RGB LED. |
| **`SERVO_PIN`** | `4` | GPIO pin outputting the PWM signal to the servo motor signal wire. |
| **`RESET_HOLD_MS`** | `5000` (ms) | Duration the BOOT button must be held down to trigger a factory reset. |
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
