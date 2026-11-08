#pragma once

#include <stdint.h>

// Pure control logic: also compiled by the host test program.
struct LedOutput {
  bool red;
  bool yellow;
  bool green;
};

class LedController {
 public:
  static constexpr uint32_t STEP_MS = 400;

  void begin(uint32_t now) {
    lastTickMs_ = now;
    started_ = true;
  }

  void tick(uint32_t now) {
    if (!started_) begin(now);
    const uint32_t delta = now - lastTickMs_;
    lastTickMs_ = now;
    if (!automatic_ || critical_) return;
    const uint64_t elapsed = static_cast<uint64_t>(phaseMs_) + delta;
    state_ = static_cast<uint8_t>((state_ + elapsed / STEP_MS) % 5);
    phaseMs_ = static_cast<uint32_t>(elapsed % STEP_MS);
  }

  // Returns true only on entry into the critical zone, for one log per event.
  bool updateDistance(bool valid, float cm, uint32_t now) {
    tick(now);
    sensorValid_ = valid && cm > 0.0f && cm <= 400.0f;
    if (!sensorValid_) return false;  // Never clear an alarm on a missing echo.
    distanceCm_ = cm;
    hasDistance_ = true;
    const bool wasCritical = critical_;
    proximity_ = cm < 15.0f;
    critical_ = cm < 10.0f;
    return critical_ && !wasCritical;
  }

  void setAutomatic(bool enabled, uint32_t now) {
    tick(now);
    automatic_ = enabled;
  }

  void setPair(bool enabled, uint32_t now) {
    tick(now);
    automatic_ = false;
    manualPair_ = enabled;
  }

  bool setRed(bool enabled, uint32_t now) {
    tick(now);
    if (critical_) return false;
    automatic_ = false;
    manualRed_ = enabled;
    return true;
  }

  LedOutput output() const {
    LedOutput value = automatic_
                          ? LedOutput{state_ == 1 || state_ == 4,
                                      state_ == 2 || state_ == 4,
                                      state_ == 3 || state_ == 4}
                          : LedOutput{manualRed_, manualPair_, manualPair_};
    if (proximity_) value.red = true;
    if (critical_) value = LedOutput{true, false, false};
    return value;
  }

  bool automatic() const { return automatic_; }
  bool critical() const { return critical_; }
  bool proximity() const { return proximity_; }
  bool sensorValid() const { return sensorValid_; }
  bool hasDistance() const { return hasDistance_; }
  float distanceCm() const { return distanceCm_; }
  bool manualRed() const { return manualRed_; }
  bool manualPair() const { return manualPair_; }
  uint8_t state() const { return state_; }
  uint32_t phaseMs() const { return phaseMs_; }

 private:
  bool started_ = false;
  bool automatic_ = true;
  bool manualPair_ = false;
  bool manualRed_ = false;
  bool critical_ = false;
  bool proximity_ = false;
  bool sensorValid_ = false;
  bool hasDistance_ = false;
  float distanceCm_ = 0.0f;
  uint8_t state_ = 0;
  uint32_t phaseMs_ = 0;
  uint32_t lastTickMs_ = 0;
};
