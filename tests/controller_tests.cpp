#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/IoT_ESP32/Controller.h"
#include "../firmware/IoT_ESP32/ButtonDebouncer.h"

static void expect(const LedController& c, bool r, bool y, bool g) {
  const LedOutput value = c.output();
  assert(value.red == r && value.yellow == y && value.green == g);
}

static void testButton(bool idleLevel) {
  ButtonDebouncer button;
  button.begin(idleLevel, 0);
  assert(button.idleLevel() == idleLevel);
  assert(!button.update(idleLevel, 100));
  assert(!button.update(!idleLevel, 110));
  assert(!button.update(idleLevel, 115));  // contact bounce
  assert(!button.update(!idleLevel, 120));
  assert(!button.update(!idleLevel, 149));
  assert(button.update(!idleLevel, 150));  // one event after stable 30 ms
  assert(!button.update(!idleLevel, 300)); // holding does not repeat
  assert(!button.update(idleLevel, 310));
  assert(!button.update(idleLevel, 340));  // release is not a press
  assert(!button.update(!idleLevel, 350));
  assert(button.update(!idleLevel, 380));  // a second press is detected

  ButtonDebouncer rollover;
  rollover.begin(idleLevel, UINT32_MAX - 100U);
  assert(!rollover.update(!idleLevel, UINT32_MAX - 9U));
  assert(!rollover.update(!idleLevel, 19U));
  assert(rollover.update(!idleLevel, 20U));
}

int main() {
  LedController c;
  c.begin(0);
  expect(c, false, false, false);
  c.tick(399); expect(c, false, false, false);
  c.tick(400); expect(c, true, false, false);
  c.tick(800); expect(c, false, true, false);
  c.tick(1200); expect(c, false, false, true);
  c.tick(1600); expect(c, true, true, true);
  c.tick(2000); expect(c, false, false, false);

  assert(!c.updateDistance(true, 15.0f, 2000));
  assert(!c.proximity() && !c.critical());
  assert(!c.updateDistance(true, 14.9f, 2000));
  assert(c.proximity() && !c.critical()); expect(c, true, false, false);
  assert(!c.updateDistance(true, 10.0f, 2000));
  assert(c.proximity() && !c.critical());
  assert(c.updateDistance(true, 9.9f, 2100));
  assert(c.critical() && c.phaseMs() == 100); expect(c, true, false, false);
  assert(!c.updateDistance(true, 5.0f, 2200));  // no duplicate entry event
  assert(!c.setRed(false, 2300));
  assert(!c.setRed(true, 2400));
  c.tick(12000);
  assert(c.state() == 0 && c.phaseMs() == 100);

  // Missing echo must not become 0 cm, clear the lock, or overwrite last good distance.
  assert(!c.updateDistance(false, 0.0f, 12100));
  assert(!c.sensorValid() && c.critical() && c.distanceCm() == 5.0f);
  assert(!c.updateDistance(true, 401.0f, 12200));
  assert(c.critical());
  assert(!c.updateDistance(true, 10.0f, 12300));
  assert(!c.critical() && c.proximity());
  c.tick(12599); assert(c.state() == 0 && c.phaseMs() == 399);
  c.tick(12600); assert(c.state() == 1 && c.phaseMs() == 0);

  // Manual pair takes ownership, but both alarm priorities still win.
  c.setPair(true, 12600); expect(c, true, true, true);
  c.updateDistance(true, 30.0f, 12600); expect(c, false, true, true);
  assert(c.setRed(true, 12600)); expect(c, true, true, true);
  assert(c.updateDistance(true, 2.0f, 12700)); expect(c, true, false, false);
  c.setPair(false, 12700); expect(c, true, false, false);
  c.setAutomatic(true, 12700); expect(c, true, false, false);
  c.updateDistance(true, 15.0f, 13000); expect(c, true, false, false);
  c.tick(13400); expect(c, false, true, false);

  // millis() rollover is handled by unsigned subtraction.
  LedController rollover;
  rollover.begin(UINT32_MAX - 199U);
  rollover.tick(200U);
  assert(rollover.state() == 1 && rollover.phaseMs() == 0);

  // Late polling catches up the phase instead of stretching every state.
  LedController late;
  late.begin(10);
  late.tick(2010);
  assert(late.state() == 0 && late.phaseMs() == 0);
  late.tick(3210);
  assert(late.state() == 3);
  testButton(true);   // released HIGH, pressed LOW
  testButton(false);  // released LOW, pressed HIGH
  puts("PASS: 10 groups: sequence, timing, thresholds, priority, pause/resume, invalid echo, lock, rollover/catch-up, active-LOW button, active-HIGH button.");
  return 0;
}
