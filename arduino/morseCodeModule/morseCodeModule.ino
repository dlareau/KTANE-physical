#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include "morse.h"

#define BUTTON_L_PIN 5
#define BUTTON_R_PIN 6
#define BUTTON_TX_PIN 7

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

int goal_freq = random(0, 16);
int selected_freq = 0;

unsigned long last_button_time = 0;

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  
  while(!module.getConfig()){
    module.interpretData();
  }

  module.sendReady();
}

void doMorse() {
  // Non-blockingly display morse word
}

void loop() {
  module.interpretData();
  doMorse();

  if(!module.is_solved){
    if(digitalRead(BUTTON_L_PIN) && (millis() - last_button_time > 250)) {
      last_button_time = millis();
      selected_freq--;
      if(selected_freq < 0) {
        selected_freq = 0;
      }
    }
    if(digitalRead(BUTTON_R_PIN) && (millis() - last_button_time > 250)) {
      last_button_time = millis();
      selected_freq++;
      if(selected_freq > 15){
        selected_freq = 15;
      }
    }

    // Update freq digits

    if(digitalRead(BUTTON_TX_PIN)) {
      if(selected_freq == goal_freq) {
        module.win();
      } else {
        module.strike();
      }
    }
  }
}