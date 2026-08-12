// Vent characterization test: no detection logic, no bike counting.
// Advertises the live raw A0 reading over BLE (like pressure_debug.ino, so
// the display shows something during the test) and independently appends
// one row every LOG_INTERVAL_MS to vent_test.csv: datetime,ms,val,tempC.
// ms (millis() since boot) exists because LOG_INTERVAL_MS is sub-second —
// the RTC datetime alone can't order/space samples within the same second.
// Deliberately as simple as possible after the prior full detection+SD
// session failed logging completely — fewer moving parts to fail here.
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

RTC_DS3231 rtc;
File f;
bool sdOk = false;
const uint16_t SD_WAITING_VALUE = 0;      // held on display while boot checks are still running
const uint16_t SD_FAIL_VALUE    = 9999;   // held on display if SD write can't be verified
const uint16_t SD_OK_VALUE      = 1111;   // held on display once SD write is verified
const unsigned long SD_OK_HOLD_MS      = 2000;  // short since it's just a confirmation
const unsigned long SD_FAIL_HOLD_MS    = 8000;  // long since this one demands action
const unsigned long SD_WAITING_HOLD_MS = 1500;  // checks below run in ms otherwise — needs its own floor to be seen at all

const unsigned long ADVERTISE_INTERVAL_MS = 200;
const unsigned long LOG_INTERVAL_MS = 200;
unsigned long lastAdvertise = 0;
unsigned long lastLog = 0;

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
  Serial.println("vent_test: start");

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  advertise(SD_WAITING_VALUE);
  delay(SD_WAITING_HOLD_MS);  // guarantee 0000 is actually visible before the checks below can overwrite it
  Serial.println("vent_test: BLE ok");

  if (!rtc.begin()) {
    Serial.println("vent_test: RTC not found — halting");
    while (1) delay(10);
  }
  Serial.println("vent_test: RTC ok");

  bool sdVerified = false;

  if (!SD.begin(10)) {
    Serial.println("vent_test: SD not found");
  } else {
    f = SD.open("VENT.CSV", FILE_WRITE);
    if (!f) {
      Serial.println("vent_test: SD open failed");
    } else {
      f.println("datetime,ms,val,tempC");
      f.flush();
      f.close();

      // Read back what was just written instead of trusting begin()/open()
      // return values alone — those can succeed even if the actual write
      // silently fails (full/corrupt card, marginal contact, etc.), which
      // is the failure mode that's bitten this project before.
      File check = SD.open("VENT.CSV", FILE_READ);
      if (check) {
        char rb[24] = {0};
        check.readBytesUntil('\n', rb, sizeof(rb) - 1);
        check.close();
        if (strncmp(rb, "datetime,ms,val,tempC", 21) == 0) {
          sdVerified = true;
        } else {
          Serial.println("vent_test: SD readback mismatch");
        }
      } else {
        Serial.println("vent_test: SD readback open failed");
      }

      if (sdVerified) {
        f = SD.open("VENT.CSV", FILE_WRITE);  // reopen for append, used by loop()
        sdOk = true;
      }
    }
  }

  // Last thing setup() does: hold an unmissable, unambiguous value on the
  // display before any real ADC readings start advertising. Deliberately
  // placed here (verified SD status, not just begin()/open() success) and
  // held long enough to actually be seen, unlike the old single advertise()
  // call that got overwritten within one ADVERTISE_INTERVAL_MS of loop()
  // starting. Display sequence: 0000 (waiting, set above) -> 9999 for 2s if
  // verified, or 8888 held for 8s if not.
  advertise(sdVerified ? SD_OK_VALUE : SD_FAIL_VALUE);
  Serial.println(sdVerified ? "vent_test: SD verified ok" : "vent_test: SD FAILED — logging to serial only");
  delay(sdVerified ? SD_OK_HOLD_MS : SD_FAIL_HOLD_MS);
}

void loop() {
  unsigned long now = millis();

  if (now - lastAdvertise >= ADVERTISE_INTERVAL_MS) {
    lastAdvertise = now;
    int val = analogRead(A0);
    advertise((uint16_t)val);
  }

  if (now - lastLog >= LOG_INTERVAL_MS) {
    lastLog = now;
    int val = analogRead(A0);
    float tempC = rtc.getTemperature();
    DateTime dt = rtc.now();
    char buf[56];
    // RTC only resolves to the second, and LOG_INTERVAL_MS now logs several
    // times per second — ms (millis() since boot) is what actually
    // distinguishes and orders samples within the same RTC second.
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d,%lu,%d,%.1f\n",
      dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second(),
      now, val, tempC);
    if (sdOk) { f.print(buf); f.flush(); }
    Serial.print(buf);
  }
}
