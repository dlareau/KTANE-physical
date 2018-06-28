#include "dserial.h"
#include <NeoICSerial.h>
NeoICSerial serial_port;
DSerialClient client(&serial_port, 2);

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
    client.sendData(data);
  }
  if (client.getData(out_message)) {
    Serial.print(out_message);
  }
  client.doSerial();
}