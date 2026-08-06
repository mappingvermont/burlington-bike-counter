// Debug firmware: BLE-broadcasts the live A0 pressure reading (not bike count)
// so it can be watched on the display without a USB connection.
// Pair with display/debug_pressure.py on the Matrix Portal.
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>

const unsigned long ADVERTISE_INTERVAL_MS = 200;

void advertise(uint16_t val) {
  uint8_t mfr[4] = {0xFF, 0xFF, (uint8_t)(val & 0xFF), (uint8_t)(val >> 8)};
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addManufacturerData(mfr, sizeof(mfr));
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(160, 160);
  Bluefruit.Advertising.start(0);
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println("pressure_debug: start");
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  advertise(0);
  Serial.println("pressure_debug: BLE ok, broadcasting A0 reading");
}

unsigned long lastAdvertise = 0;

void loop() {
  unsigned long now = millis();
  if (now - lastAdvertise >= ADVERTISE_INTERVAL_MS) {
    lastAdvertise = now;
    int val = analogRead(A0);
    advertise((uint16_t)val);
    Serial.println(val);
  }
}
