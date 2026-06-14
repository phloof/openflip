/**
 * ESP32-S3 SuperMini – Matter Smart Switch Presser
 * 
 * --- SWITCH MODES ------------------------
 *  MODE_DIRECT  Servo holds its position to physically hold a switch in place.
 *                 OFF -> servo sweeps to POS_OFF and stays.
 *                 ON  -> servo sweeps to POS_ON  and stays.
 *                 Use for: rocker switches, push-on/push-off buttons.
 *
 *  MODE_TOGGLE  Servo presses and releases for every logical state change;
 *                 the logical ON/OFF state is tracked in NVS and the servo
 *                 always returns to POS_REST after each press.
 *                 Use for: flip/toggle switches, momentary push buttons.
 *
 * --- LED INDICATORS ------------------------
 *  Blue blinking  = waiting to be commissioned (first run)
 *  Red            = switch is OFF
 *  Green          = switch is ON
 *  Off (black)    = servo is moving
 *  Purple flash   = factory-reset triggered
 *
 * ─── FIRST RUN ────────────────────────────────────────────────────────────────
 *  1. Flash the board and open the Serial monitor at 115200 baud.
 *  2. Copy the QR URL printed to Serial → open in a browser to show the QR code.
 *  3. Google Home app → + → Set up device → Matter → scan QR.
 *     (Google Home sends Wi-Fi credentials over BLE — no hardcoded SSID needed.)
 *
 * ─── FACTORY RESET ────────────────────────────────────────────────────────────
 *  Hold the BOOT button (GPIO 0) for ≥ 5 s. The board clears its Matter
 *  credentials and restarts into commissioning mode.
 */

#include <Arduino.h>
#include <Matter.h>
#include <MatterEndpoints/MatterOnOffPlugin.h>
#include <FastLED.h>
#include <ESP32Servo.h>
#include <Preferences.h>

//  CONFIG

// Select switch behaviour by changing the value on the right:
//   MODE_DIRECT  – servo holds position (rocker / latching switch)
//   MODE_TOGGLE  – servo presses & releases for each change (flip / toggle switch)
enum SwitchMode { MODE_DIRECT, MODE_TOGGLE };
constexpr SwitchMode SWITCH_MODE = MODE_TOGGLE;   //  change here

//  MODE_DIRECT:
//    POS_OFF  physical position when switch is OFF (servo released / resting)
//    POS_ON   physical position when switch is ON  (servo pressing)
//    POS_REST unused in this mode.
//
//  MODE_TOGGLE:
//    POS_REST neutral resting position — servo parks here when idle
//    POS_ON   press position — servo nudges the switch lever here then returns
//    POS_OFF  unused in this mode.
//
constexpr int POS_OFF  =   15;   // degrees
constexpr int POS_ON   =   35;   // degrees
constexpr int POS_REST =   15;   // degrees (toggle mode only; often same as POS_OFF)

// Timing
constexpr int  SWEEP_MS       =  20;    // ms per degree during sweep (reduce power spike)
constexpr int  PRESS_HOLD_MS  = 150;    // toggle mode: ms to dwell at POS_ON
constexpr int  SERVO_SETTLE   =  100;    // ms after writing a position before detaching

// Pins
constexpr int LED_PIN   = 48;   // built-in LED on ESP32-S3 SuperMini
constexpr int SERVO_PIN =  4;

// Factory-reset hold time
constexpr unsigned long RESET_HOLD_MS = 5000UL;

// CONFIG END

CRGB              leds[1];
Servo             myServo;
MatterOnOffPlugin switchPlugin;
Preferences       prefs;

// reflects current ON/OFF state.
bool           hwState    = false;
volatile bool  targetOn   = false;
volatile bool  hasRequest = false;

bool loadState() {
  prefs.begin("sw", /*readOnly=*/true);
  bool s = prefs.getBool("on", false);
  prefs.end();
  return s;
}

void saveState(bool on) {
  prefs.begin("sw", /*readOnly=*/false);
  prefs.putBool("on", on);
  prefs.end();
}

// LED
void showLED(CRGB color) { leds[0] = color; FastLED.show(); }

// Servo Logic
void servoAttach() {
  myServo.attach(SERVO_PIN, 500, 2400);
}

void servoDetach() {
  delay(SERVO_SETTLE);   // let the last write complete before cutting PWM
  myServo.detach();
}

void sweepTo(int from, int to) {
  if (from == to) return;
  const int step = (from < to) ? 1 : -1;
  for (int pos = from; pos != to + step; pos += step) {
    myServo.write(pos);
    delay(SWEEP_MS);
  }
}

