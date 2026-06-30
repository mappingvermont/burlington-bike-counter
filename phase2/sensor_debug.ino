#include <Adafruit_TinyUSB.h>

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println("sensor_debug: start");
}

void loop() {
  int val = analogRead(A0);
  Serial.println(val);
  delay(10); // 100 samples/sec
}
