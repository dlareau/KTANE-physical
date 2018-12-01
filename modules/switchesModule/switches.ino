#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define SWITCH_PIN_BASE 9
#define LED_PIN_BASE 14


NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

uint8_t switch_state, last_switch_state;
int leds[5] = {12,11,7,6,5};
int switches[5] = {A0, A1, A2, A3, A4};
uint8_t bad[10] = {4,11,15,18,19,23,24,26,28,30};
uint8_t good[22] = {0,1,2,3,5,6,7,8,9,10,12,13,14,16,17,20,21,22,25,27,29,31};

uint8_t goal;

uint8_t goal_matrix[32][16] = {
  { 3, 5, 6, 7, 9,10,12,13,17,20,21,22,25,27,29,31},
  { 2, 6, 7, 8,10,12,13,14,16,20,21,22,25,27,29,31},
  { 1, 5, 7, 8, 9,13,14,16,17,20,21,22,25,27,29,31},
  { 0, 5, 6, 8,10,12,13,14,16,17,20,22,25,27,29,31},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 2, 3, 6, 9,10,12,14,16,17,20,22,25,27,29,31},
  { 0, 1, 3, 8, 9,10,12,13,16,17,20,21,25,27,29,31},
  { 0, 1, 2, 8, 9,10,12,13,14,16,17,20,21,22,25,29},
  { 1, 2, 3, 5, 6,13,14,16,17,20,21,22,25,27,29,31},
  { 0, 2, 3, 5, 6, 7,10,12,14,16,17,20,21,22,27,29},
  { 0, 1, 3, 5, 6, 7, 9,12,13,16,17,20,21,27,29,31},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 5, 7, 9,10,16,17,20,21,22,25,27,31},
  { 1, 2, 3, 6, 7, 8,10,14,16,17,20,21,22,25,27,31},
  { 0, 1, 2, 3, 5, 7, 8, 9,13,16,17,20,21,22,25,27},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 1, 2, 3, 5, 6, 7, 9,10,12,13,14,22,25,27,29,31},
  { 0, 2, 3, 5, 6, 8, 9,10,12,13,14,20,22,27,29,31},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 5, 6, 8, 9,10,12,13,17,25,27,29,31},
  { 0, 1, 2, 3, 6, 7, 8, 9,12,13,14,16,22,25,27,31},
  { 0, 1, 3, 5, 7, 8,10,12,13,14,17,21,25,27,29,31},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 5, 6, 7, 8,10,12,13,16,20,21,22,31},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 6, 7, 8, 9,10,12,13,16,20,21,22,29},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 5, 6, 7, 8, 9,10,12,14,16,17,22,27},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 2, 3, 5, 6, 7, 8, 9,10,13,14,16,20,21,25}};

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  
  while(!module.getConfig()){
    module.interpretData();
  }

  for(int i = 0; i < 10; i++){
    bad[i] = (~bad[i]) & 0x1F;
  }
  for(int i = 0; i < 22; i++){
    good[i] = (~good[i]) & 0x1F;
  }

  randomSeed(analogRead(A5));
  int test = random(0,22);
  goal = good[test];
  Serial.println(test);
  Serial.println(goal);

  for(int i = 0; i < 5; i++) {
    pinMode(switches[i], INPUT_PULLUP);
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], (goal >> i) & 1);
  }

  module.sendReady();
}

void loop() {
  module.interpretData();
  int bad_counter = 0;
  int loop_broke = 0;

  // Most of the "bad_counter" stuff is just debouncing logic.
  delay(10);
  if(!module.is_solved){
    last_switch_state = switch_state;
    switch_state = 0;
    for(int i = 0; i < 5; i++) {
      switch_state |= (digitalRead(switches[i]) << i);
    }
    if(switch_state != last_switch_state) {
      for(int i = 0; i < 10; i++){
        if(switch_state == bad[i]){
          loop_broke = 1;
          bad_counter += 1;
          if(bad_counter >= 3){
            module.strike();
          }
          break;
        }
      }
      if(!loop_broke){
        bad_counter = 0;
      }
      if(switch_state == goal){
        module.win();
      }
    }
  }
}