// Apply
// Must only be called from loop() — servo PWM is not safe from other tasks.
void applyState(bool on) {
  if (on == hwState) return;

  showLED(CRGB::Black);   // LED off while servo is moving reduce power spiek 
  servoAttach();

  if constexpr (SWITCH_MODE == MODE_DIRECT) {
    const int fromPos = hwState ? POS_ON : POS_OFF;
    const int toPos   = on      ? POS_ON : POS_OFF;

    myServo.write(fromPos);   // anchor to known position on attach to prevent jerk
    delay(SERVO_SETTLE);
    sweepTo(fromPos, toPos);  // slow sweep to target

    Serial.printf("[SWITCH] %s  →  servo held at %d°\n",
                  on ? "ON" : "OFF", toPos);

  } else {
    // Toggle
    // Press the switch lever and release, regardless of which direction we
    // are toggling. Logical state is tracked in hwState / NVS.
    myServo.write(POS_REST);       // anchor to rest on attach
    delay(SERVO_SETTLE);
    sweepTo(POS_REST, POS_ON);     // press
    delay(PRESS_HOLD_MS);          // hold
    sweepTo(POS_ON,   POS_REST);   // release

    Serial.printf("[SWITCH] Toggled  →  logical state: %s\n",
                  on ? "ON" : "OFF");
  }

  servoDetach();

  hwState = on;
  saveState(on);
  showLED(on ? CRGB::Green : CRGB::Red);
}

// Matter callback
// Runs on Matter's internal FreeRTOS task — only set flags, never touch servo.
bool onMatterChange(bool state) {
  targetOn   = state;
  hasRequest = true;
  return true;
}

// Boot tap for toggle hold defined time for reset
void checkResetButton() {
  if (digitalRead(BOOT_PIN) != LOW) return;

  unsigned long t = millis();
  while (digitalRead(BOOT_PIN) == LOW) {
    if (millis() - t >= RESET_HOLD_MS) {
      Serial.println("[RESET] Wiping Matter credentials – restarting…");
      showLED(CRGB(60, 0, 60));   // purple flash
      Matter.decommission();
      delay(500);
      ESP.restart();
    }
  }
  if (millis() - t < 50) return;   // debounce

  bool newState = !hwState;
  switchPlugin.setOnOff(newState);
  targetOn   = newState;
  hasRequest = true;
  Serial.println("[BUTTON] Manual toggle");
}

void setup() {
  Serial.begin(115200);
  delay(600);
  Serial.println("\n=== ESP32-S3 Matter Smart Switch Presser ===\n");
  Serial.printf("[CONFIG] Switch mode: %s\n\n",
                SWITCH_MODE == MODE_DIRECT ? "DIRECT (holds position)"
                                           : "TOGGLE (press & release)");

  // LED
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, 1);
  FastLED.setBrightness(1); // brightness 1 for low power
  showLED(CRGB::Black);

  // Restore state from NVS
  hwState = loadState();
  Serial.printf("[BOOT]   Restored state: %s\n", hwState ? "ON" : "OFF");

  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  servoAttach();

  if constexpr (SWITCH_MODE == MODE_DIRECT) {
    const int initPos = hwState ? POS_ON : POS_OFF;
    myServo.write(initPos);
    delay(400);
    Serial.printf("[SERVO]  Snapped to %d° (saved position), now detaching\n",
                  initPos);
  } else {
    // Toggle logic
    myServo.write(POS_REST);
    delay(400);
    Serial.println("[SERVO]  Parked at rest position, now detaching");
  }

  servoDetach();

  pinMode(BOOT_PIN, INPUT_PULLUP);

  // Matter
  switchPlugin.begin(hwState);
  Matter.begin();
  switchPlugin.onChangeOnOff(onMatterChange);

  // Commissioning
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("[MATTER] Device not commissioned yet.\n");
    Serial.println("  1. Open the QR URL below in a browser:");
    Serial.println("     " + Matter.getOnboardingQRCodeUrl());
    Serial.println("  2. Google Home → + → Set up device → Matter → scan QR.");
    Serial.println("  Or manual pairing code: " + Matter.getManualPairingCode());
    Serial.println("\n  LED blinks blue until commissioned.\n");

    while (!Matter.isDeviceCommissioned()) {
      showLED(CRGB::Blue);  delay(350);
      showLED(CRGB::Black); delay(350);
      checkResetButton();
    }
    Serial.println("[MATTER] Commissioned!\n");
  }

  showLED(hwState ? CRGB::Green : CRGB::Red);
  Serial.println("[MATTER] Online – awaiting commands.\n");
}

// Loop
void loop() {
  checkResetButton();

  if (hasRequest) {
    hasRequest = false;
    applyState(targetOn);
  }

  delay(10);
}