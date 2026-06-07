// Detection parameters derived from characterization test 2026-05-31
// Noise floor: 34 ADC, std 3.68. Weakest real event: peak 68, duration 7ms.
// Front/rear gap: 320-800ms observed. Third bounce pulse sometimes follows.

#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

const int THRESHOLD    = 65;
const int MIN_PULSE_MS = 2;   // 5ms calibrated at slow speed; 2ms supports fast riders
const int MIN_PAIR_GAP = 70;  // 70ms = 30mph ceiling with 95cm wheelbase; bounces top out at ~22ms so safe
const int MAX_PAIR_GAP = 1500;

RTC_DS3231 rtc;

File f;
int bikeCount = 0;

enum State { IDLE, IN_PULSE_1, BETWEEN, IN_PULSE_2 };
State state = IDLE;

unsigned long pulseStart = 0;
unsigned long pulse1End  = 0;
int pulsePeak = 0;

void advertise() {
  uint8_t mfr[4] = {0xFF, 0xFF, (uint8_t)(bikeCount & 0xFF), (uint8_t)(bikeCount >> 8)};
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addManufacturerData(mfr, sizeof(mfr));
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(160, 160); // 100ms in 0.625ms units
  Bluefruit.Advertising.start(0);
}

void logEvent(const char* type, int peak) {
  DateTime now = rtc.now();
  char buf[64];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d,%s,%d,%d\n",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second(),
    type, peak, bikeCount);
  f.print(buf);
  f.flush();
  Serial.print(buf);
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println("setup: start");
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  advertise();
  Serial.println("setup: BLE ok");
  if (!rtc.begin()) {
    Serial.println("setup: RTC not found — halting");
    while (1) delay(10);
  }
  Serial.println("setup: RTC ok");
  if (!SD.begin(10)) {
    Serial.println("setup: SD init failed — halting");
    while (1) delay(10);
  }
  f = SD.open("counts.csv", FILE_WRITE);
  f.println("datetime,event,peak,count");
  f.flush();
  Serial.println("setup: SD ok — ready");
}

void loop() {
  unsigned long now = millis();
  int val = analogRead(A0);
bool above = val > THRESHOLD;

  switch (state) {

    case IDLE:
      if (above) {
        state      = IN_PULSE_1;
        pulseStart = now;
        pulsePeak  = val;
      }
      break;

    case IN_PULSE_1:
      if (above) {
        pulsePeak = max(pulsePeak, val);
      } else {
        if (now - pulseStart >= MIN_PULSE_MS) {
          pulse1End = now;
          state = BETWEEN;
        } else {
          state = IDLE;
        }
      }
      break;

    case BETWEEN:
      if (now - pulse1End > MAX_PAIR_GAP) {
        logEvent("unpaired", pulsePeak);
        state = IDLE;
      } else if (above && (now - pulse1End >= MIN_PAIR_GAP)) {
        state      = IN_PULSE_2;
        pulseStart = now;
        pulsePeak  = val;
      }
      break;

    case IN_PULSE_2:
      if (above) {
        pulsePeak = max(pulsePeak, val);
      } else {
        if (now - pulseStart >= MIN_PULSE_MS) {
          bikeCount++;
          advertise();
          logEvent("bike", pulsePeak);
          state = IDLE;
        } else {
          // brief dip at pulse edge — stay in pairing window if still within MAX_PAIR_GAP
          state = (now - pulse1End <= MAX_PAIR_GAP) ? BETWEEN : IDLE;
        }
      }
      break;
  }
}
