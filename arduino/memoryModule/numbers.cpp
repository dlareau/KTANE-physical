#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define DATA_IN_PIN 13
#define LOAD_PIN 12
#define CLOCK_PIN 14

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client);

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

void putByte(byte data) {
  byte i = 8;
  byte mask;
  while(i > 0) {
    mask = 0x01 << (i - 1);      // get bitmask
    digitalWrite( CLOCK_PIN, LOW);   // tick
    if (data & mask){            // choose bit
      digitalWrite(DATA_IN_PIN, HIGH);// send 1
    }else{
      digitalWrite(DATA_IN_PIN, LOW); // send 0
    }
    digitalWrite(CLOCK_PIN, HIGH);   // tock
    --i;                         // move to lesser bit
  }
}

void maxSingle(byte reg, byte col) {
  //maxSingle is the "easy"  function to use for a single max7219
  digitalWrite(LOAD_PIN, LOW);       // begin
  putByte(reg);                  // specify register
  putByte(col);                  // put data
  digitalWrite(LOAD_PIN, LOW);       // and load da stuff
  digitalWrite(LOAD_PIN,HIGH);
}

uint8_t getIndexFromNumber(uint8_t *buttons, uint8_t num){
  for(int i = 0; i < 4; i++) {
    if(buttons[i] == num) {
      return i;
    }
  }
  return 255;
}

void updateDisplays() {
  maxSingle(0, constants[bottom_nums[stage][0]]);
  maxSingle(1, constants[bottom_nums[stage][1]]);
  maxSingle(2, constants[bottom_nums[stage][2]]);
  maxSingle(3, constants[bottom_nums[stage][3]]);
  maxSingle(4, constants[top_nums[stage]]);
}

void youWin() {
  module.sendSolve();
  digitalWrite(3, HIGH);
}

void youLose() {
  module.sendStrike();
  digitalWrite(4, HIGH);
  delayWithUpdates(module, 500);
  digitalWrite(4, LOW);
}

void displayWaitingScreen() {

  // needs to delay for a total of 2500
  for(int i = 0; i < 5; i++) {
    maxSingle(i, 0);
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 15; i++){
    for(int j = 0; j < 5; j++) {
      maxSingle(i, 1 << ((i+j)%7));
    }
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 5; i++) {
    maxSingle(i, 0);
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
  serial_port.begin(9600);
  Serial.begin(9600);
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
        youLose();
      }
      displayWaitingScreen();
      updateDisplays();
    }
  }

}