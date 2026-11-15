#include "ble_link.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>

using namespace astronav;

namespace ble {

// Geen koppelen op bluetooth-niveau: Windows en btleplug konden de codevraag
// niet goed afhandelen, en een telefoon viel na het koppelen weer weg. Nu mag
// iedereen verbinden, maar het horloge stuurt niks en luistert naar niks tot
// de app de code van het scherm stuurt (control 0x13). Bij verbreken gaat hij
// weer op slot.
const int MAX_TRIES = 5;

struct Control {
  uint8_t len;
  uint8_t data[20];
};

static BLEServer *server;
static BLECharacteristic *dataChar;
static BLECharacteristic *eventsChar;
static BLECharacteristic *historyChar;
static QueueHandle_t controlQueue;
// events gaan via een eigen task: indicate() wacht tot 1 s op de bevestiging
// van de laptop, en dat mag loop() (sensoren, scherm, watchdog) niet ophouden
static QueueHandle_t eventQueue;

static volatile bool link = false;      // er is iemand verbonden
static volatile bool unlocked = false;  // en die heeft de goede code gestuurd
static volatile int tries = 0;
static uint32_t code;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s) override {
    link = true;
    unlocked = false;
    tries = 0;
  }

  void onDisconnect(BLEServer *s) override {
    link = false;
    unlocked = false;
    s->startAdvertising();
  }
};

class ControlCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string value = c->getValue();
    if (value.empty()) return;

    if (!unlocked) {
      // op slot: alleen de code telt, al het andere wordt genegeerd
      uint32_t given = 0;
      if ((uint8_t)value[0] == CMD_UNLOCK && value.size() >= 5) memcpy(&given, value.data() + 1, 4);
      if (given == code) {
        unlocked = true;
        Serial.println("bluetooth: ontgrendeld");
      } else if (++tries >= MAX_TRIES) {
        Serial.println("bluetooth: te vaak een verkeerde code, verbinding verbroken");
        server->disconnect(server->getConnId());
      }
      return;
    }

    Control cmd = {};
    cmd.len = min(value.size(), sizeof(cmd.data));
    memcpy(cmd.data, value.data(), cmd.len);
    // draait in de bluetooth-task, dus alleen doorgeven aan loop()
    xQueueSend(controlQueue, &cmd, 0);
  }
};

static void eventTask(void *) {
  EventPacket e;
  for (;;) {
    if (xQueueReceive(eventQueue, &e, portMAX_DELAY) != pdTRUE) continue;
    if (!unlocked) continue;  // verbinding weg terwijl hij in de rij stond
    eventsChar->setValue((uint8_t *)&e, sizeof(e));
    eventsChar->indicate();
  }
}

static BLECharacteristic *addChar(BLEService *service, const char *uuid, uint32_t props) {
  BLECharacteristic *c = service->createCharacteristic(uuid, props);
  c->addDescriptor(new BLE2902());
  return c;
}

void begin() {
  // code uit het mac-adres: per horloge anders, maar blijft altijd hetzelfde
  uint64_t mac = ESP.getEfuseMac();
  code = 100000 + (uint32_t)(mac % 900000);

  controlQueue = xQueueCreate(8, sizeof(Control));
  eventQueue = xQueueCreate(16, sizeof(EventPacket));
  // core 0 bij de bluetooth-stack, lage prioriteit: wachten mag hier
  xTaskCreatePinnedToCore(eventTask, "ble-events", 4096, nullptr, 1, nullptr, 0);

  BLEDevice::init(DEVICE_NAME);
  BLEDevice::setMTU(247);

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(BLEUUID(SERVICE_UUID), 16);
  dataChar = addChar(service, CHAR_DATA_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  eventsChar = addChar(service, CHAR_EVENTS_UUID, BLECharacteristic::PROPERTY_INDICATE);
  historyChar = addChar(service, CHAR_HISTORY_UUID, BLECharacteristic::PROPERTY_NOTIFY);

  BLECharacteristic *control = service->createCharacteristic(
      CHAR_CONTROL_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  control->setCallbacks(new ControlCallbacks());

  service->start();

  // uuid en naam passen samen niet in 31 bytes, dus naam in de scan response
  BLEAdvertisementData advData;
  advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));

  BLEAdvertisementData scanData;
  scanData.setName(DEVICE_NAME);

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);
  BLEDevice::startAdvertising();

  Serial.printf("bluetooth: %s, code %06lu\n", DEVICE_NAME, code);
}

bool connected() { return unlocked; }
bool linked() { return link; }
uint32_t passkey() { return code; }
const char *name() { return DEVICE_NAME; }

void sendData(const void *packet, size_t len) {
  if (!unlocked) return;
  dataChar->setValue((uint8_t *)packet, len);
  dataChar->notify();
}

void sendEvent(const EventPacket &e) {
  if (!unlocked) return;
  // vol = de laptop bevestigt niks meer; dan liever een event kwijt dan een hangend horloge
  if (xQueueSend(eventQueue, &e, 0) != pdTRUE) Serial.println("bluetooth: event-rij vol, event overgeslagen");
}

void sendHistory(const uint8_t *data, size_t len) {
  if (!unlocked) return;
  historyChar->setValue((uint8_t *)data, len);
  historyChar->notify();
}

size_t maxPayload() {
  if (!link) return 20;
  return server->getPeerMTU(server->getConnId()) - 3;
}

bool readControl(uint8_t *buf, size_t &len) {
  Control cmd;
  if (xQueueReceive(controlQueue, &cmd, 0) != pdTRUE) return false;
  memcpy(buf, cmd.data, cmd.len);
  len = cmd.len;
  return true;
}

}
