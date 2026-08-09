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
const unsigned long DEBUG_DURATION_MS = 1UL * 60 * 1000;
const unsigned long DEBUG_ADVERTISE_INTERVAL_MS = 200;
bool debugMode = true;
unsigned long lastDebugAdvertise = 0;

// Near-miss diagnostic flash: briefly advertise a sentinel count so the
// existing (unmodified) display renders it, then revert to the real count.
// Relies on the display re-rendering on any count change.
const uint16_t NEARMISS_FLASH_VALUE = 9999;
const unsigned long NEARMISS_FLASH_MS = 1000;
bool nearMissFlashing = false;
unsigned long nearMissFlashUntil = 0;

void advertise(uint16_t val) {
  uint8_t mfr[4] = {0xFF, 0xFF, (uint8_t)(val & 0xFF), (uint8_t)(val >> 8)};
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addManufacturerData(mfr, sizeof(mfr));
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(160, 160);
  Bluefruit.Advertising.start(0);
}

void logEvent(const char* type, int peak, int rawPeak, float baseline, float smoothed) {
  DateTime now = rtc.now();
  char buf[96];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d,%s,%d,%d,%d,%.1f,%.1f\n",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second(),
    type, peak, rawPeak, d.bikeCount, baseline, smoothed);
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
    f.println("datetime,event,peak,rawPeak,count,baseline,smoothed");
    f.flush();
    Serial.println("setup: SD ok");
  }
  Serial.println("setup: ready, entering raw-pressure debug mode for 1 min");
}

void loop() {
  unsigned long now = millis();

  if (debugMode) {
    if (now >= DEBUG_DURATION_MS) {
      debugMode = false;
      Serial.println("debug window elapsed: switching to bike counting mode");
      advertise(d.bikeCount);
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

  if (nearMissFlashing && now >= nearMissFlashUntil) {
    nearMissFlashing = false;
    advertise(d.bikeCount);
  }

  DetectionResult result = d.tick(now, val);
  if (result.type == DE_PAIRED) {
    nearMissFlashing = false;
    advertise(d.bikeCount);
    logEvent("bike", result.peak, result.rawPeak, d.baseline, d.smoothed);
  } else if (result.type == DE_UNPAIRED) {
    nearMissFlashing = false;
    advertise(d.bikeCount);
    logEvent("single", result.peak, result.rawPeak, d.baseline, d.smoothed);
  } else if (result.type == DE_NEARMISS) {
    nearMissFlashing = true;
    nearMissFlashUntil = now + NEARMISS_FLASH_MS;
    advertise(NEARMISS_FLASH_VALUE);
    logEvent("nearmiss", result.peak, result.rawPeak, d.baseline, d.smoothed);
  }
}
