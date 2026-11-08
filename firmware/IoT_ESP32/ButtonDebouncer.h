#pragma once

#include <stdint.h>

// Calibrate with a released button. Supports active-LOW and active-HIGH modules.
class ButtonDebouncer {
 public:
  static constexpr uint32_t DEBOUNCE_MS = 30;

  void begin(bool releasedLevel, uint32_t now) {
    idleLevel_ = releasedLevel;
    previousRaw_ = releasedLevel;
    stable_ = releasedLevel;
    changedMs_ = now;
  }

  bool update(bool raw, uint32_t now) {
    if (raw != previousRaw_) {
      previousRaw_ = raw;
      changedMs_ = now;
    }
    if (raw != stable_ && static_cast<uint32_t>(now - changedMs_) >= DEBOUNCE_MS) {
      stable_ = raw;
      return stable_ != idleLevel_;
    }
    return false;
  }

  bool idleLevel() const { return idleLevel_; }

 private:
  bool idleLevel_ = true;
  bool previousRaw_ = true;
  bool stable_ = true;
  uint32_t changedMs_ = 0;
};
