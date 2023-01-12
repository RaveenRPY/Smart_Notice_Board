#define BLYNK_TEMPLATE_ID "TMPLo09e7pqv"
#define BLYNK_DEVICE_NAME "SmartNotice"

#define BLYNK_FIRMWARE_VERSION "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG

#include "BlynkEdgent.h"
#include <SoftwareSerial.h>
#include <Wire.h>

String msg;
String tSize;

SoftwareSerial text(14, 2);
SoftwareSerial size(13, 15);

BLYNK_WRITE(V0) {
  msg = param.asStr();
  text.println(msg);
  Serial.println(msg);
}

BLYNK_WRITE(V1) {
  tSize = param.asStr();
  size.println(tSize);
  Serial.println(tSize);
}

void setup() {
  Serial.begin(115200);
  size.begin(115200);
  text.begin(115200);
  delay(100);

  BlynkEdgent.begin();
}

void loop() {
  BlynkEdgent.run();
}
