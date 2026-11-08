// De 2: ESP32 LEDs, physical buttons, Blynk, HC-SR04 and safety priority.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "Controller.h"

constexpr uint8_t LED_RED = 22;
constexpr uint8_t LED_YELLOW = 21;
constexpr uint8_t LED_GREEN = 18;
constexpr uint8_t BUTTON1 = 19;
constexpr uint8_t BUTTON2 = 5;
constexpr uint8_t HC_TRIG = 23;
constexpr uint8_t HC_ECHO = 27;
constexpr uint32_t SENSOR_PERIOD_MS = 100;
constexpr uint32_t ECHO_TIMEOUT_US = 25000;
constexpr uint32_t TELEMETRY_PERIOD_MS = 1000;
constexpr uint32_t NETWORK_RETRY_MS = 5000;

LedController controller;
portMUX_TYPE controllerMux = portMUX_INITIALIZER_UNLOCKED;
BlynkTimer telemetryTimer;

struct CriticalEvent {
  uint32_t uptimeMs;
  float distanceCm;
};
constexpr size_t EVENT_CAPACITY = 16;
CriticalEvent eventLog[EVENT_CAPACITY] = {};
size_t eventWriteIndex = 0;
size_t eventCount = 0;

struct DebouncedButton {
  uint8_t pin;
  bool previousRaw = HIGH;
  bool stable = HIGH;
  uint32_t changedMs = 0;

  bool pressed(uint32_t now) {
    const bool raw = digitalRead(pin);
    if (raw != previousRaw) {
      previousRaw = raw;
      changedMs = now;
    }
    if (raw != stable && static_cast<uint32_t>(now - changedMs) >= 30) {
      stable = raw;
      return stable == LOW;
    }
    return false;
  }
};

DebouncedButton button1{BUTTON1};
DebouncedButton button2{BUTTON2};

LedController snapshotController() {
  portENTER_CRITICAL(&controllerMux);
  const LedController snapshot = controller;
  portEXIT_CRITICAL(&controllerMux);
  return snapshot;
}

void applyOutputs() {
  const LedOutput value = snapshotController().output();
  digitalWrite(LED_RED, value.red ? HIGH : LOW);
  digitalWrite(LED_YELLOW, value.yellow ? HIGH : LOW);
  digitalWrite(LED_GREEN, value.green ? HIGH : LOW);
}

void appendCriticalEvent(uint32_t now, float cm) {
  eventLog[eventWriteIndex] = CriticalEvent{now, cm};
  eventWriteIndex = (eventWriteIndex + 1) % EVENT_CAPACITY;
  if (eventCount < EVENT_CAPACITY) ++eventCount;
  Serial.printf("[CRITICAL] uptime_ms=%lu distance_cm=%.2f auto_paused=1 red_manual_locked=1\n",
                static_cast<unsigned long>(now), cm);
}

void dumpEventLog() {
  Serial.printf("[LOG] %u most recent critical entries; timestamps are uptime since reset.\n",
                static_cast<unsigned>(eventCount));
  const size_t start = (eventWriteIndex + EVENT_CAPACITY - eventCount) % EVENT_CAPACITY;
  for (size_t i = 0; i < eventCount; ++i) {
    const CriticalEvent& entry = eventLog[(start + i) % EVENT_CAPACITY];
    Serial.printf("  uptime_ms=%lu distance_cm=%.2f\n",
                  static_cast<unsigned long>(entry.uptimeMs), entry.distanceCm);
  }
}

