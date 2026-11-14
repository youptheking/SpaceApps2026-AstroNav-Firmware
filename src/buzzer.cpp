#include "buzzer.h"

#include "astronav_protocol.h"
#include "pins.h"

using namespace astronav;

namespace buzzer {

const uint16_t HIGH_HZ = 2700, LOW_HZ = 1800;  // piezo is rond 2,7 kHz het hardst
const uint32_t WARNING_REPEAT_MS = 30000;

static int mode = -1;
static uint32_t started = 0;
static uint32_t lastStep = 0;
static int step = 0;

void begin() {
  pinMode(pins::BUZZER, OUTPUT);
}

void beep() {
  tone(pins::BUZZER, HIGH_HZ, 120);
}

void set(int severity) {
  if (severity == SEV_INFO) severity = -1;  // info is één piep, geen patroon
  if (severity == mode) return;
  mode = severity;
  started = millis();
  step = 0;
  lastStep = 0;
  noTone(pins::BUZZER);
}

void update() {
  if (mode < 0) return;
  uint32_t now = millis();

  if (mode == SEV_ALARM) {
    // 300 ms hoog, 300 ms laag, zolang het alarm loopt
    if (lastStep && now - lastStep < 300) return;
    lastStep = now;
    tone(pins::BUZZER, step++ % 2 ? LOW_HZ : HIGH_HZ, 300);
    return;
  }

  // waarschuwing: 3x 100 ms met 100 ms ertussen, dan 30 s wachten
  uint32_t t = (now - started) % WARNING_REPEAT_MS;
  int beepNr = t / 200;
  if (beepNr < 3 && t % 200 < 100) {
    if (step != beepNr + 1) {
      step = beepNr + 1;
      tone(pins::BUZZER, HIGH_HZ, 100);
    }
  } else if (beepNr >= 3) {
    step = 0;
  }
}

}
