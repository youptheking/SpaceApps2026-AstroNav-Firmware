#include "ble_link.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>

using namespace astronav;

namespace ble {

struct Control {
  uint8_t len;
  uint8_t data[20];
};

static BLEServer *server;
static BLECharacteristic *dataChar;
static BLECharacteristic *eventsChar;
static BLECharacteristic *historyChar;
static QueueHandle_t controlQueue;

static volatile bool linked = false;
static uint32_t code;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s, esp_ble_gatts_cb_param_t *param) override {
    // direct om koppeling vragen, dan krijgt de laptop meteen de codevraag
    esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
  }

  void onDisconnect(BLEServer *s) override {
    linked = false;
    s->startAdvertising();
  }
};

class SecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override { return code; }
  void onPassKeyNotify(uint32_t) override {}
  bool onSecurityRequest() override { return true; }
  bool onConfirmPIN(uint32_t) override { return false; }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t result) override {
    if (result.success) {
      linked = true;
    } else {
      Serial.printf("koppelen mislukt (reden 0x%02X)\n", result.fail_reason);
      server->disconnect(server->getConnId());
    }
  }
};

class ControlCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string value = c->getValue();
    Control cmd = {};
    cmd.len = min(value.size(), sizeof(cmd.data));
    memcpy(cmd.data, value.data(), cmd.len);
    // draait in de bluetooth-task, dus alleen doorgeven aan loop()
    xQueueSend(controlQueue, &cmd, 0);
  }
};

static BLECharacteristic *addChar(BLEService *service, const char *uuid, uint32_t props) {
  BLECharacteristic *c = service->createCharacteristic(uuid, props);
  c->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);

  BLE2902 *cccd = new BLE2902();
  cccd->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  c->addDescriptor(cccd);
  return c;
}

void begin() {
  // code uit het mac-adres: per horloge anders, maar blijft altijd hetzelfde
  uint64_t mac = ESP.getEfuseMac();
  code = 100000 + (uint32_t)(mac % 900000);

  controlQueue = xQueueCreate(8, sizeof(Control));

  BLEDevice::init(DEVICE_NAME);
  BLEDevice::setMTU(247);
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(BLEUUID(SERVICE_UUID), 16);
  dataChar = addChar(service, CHAR_DATA_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  eventsChar = addChar(service, CHAR_EVENTS_UUID, BLECharacteristic::PROPERTY_INDICATE);
  historyChar = addChar(service, CHAR_HISTORY_UUID, BLECharacteristic::PROPERTY_NOTIFY);

  BLECharacteristic *control = service->createCharacteristic(CHAR_CONTROL_UUID, BLECharacteristic::PROPERTY_WRITE);
  control->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  control->setCallbacks(new ControlCallbacks());

  service->start();

  BLESecurity *security = new BLESecurity();
  security->setStaticPIN(code);
  // setStaticPIN zet zelf alleen SC aan, zonder MITM vraagt windows geen code
  security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  security->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

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

bool connected() { return linked; }
uint32_t passkey() { return code; }
const char *name() { return DEVICE_NAME; }

void sendData(const void *packet, size_t len) {
  if (!linked) return;
  dataChar->setValue((uint8_t *)packet, len);
  dataChar->notify();
}

void sendEvent(const EventPacket &e) {
  if (!linked) return;
  eventsChar->setValue((uint8_t *)&e, sizeof(e));
  eventsChar->indicate();
}

void sendHistory(const uint8_t *data, size_t len) {
  if (!linked) return;
  historyChar->setValue((uint8_t *)data, len);
  historyChar->notify();
}

size_t maxPayload() {
  if (!linked) return 20;
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