void sampleDistance() {
  digitalWrite(HC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(HC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(HC_TRIG, LOW);
  const unsigned long echoUs = pulseIn(HC_ECHO, HIGH, ECHO_TIMEOUT_US);
  const float cm = echoUs * 0.0343f / 2.0f;
  const bool valid = echoUs != 0 && cm > 0.0f && cm <= 400.0f;
  uint32_t now;
  portENTER_CRITICAL(&controllerMux);
  now = millis();
  const bool enteredCritical = controller.updateDistance(valid, cm, now);
  portEXIT_CRITICAL(&controllerMux);
  // Apply the safety decision before logging or doing any network work.
  applyOutputs();
  if (enteredCritical) appendCriticalEvent(now, cm);
}

// All Blynk operations run in networkTask, never in the hardware safety loop.
void publishTelemetry() {
  if (!Blynk.connected()) return;
  const LedController value = snapshotController();
  const LedOutput leds = value.output();
  Blynk.virtualWrite(V0, leds.yellow && leds.green ? 1 : 0);
  if (value.sensorValid()) Blynk.virtualWrite(V1, value.distanceCm());
  Blynk.virtualWrite(V2, value.proximity() ? 255 : 0);
  Blynk.virtualWrite(V3, value.critical() ? 1 : 0);
  Blynk.virtualWrite(V4, value.manualRed() ? 1 : 0);
  Blynk.virtualWrite(V5, value.automatic() ? 1 : 0);
}

BLYNK_CONNECTED() {
  // Publish the current local state. Do not replay stale cloud commands on reconnect.
  publishTelemetry();
}

BLYNK_WRITE(V0) {
  const bool on = param.asInt() != 0;
  portENTER_CRITICAL(&controllerMux);
  controller.setPair(on, millis());
  portEXIT_CRITICAL(&controllerMux);
}

BLYNK_WRITE(V4) {
  const bool on = param.asInt() != 0;
  portENTER_CRITICAL(&controllerMux);
  const bool accepted = controller.setRed(on, millis());
  portEXIT_CRITICAL(&controllerMux);
  if (!accepted) {
    Serial.printf("[LOCK] uptime_ms=%lu rejected Blynk V4 while distance <10 cm\n",
                  static_cast<unsigned long>(millis()));
    Blynk.virtualWrite(V4, snapshotController().manualRed() ? 1 : 0);
  }
}

BLYNK_WRITE(V5) {
  const bool automatic = param.asInt() != 0;
  portENTER_CRITICAL(&controllerMux);
  controller.setAutomatic(automatic, millis());
  portEXIT_CRITICAL(&controllerMux);
}

void networkTask(void*) {
#if IOT_ENABLE_CLOUD
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Blynk.config(BLYNK_AUTH_TOKEN);
  telemetryTimer.setInterval(TELEMETRY_PERIOD_MS, publishTelemetry);
  uint32_t lastRetryMs = millis() - NETWORK_RETRY_MS;
  while (true) {
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      if (Blynk.connected()) {
        Blynk.run();
        telemetryTimer.run();
      } else if (static_cast<uint32_t>(now - lastRetryMs) >= NETWORK_RETRY_MS) {
        lastRetryMs = now;
        Blynk.connect(100);
      }
    }
    // A slow DNS/server/socket cannot block sensor sampling on the other core.
    vTaskDelay(pdMS_TO_TICKS(5));
  }
#else
  Serial.println("[CLOUD] Disabled. Copy secrets.example.h to secrets.h and configure Wi-Fi/Blynk.");
  vTaskDelete(nullptr);
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(HC_TRIG, OUTPUT);
  pinMode(HC_ECHO, INPUT);
  digitalWrite(HC_TRIG, LOW);
  controller.begin(millis());
  applyOutputs();
  Serial.println("[START] De 2; LED states 0..4 at 400 ms; critical<10 cm; alarm<15 cm.");
  Serial.println("[SERIAL] Send L to print the 16-entry critical event buffer.");
  const BaseType_t result = xTaskCreatePinnedToCore(networkTask, "blynk", 8192,
                                                 nullptr, 1, nullptr, 0);
  if (result != pdPASS) Serial.println("[CLOUD] Could not create network task; local control continues.");
}

void loop() {
  const uint32_t now = millis();
  portENTER_CRITICAL(&controllerMux);
  controller.tick(millis());
  portEXIT_CRITICAL(&controllerMux);

  if (button1.pressed(now)) {
    portENTER_CRITICAL(&controllerMux);
    controller.setAutomatic(!controller.automatic(), millis());
    portEXIT_CRITICAL(&controllerMux);
  }
  if (button2.pressed(now)) {
    portENTER_CRITICAL(&controllerMux);
    controller.setPair(!controller.manualPair(), millis());
    portEXIT_CRITICAL(&controllerMux);
  }

  static uint32_t lastSensorMs = millis() - SENSOR_PERIOD_MS;
  if (static_cast<uint32_t>(now - lastSensorMs) >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;
    sampleDistance();
  }
  applyOutputs();
  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 'L' || command == 'l') dumpEventLog();
  }
  delay(1);
}
