/*
  helmet_esp32.ino
  ---------------------
  Helmet Controller — Arduino Nano ESP32

  Receives BLE commands from Raspberry Pi 5 and triggers
  buzzers and vibration motors accordingly.

  Pin assignments:
    D4  → Left  passive buzzer
    D6  → Right passive buzzer
    D2  → Left  vibration motor
    D9  → Right vibration motor

  Alert behavior:
    LOW  → slow pulse, low tone (800Hz)
    HIGH → fast pulse, high tone (2500Hz)
    CRASH → continuous rapid alarm on both sides

  BLE UUIDs must match Pi's main.py exactly.
*/

#include <ArduinoBLE.h>

// --- Pin definitions ---
#define PIN_BUZZER_LEFT  4
#define PIN_BUZZER_RIGHT 6
#define PIN_MOTOR_LEFT   2
#define PIN_MOTOR_RIGHT  9

// --- BLE UUIDs ---
#define SERVICE_UUID  "12345678-1234-5678-1234-56789abcdef0"
#define CHAR_UUID     "12345678-1234-5678-1234-56789abcdef1"

BLEService helmetService(SERVICE_UUID);
BLEStringCharacteristic cmdChar(CHAR_UUID, BLEWrite | BLEWriteWithoutResponse, 20);

// --- Alert state ---
String leftState  = "OFF";
String rightState = "OFF";
bool   crashing   = false;

unsigned long lastPulseLeft  = 0;
unsigned long lastPulseRight = 0;
unsigned long lastCrashPulse = 0;

bool motorLeftOn  = false;
bool motorRightOn = false;
bool buzzerLeftOn = false;
bool buzzerRightOn= false;

// --- Pulse config per level ---
// { tone_hz, on_ms, off_ms }
struct PulseConfig {
  int  freq;
  int  on_ms;
  int  off_ms;
};

PulseConfig CONFIG_LOW  = { 800,  100, 500 };  // slow, quiet
PulseConfig CONFIG_HIGH = { 2500,  60, 150 };  // fast, loud
PulseConfig CONFIG_CRASH= { 3500,  50,  50 };  // rapid alarm

// --- Helpers ---

void allOff() {
  noTone(PIN_BUZZER_LEFT);
  noTone(PIN_BUZZER_RIGHT);
  digitalWrite(PIN_MOTOR_LEFT,  LOW);
  digitalWrite(PIN_MOTOR_RIGHT, LOW);
  leftState    = "OFF";
  rightState   = "OFF";
  crashing     = false;
  motorLeftOn  = false;
  motorRightOn = false;
  buzzerLeftOn = false;
  buzzerRightOn= false;
  Serial.println("[Helmet] All OFF");
}

void buzzBoth(int freq, int on_ms, int times, int off_ms) {
  for (int i = 0; i < times; i++) {
    tone(PIN_BUZZER_LEFT,  freq, on_ms);
    tone(PIN_BUZZER_RIGHT, freq, on_ms);
    delay(on_ms);
    noTone(PIN_BUZZER_LEFT);
    noTone(PIN_BUZZER_RIGHT);
    if (i < times - 1) delay(off_ms);
  }
}

// --- Command handler ---

void handleCommand(String cmd) {
  cmd.trim();
  Serial.print("[CMD] ");
  Serial.println(cmd);

  if (cmd == "LEFT_LOW") {
    leftState     = "LOW";
    lastPulseLeft = 0;

  } else if (cmd == "LEFT_HIGH") {
    leftState     = "HIGH";
    lastPulseLeft = 0;

  } else if (cmd == "LEFT_OFF") {
    leftState = "OFF";
    digitalWrite(PIN_MOTOR_LEFT, LOW);
    noTone(PIN_BUZZER_LEFT);
    motorLeftOn  = false;
    buzzerLeftOn = false;

  } else if (cmd == "RIGHT_LOW") {
    rightState     = "LOW";
    lastPulseRight = 0;

  } else if (cmd == "RIGHT_HIGH") {
    rightState     = "HIGH";
    lastPulseRight = 0;

  } else if (cmd == "RIGHT_OFF") {
    rightState = "OFF";
    digitalWrite(PIN_MOTOR_RIGHT, LOW);
    noTone(PIN_BUZZER_RIGHT);
    motorRightOn  = false;
    buzzerRightOn = false;

  } else if (cmd == "ALL_OFF") {
    allOff();

  } else if (cmd == "CRASH") {
    crashing       = true;
    leftState      = "OFF";
    rightState     = "OFF";
    lastCrashPulse = 0;

  } else {
    Serial.print("[CMD] Unknown: ");
    Serial.println(cmd);
  }
}

