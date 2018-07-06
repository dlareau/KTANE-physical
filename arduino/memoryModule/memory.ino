#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define DATA_IN_PIN 13
#define LOAD_PIN 12
#define CLOCK_PIN 14
#define DISP_SINGLE(x,y) maxSingle((x), (y), LOAD_PIN, CLOCK_PIN, DATA_IN_PIN)

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

uint8_t bottom_nums[5][4];
uint8_t top_nums[5];
uint8_t buttons_to_press[5];
int stage = 0;

int constants[22] = {
  0b11110110, // 0
  0b11000000, // 1
  0b01010111, // 2
  0b11000111, // 3
  0b11100001, // 4
};

uint8_t getIndexFromNumber(uint8_t *buttons, uint8_t num){
  for(int i = 0; i < 4; i++) {
    if(buttons[i] == num) {
      return i;
    }
  }
  return 255;
}

void updateDisplays() {
  DISP_SINGLE(0, constants[bottom_nums[stage][0]]);
  DISP_SINGLE(1, constants[bottom_nums[stage][1]]);
  DISP_SINGLE(2, constants[bottom_nums[stage][2]]);
  DISP_SINGLE(3, constants[bottom_nums[stage][3]]);
  DISP_SINGLE(4, constants[top_nums[stage]]);
}

void displayWaitingScreen() {

  // needs to delay for a total of 2500
  for(int i = 0; i < 5; i++) {
    DISP_SINGLE(i, 0);
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 15; i++){
    for(int j = 0; j < 5; j++) {
      DISP_SINGLE(i, 1 << ((i+j)%7));
    }
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 5; i++) {
    DISP_SINGLE(i, 0);
  }
  delayWithUpdates(module, 500);
}

void generateRandomNumbers() {
  int r1, r2;
  uint8_t temp;
  for(int i = 0; i < 5; i++){
    bottom_nums[i][0] = 1;
    bottom_nums[i][1] = 2;
    bottom_nums[i][2] = 3;
    bottom_nums[i][3] = 4;

    for(int j = 0; j < 20; j++){
      r1 = random(0, 4);
      r2 = random(0, 4);
      temp = bottom_nums[i][r1];
      bottom_nums[i][r1] = bottom_nums[i][r2];
      bottom_nums[i][r2] = temp;
    }

    top_nums[i] = random(1,5);
  }

  switch(top_nums[0]) {
    case(1):
      buttons_to_press[0] = 1; // Second Position
      break;
    case(2):
      buttons_to_press[0] = 1; // Second Position
      break;
    case(3):
      buttons_to_press[0] = 2; // Third Position
      break;
    case(4):
      buttons_to_press[0] = 3; // Fourth Position
      break;
  }
  switch(top_nums[1]) {
    case(1):
      // Button labeled 4
      buttons_to_press[1] = getIndexFromNumber(bottom_nums[1], 4);
      break;
    case(2):
      buttons_to_press[1] = buttons_to_press[0]; // Same position as stage 1
      break;
    case(3):
      buttons_to_press[1] = 0; // First Position
      break;
    case(4):
      buttons_to_press[1] = buttons_to_press[0]; // Same position as stage 1
      break;
  }
  switch(top_nums[2]) {
    case(1):
      // Same label as stage 2
      buttons_to_press[2] = getIndexFromNumber(bottom_nums[2], bottom_nums[1][buttons_to_press[1]]);
      break;
    case(2):
      // Same label as stage 1
      buttons_to_press[2] = getIndexFromNumber(bottom_nums[2], bottom_nums[0][buttons_to_press[0]]);
      break;
    case(3):
      buttons_to_press[2] = 2;  // Third Position
      break;
    case(4):
      // Button labeled 4
      buttons_to_press[2] = getIndexFromNumber(bottom_nums[2], 4) ;
      break;
  }
  switch(top_nums[3]) {
    case(1):
      buttons_to_press[3] = buttons_to_press[0]; // Same position as stage 1
      break;
    case(2):
      buttons_to_press[3] = 0;  // First Position
      break;
    case(3):
      buttons_to_press[3] = buttons_to_press[1]; // Same position as stage 2
      break;
    case(4):
      buttons_to_press[3] = buttons_to_press[1]; // Same position as stage 2
      break;
  }
  switch(top_nums[4]) {
    case(1):
      // Same label as stage 1
      buttons_to_press[4] = getIndexFromNumber(bottom_nums[4], bottom_nums[0][buttons_to_press[0]]);
      break;
    case(2):
      // Same label as stage 2
      buttons_to_press[4] = getIndexFromNumber(bottom_nums[4], bottom_nums[1][buttons_to_press[1]]);
      break;
    case(3):
      // Same label as stage 3
      buttons_to_press[4] = getIndexFromNumber(bottom_nums[4], bottom_nums[2][buttons_to_press[2]]);
      break;
    case(4):
      // Same label as stage 4
      buttons_to_press[4] = getIndexFromNumber(bottom_nums[4], bottom_nums[3][buttons_to_press[3]]);
      break;
  }
}

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  pinMode(DATA_IN_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);

  randomSeed(analogRead(0));

  // Generate numbers
  generateRandomNumbers();

  while(!module.getConfig()){
    module.interpretData();
  }
  module.sendReady();
}

void loop() {
  int button_pressed = -1;

  module.interpretData();

  if(!module.is_solved) {
    if(digitalRead(9)) {
      button_pressed = 0;
    } else if(digitalRead(10)) {
      button_pressed = 1;
    } else if(digitalRead(11)) {
      button_pressed = 2;
    } else if(digitalRead(12)) {
      button_pressed = 3;
    }

    if(button_pressed != -1) {
      if(button_pressed == buttons_to_press[stage]) {
        stage++;
      } else {
        stage = 0;
        generateRandomNumbers();
        module.strike();
      }
      if(stage == 5){
        module.win();
      } else {
      displayWaitingScreen();
      updateDisplays();
      }
    }
  }

}