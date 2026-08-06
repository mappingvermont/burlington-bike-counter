#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include "detection.h"

RTC_DS3231 rtc;
File f;
Detection d;
bool sdOk = false;

// Boot into raw-pressure debug mode (broadcasts live A0 reading, like
// pressure_debug.ino) for DEBUG_DURATION_MS, so the tube/sensor can be
// checked on the display after a power cycle with no computer on site.
// Then falls through into normal bike-counting mode for the rest of the
// session.
const unsigned long DEBUG_DURATION_MS = 5UL * 60 * 1000;
const unsigned long DEBUG_ADVERTISE_INTERVAL_MS = 200;
bool debugMode = true;
unsigned long lastDebugAdvertise = 0;

void advertise(uint16_t val) {
  uint8_t mfr[4] = {0xFF, 0xFF, (uint8_t)(val & 0xFF), (uint8_t)(val >> 8)};
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addManufacturerData(mfr, sizeof(mfr));
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(160, 160);
  Bluefruit.Advertising.start(0);
}

void logEvent(const char* type, int peak) {
  DateTime now = rtc.now();
  char buf[64];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d,%s,%d,%d\n",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second(),
    type, peak, d.bikeCount);
  if (sdOk) { f.print(buf); f.flush(); }
  Serial.print(buf);
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println("setup: start");
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  advertise(0);
  Serial.println("setup: BLE ok");
  if (!rtc.begin()) {
    Serial.println("setup: RTC not found — halting");
    while (1) delay(10);
  }
  Serial.println("setup: RTC ok");
  if (!SD.begin(10)) {
    Serial.println("setup: SD not found — logging to serial only");
  } else {
    sdOk = true;
    f = SD.open("counts.csv", FILE_WRITE);
    f.println("datetime,event,peak,count");
    f.flush();
    Serial.println("setup: SD ok");
  }
  Serial.println("setup: ready, entering raw-pressure debug mode for 5 min");
}

int lastResetMinute = -1;
unsigned long lastResetCheck = 0;

void checkReset(unsigned long now) {
  if (now - lastResetCheck < 1800000) return;
  lastResetCheck = now;
  DateTime t = rtc.now();
  if (t.hour() == 0 && t.minute() == 0 && lastResetMinute != 0) {
    lastResetMinute = 0;
    d.bikeCount = 0;
    advertise(d.bikeCount);
    logEvent("reset", 0);
  }
}

void loop() {
  unsigned long now = millis();

  if (debugMode) {
    if (now >= DEBUG_DURATION_MS) {
      debugMode = false;
      Serial.println("debug window elapsed: switching to bike counting mode");
      advertise(d.bikeCount);
      lastResetCheck = now;
      return;
    }
    if (now - lastDebugAdvertise >= DEBUG_ADVERTISE_INTERVAL_MS) {
      lastDebugAdvertise = now;
      int val = analogRead(A0);
      advertise((uint16_t)val);
      Serial.println(val);
    }
    return;
  }

  int val = analogRead(A0);
  checkReset(now);

  DetectionResult result = d.tick(now, val);
  if (result.type == DE_PAIRED) {
    advertise(d.bikeCount);
    logEvent("bike", result.peak);
  } else if (result.type == DE_UNPAIRED) {
    advertise(d.bikeCount);
    logEvent("single", result.peak);
  }
}
