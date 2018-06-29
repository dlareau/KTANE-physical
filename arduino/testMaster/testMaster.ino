#include "DSerial.h"
#include "dserial.h"
#include <NeoICSerial.h>
NeoICSerial serial_port;
DSerialMaster master(serial_port);

void setup() {
  Serial.begin(9600);
  serial_port.begin(9600);
  uint8_t a[32];
  master.getClients(a);
}

void loop() {
  char out_message[MAX_MSG_LEN];

  if (master.getData(out_message)) {
    Serial.print(out_message);
  }
  delay(10);

  master.doSerial();
}