#include "ble_link.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "6e0a0001-5a3c-4c1e-9d2a-41535452304e"
#define TELEMETRY_UUID "6e0a0002-5a3c-4c1e-9d2a-41535452304e"
#define ECG_UUID "6e0a0003-5a3c-4c1e-9d2a-41535452304e"
#define COMMAND_UUID "6e0a0004-5a3c-4c1e-9d2a-41535452304e"
#define REPLY_UUID "6e0a0005-5a3c-4c1e-9d2a-41535452304e"

namespace ble {

struct Command {
  char text[128];
};

static BLEServer *server;
static BLECharacteristic *telemetryChar;
static BLECharacteristic *ecgChar;
static BLECharacteristic *replyChar;
static QueueHandle_t commandQueue;

static volatile bool linked = false;
static uint32_t code;
static char deviceName[16];

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

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string value = c->getValue();
    Command cmd = {};
    strncpy(cmd.text, value.c_str(), sizeof(cmd.text) - 1);
    // draait in de bluetooth-task, dus alleen doorgeven aan loop()
    xQueueSend(commandQueue, &cmd, 0);
  }
};

static BLECharacteristic *addNotifyChar(BLEService *service, const char *uuid, uint32_t props) {
  BLECharacteristic *c = service->createCharacteristic(uuid, props | BLECharacteristic::PROPERTY_NOTIFY);
  c->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);

  BLE2902 *cccd = new BLE2902();
  cccd->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  c->addDescriptor(cccd);
  return c;
}

void begin() {
  // code en naam uit het mac-adres halen: per horloge anders, maar blijft altijd hetzelfde
  uint64_t mac = ESP.getEfuseMac();
  code = 100000 + (uint32_t)(mac % 900000);
  snprintf(deviceName, sizeof(deviceName), "AstroNav-%04X", (uint16_t)(mac >> 32));

  commandQueue = xQueueCreate(8, sizeof(Command));

  BLEDevice::init(deviceName);
  BLEDevice::setMTU(247);
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(BLEUUID(SERVICE_UUID), 20);
  telemetryChar = addNotifyChar(service, TELEMETRY_UUID, BLECharacteristic::PROPERTY_READ);
  ecgChar = addNotifyChar(service, ECG_UUID, 0);
  replyChar = addNotifyChar(service, REPLY_UUID, 0);

  BLECharacteristic *commandChar = service->createCharacteristic(
      COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  commandChar->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  commandChar->setCallbacks(new CommandCallbacks());

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
  scanData.setName(deviceName);

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);
  BLEDevice::startAdvertising();

  Serial.printf("bluetooth: %s, code %06lu\n", deviceName, code);
}

bool connected() { return linked; }
uint32_t passkey() { return code; }
const char *name() { return deviceName; }

void send(const Telemetry &t) {
  telemetryChar->setValue((uint8_t *)&t, sizeof(t));
  if (linked) telemetryChar->notify();
}

void sendEcg(const int16_t *samples, size_t count) {
  if (!linked) return;
  static uint8_t counter = 0;
  uint8_t buf[1 + 2 * 100];
  count = min(count, (size_t)100);
  buf[0] = counter++;
  memcpy(buf + 1, samples, count * 2);
  ecgChar->setValue(buf, 1 + count * 2);
  ecgChar->notify();
}

void reply(const char *text) {
  if (!linked) return;
  replyChar->setValue((uint8_t *)text, strlen(text));
  replyChar->notify();
}

bool readCommand(String &out) {
  Command cmd;
  if (xQueueReceive(commandQueue, &cmd, 0) != pdTRUE) return false;
  out = cmd.text;
  return true;
}

}
