#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define SWITCH_PIN_BASE 9
#define LED_PIN_BASE 14

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

uint8_t switch_state, last_switch_state;
uint8_t bad[10] = {4,11,15,18,19,23,24,26,28,30};
uint8_t good[22] = {0,1,2,3,5,6,7,8,9,10,12,13,14,16,17,20,21,22,25,27,29,31};

uint8_t goal = good[random(0,22)];

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  
  while(!module.getConfig()){
    module.interpretData();
  }

  for(int i = 0; i < 5; i++) {
    pinMode(SWITCH_PIN_BASE + i, INPUT);
    pinMode(LED_PIN_BASE + i, OUTPUT);
    digitalWrite(LED_PIN_BASE + i, (goal >> i) & 1);
  }

  module.sendReady();
}

void loop() {
  module.interpretData();

  if(!module.is_solved){
    last_switch_state = switch_state;
    switch_state = 0;
    for(int i = 0; i < 5; i++) {
      switch_state |= (digitalRead(SWITCH_PIN_BASE + i) << i);
    }
    if(switch_state != last_switch_state) {
      for(int i = 0; i < 10; i++){
        if(switch_state == bad[i]){
          module.strike();
        }
      }
      if(switch_state == goal){
        module.win();
      }
    }
  }
}