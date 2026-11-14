#include "sampler.h"

#include <esp_task_wdt.h>

#include "astronav_protocol.h"
#include "drivers/drivers.h"
#include "sensors.h"

using namespace astronav;

namespace sampler {

const uint32_t TICK_MS = 4;

struct Raw {
  uint8_t len;
  uint8_t data[sizeof(PpgPacket)];  // grootste pakket
};

static QueueHandle_t queue;
static volatile bool raw = true;
static volatile uint32_t lost = 0;

void push(const void *packet, size_t len) {
  Raw r;
  r.len = min(len, sizeof(r.data));
  memcpy(r.data, packet, r.len);
  if (xQueueSend(queue, &r, 0) != pdTRUE) lost++;
}

bool pop(uint8_t *buf, size_t &len) {
  Raw r;
  if (xQueueReceive(queue, &r, 0) != pdTRUE) return false;
  memcpy(buf, r.data, r.len);
  len = r.len;
  return true;
}

void setRawStreams(bool on) { raw = on; }
bool rawStreams() { return raw; }
uint32_t dropped() { return lost; }

static void task(void *) {
  esp_task_wdt_add(nullptr);
  TickType_t wake = xTaskGetTickCount();
  uint32_t tick = 0;

  bool ecg = sensors::has(OK_AD8232);
  bool gsr = sensors::has(OK_GSR);
  bool imu = sensors::has(OK_MPU6050);
  bool ppg = sensors::has(OK_MAX30102);

  for (;;) {
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(TICK_MS));
    uint32_t now = millis();

    if (ecg) ecgSample(now);          // 250 Hz
    if (imu && tick % 5 == 0) mpu6050Sample(now);  // 50 Hz
    if (ppg && tick % 10 == 2) max30102Sample(now);  // fifo elke 40 ms legen
    if (gsr && tick % 25 == 4) gsrSample(now);     // 10 Hz
    if (ecg && tick % 250 == 7) rrSend(now);       // elke seconde

    tick++;
    esp_task_wdt_reset();
  }
}

void begin() {
  queue = xQueueCreate(32, sizeof(Raw));
  // core 1 naast loop(), bluetooth zit op core 0. Hogere prioriteit dan
  // loop(), zodat het scherm of de flash de samples niet ophoudt.
  xTaskCreatePinnedToCore(task, "sampler", 6144, nullptr, 3, nullptr, 1);
}

}
