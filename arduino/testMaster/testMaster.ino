#include "DSerial.h"
#include "dserial.h"
#include <NeoICSerial.h>
NeoICSerial serial_port;
DSerialMaster master(&serial_port);

void setup() {
  Serial.begin(9600);
  Serial.println("AltSoftSerial Test Begin");
  serial_port.begin(9600);
  serial_port.println("Hello World");
}

void loop() {
  char c;
  char data[2] = " ";
  char out_message[MAX_MSG_LEN];

  if (Serial.available()) {
    c = Serial.read();
    data[0] = c;
    master.sendData(2, data);
  }
  if (master.getData(out_message)) {
    Serial.print(out_message);
  }
  master.doSerial();
}