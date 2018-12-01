#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

// Resistor values = 33, 330, 1000, 3300, 22000
// Wire colors  = White, Blue, Yellow, Black, Red
// Wire int     =    1    2     3      4     5
// 0 indicates no wire
#define WHITE 1
#define BLUE 2
#define YELLOW 3
#define RED 4
#define BLACK 5

int wires[6] = {0,0,0,0,0,0};
int color_count[6] = {0,0,0,0,0,0};
int wire_to_cut; // One indexed and relative
int cut_index; // wire_to_cut but zero indexed and absolute
int num_wires = 0;

int voltageToWire(int voltage) {
  if(voltage < 10) {          return 0;
  } else if(voltage < 138) {  return 1;
  } else if(voltage < 384) {  return 2;
  } else if(voltage < 640) {  return 3;
  } else if(voltage < 896) {  return 4;
  } else {                    return 5;
  }
  return 0;
}

int lastWireIndex() {
  for(int i = 5; i >= 0; i++) {
    if(wires[i] != 0){
      return i;
    }
  }
  return 0;
}

int firstWireIndex() {
  for(int i = 0; i < 6; i++) {
    if(wires[i] != 0){
      return i;
    }
  }
  return 0;
}

int relLastColorIndex(int color) {
  int index = 0;
  int retIndex = 0;
  for(int i = 0; i < 6; i++) {
    if(wires[i] != 0){
      if(wires[i] == color){
        retIndex = index;
      }
      index++;
    }
  }
  return retIndex;
}

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  // Detect wires:
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  for(int i = 0; i < 6; i++) {
    wires[i] = voltageToWire(analogRead(i));
    color_count[wires[i]] = color_count[wires[i]] + 1;
    if(wires[i] != 0) {
      num_wires++;
    }
    delay(10);
  }


  while(!module.getConfig()){
    module.interpretData();
  }

  // Detect Solution:
  switch(num_wires) {

    case(3):
      if(color_count[RED] == 0){
        wire_to_cut = 2; // Second wire
      } else if(wires[lastWireIndex()] == WHITE) {
        wire_to_cut = num_wires; // Last wire
      } else if(color_count[BLUE] > 1) {
        wire_to_cut = relLastColorIndex(BLUE) + 1; // Last blue wire
      } else {
        wire_to_cut = num_wires; // Last wire
      }
      break;

    case(4):
      if(color_count[RED] > 1 && IS_ODD(module.getSerialDigit(5))){
        wire_to_cut = relLastColorIndex(RED) + 1; // Last red wire
      } else if(wires[lastWireIndex()] == YELLOW && color_count[RED] == 0) {
        wire_to_cut = 1; // First wire
      } else if(color_count[BLUE] == 1) {
        wire_to_cut = 1; // First wire
      } else if(color_count[YELLOW] <= 1) {
        wire_to_cut = num_wires; // Last wire
      } else {
        wire_to_cut = 2; // Second wire
      }
      break;

    case(5):
      if(wires[lastWireIndex()] == BLACK && IS_ODD(module.getSerialDigit(5))) {
        wire_to_cut = 4; // Fourth wire
      } else if(color_count[RED] == 1 && color_count[YELLOW] > 1) {
        wire_to_cut = 1; // First wire
      } else if(color_count[BLACK] == 0) {
        wire_to_cut = 2; // Second wire
      } else {
        wire_to_cut = 1; // First wire
      }
      break;

    case(6):
      if(color_count[YELLOW] == 0 && IS_ODD(module.getSerialDigit(5))) {
        wire_to_cut = 3; // Third wire
      } else if(color_count[YELLOW] == 1 && color_count[WHITE] > 1) {
        wire_to_cut = 4; // Fourth wire
      } else if(color_count[RED] == 0) {
        wire_to_cut = num_wires; // Last wire
      } else {
        wire_to_cut = 4; // Fourth wire
      }
      break;

    default:
      module.strike();
  }

  int temp = wire_to_cut;
  for(int i = 0; i < 6; i++) {
    if(wires[i] != 0){
      temp--;
      if(temp == 0){
        cut_index = i;
        break;
      }
    }
  }

  module.sendReady();
}

void loop() {
  module.interpretData();


  if(!module.is_solved){
    for(int i = 0; i < 6; i++) {
      if(wires[i] != 0){
        if(wires[i] != voltageToWire(analogRead(i))) {
          delayWithUpdates(module, 100);
          if(wires[i] != voltageToWire(analogRead(i))) { // Check again for debouncing reasons
            if(i == cut_index) {
              module.win();
            } else {
              module.strike();
              wires[i] = voltageToWire(analogRead(i));
            }
          }
        }
        delayWithUpdates(module, 10);
      }
    }
  }
}