// --- Pulse engine ---
// Called every loop iteration — handles non-blocking pulsing

void updateOutputs() {
  unsigned long now = millis();

  // --- CRASH takes full priority ---
  if (crashing) {
    PulseConfig& cfg = CONFIG_CRASH;
    unsigned long interval = buzzerLeftOn ? cfg.on_ms : cfg.off_ms;

    if (now - lastCrashPulse >= (unsigned long)interval) {
      buzzerLeftOn  = !buzzerLeftOn;
      buzzerRightOn = buzzerLeftOn;
      motorLeftOn   = buzzerLeftOn;
      motorRightOn  = buzzerLeftOn;

      if (buzzerLeftOn) {
        tone(PIN_BUZZER_LEFT,  cfg.freq);
        tone(PIN_BUZZER_RIGHT, cfg.freq);
      } else {
        noTone(PIN_BUZZER_LEFT);
        noTone(PIN_BUZZER_RIGHT);
      }
      digitalWrite(PIN_MOTOR_LEFT,  motorLeftOn  ? HIGH : LOW);
      digitalWrite(PIN_MOTOR_RIGHT, motorRightOn ? HIGH : LOW);
      lastCrashPulse = now;
    }
    return;
  }

  // --- Left side ---
  if (leftState != "OFF") {
    PulseConfig& cfg = (leftState == "HIGH") ? CONFIG_HIGH : CONFIG_LOW;
    unsigned long interval = motorLeftOn ? cfg.on_ms : cfg.off_ms;

    if (now - lastPulseLeft >= (unsigned long)interval) {
      motorLeftOn  = !motorLeftOn;
      buzzerLeftOn = motorLeftOn;

      digitalWrite(PIN_MOTOR_LEFT, motorLeftOn ? HIGH : LOW);

      if (buzzerLeftOn) {
        int freq = (leftState == "HIGH") ? CONFIG_HIGH.freq : CONFIG_LOW.freq;
        tone(PIN_BUZZER_LEFT, freq);
      } else {
        noTone(PIN_BUZZER_LEFT);
      }
      lastPulseLeft = now;
    }
  }

  // --- Right side ---
  if (rightState != "OFF") {
    PulseConfig& cfg = (rightState == "HIGH") ? CONFIG_HIGH : CONFIG_LOW;
    unsigned long interval = motorRightOn ? cfg.on_ms : cfg.off_ms;

    if (now - lastPulseRight >= (unsigned long)interval) {
      motorRightOn  = !motorRightOn;
      buzzerRightOn = motorRightOn;

      digitalWrite(PIN_MOTOR_RIGHT, motorRightOn ? HIGH : LOW);

      if (buzzerRightOn) {
        int freq = (rightState == "HIGH") ? CONFIG_HIGH.freq : CONFIG_LOW.freq;
        tone(PIN_BUZZER_RIGHT, freq);
      } else {
        noTone(PIN_BUZZER_RIGHT);
      }
      lastPulseRight = now;
    }
  }

  // --- Both sides off → silence both buzzers ---
  if (leftState == "OFF" && rightState == "OFF") {
    noTone(PIN_BUZZER_LEFT);
    noTone(PIN_BUZZER_RIGHT);
  }
}

// --- Setup ---

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_BUZZER_LEFT,  OUTPUT);
  pinMode(PIN_BUZZER_RIGHT, OUTPUT);
  pinMode(PIN_MOTOR_LEFT,   OUTPUT);
  pinMode(PIN_MOTOR_RIGHT,  OUTPUT);
  allOff();

  Serial.println("[Helmet] Starting...");

  // Startup beep — both sides
  buzzBoth(1200, 200, 1, 0);

  if (!BLE.begin()) {
    Serial.println("[BLE] Failed to start!");
    while (1);
  }

  BLE.setLocalName("HelmetESP32");
  BLE.setAdvertisedService(helmetService);
  helmetService.addCharacteristic(cmdChar);
  BLE.addService(helmetService);

  BLE.advertise();
  Serial.println("[BLE] Advertising as HelmetESP32...");
}

// --- Loop ---

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("[BLE] Connected to: ");
    Serial.println(central.address());

    buzzBoth(1200, 150, 2, 80);

    while (central.connected()) {
      if (cmdChar.written()) {
        String cmd = cmdChar.value();
        handleCommand(cmd);
      }
      updateOutputs();
    }

    buzzBoth(800, 200, 1, 0);
    allOff();
    Serial.println("[BLE] Disconnected — re-advertising...");
  }
}