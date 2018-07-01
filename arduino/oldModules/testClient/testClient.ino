#include "DSerial.h"
#include <NeoICSerial.h>
NeoICSerial serial_port;
DSerialClient client(serial_port, 2);
int button_state = 0;
int last_button_state = 0;

void setup() {
  Serial.begin(9600);
  serial_port.begin(9600);
  pinMode(2, INPUT);
}

void loop() {
  char out_message[MAX_MSG_LEN];
  char data[6] = "HELLO";
  client.doSerial();
  client.getData(out_message);
  button_state = digitalRead(2);
  if (button_state != last_button_state && button_state) {
  	last_button_state = button_state;
    client.sendData(data);
    delay(5);
  }
}