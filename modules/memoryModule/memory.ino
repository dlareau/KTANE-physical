#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define DATA_IN_PIN 11
#define LOAD_PIN 12
#define CLOCK_PIN 13
#define DISP_SINGLE(x,y) maxSingle((x), (y), LOAD_PIN, CLOCK_PIN, DATA_IN_PIN)

#define BUTTON1_PIN 14
#define BUTTON2_PIN 15
#define BUTTON3_PIN 16
#define BUTTON4_PIN 17

#define LED1_PIN 19
#define LED2_PIN 18
#define LED3_PIN 7
#define LED4_PIN 6
#define LED5_PIN 5

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

uint8_t bottom_nums[5][4];
uint8_t top_nums[5];
uint8_t buttons_to_press[5];
int stage = 0;

// Has 6 elements to stop overflow when you win
int led_pins[6] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN, LED5_PIN, LED5_PIN};

byte max7219_reg_decodeMode  = 0x09;
byte max7219_reg_intensity   = 0x0a;
byte max7219_reg_scanLimit   = 0x0b;
byte max7219_reg_shutdown    = 0x0c;
byte max7219_reg_displayTest = 0x0f;

int constants[5] = {
  0b10111110, // 0
  0b00010010, // 1
  0b11011100, // 2
  0b11011010, // 3
  0b01110010 // 4
};

int digits[5] = {3, 8, 6, 5, 4};

uint8_t getIndexFromNumber(uint8_t *buttons, uint8_t num){
  for(int i = 0; i < 4; i++) {
    if(buttons[i] == num) {
      return i;
    }
  }
  return 255;
}

void updateDisplays() {
  DISP_SINGLE(5, constants[bottom_nums[stage][0]]);
  DISP_SINGLE(6, constants[bottom_nums[stage][1]]);
  DISP_SINGLE(8, constants[bottom_nums[stage][2]]);
  DISP_SINGLE(3, constants[bottom_nums[stage][3]]);
  DISP_SINGLE(4, constants[top_nums[stage]]);
  for(int i = 0; i < 5; i++) {
    digitalWrite(led_pins[i], LOW);
  }
  for(int i = 0; i <= stage; i++) {
    digitalWrite(led_pins[i], HIGH);
  }
}

void displayWaitingScreen() {

  // needs to delay for a total of 2500
  for(int i = 0; i < 5; i++) {
    DISP_SINGLE(digits[i], 0);
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 15; i++){
    for(int j = 0; j < 5; j++) {
      DISP_SINGLE(digits[j], 1 << ((i+j)%7));
    }
    delayWithUpdates(module, 100);
  }
  for(int i = 0; i < 5; i++) {
    DISP_SINGLE(digits[i], 0);
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

  // for(int i = 0; i < 5; i++) {
  //   Serial.println("");
  //   Serial.println("");
  //   Serial.println(top_nums[i]);
  //   Serial.println(buttons_to_press[i]);
  //   for(int j = 0; j < 4; j++){
  //     Serial.print(bottom_nums[i][j]);
  //   }
  // }
}

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  pinMode(DATA_IN_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUTTON3_PIN, INPUT);
  pinMode(BUTTON4_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  pinMode(LED5_PIN, OUTPUT);

  DISP_SINGLE(max7219_reg_scanLimit, 0x07);
  DISP_SINGLE(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)
  DISP_SINGLE(max7219_reg_shutdown, 0x01);    // not in shutdown mode
  DISP_SINGLE(max7219_reg_displayTest, 0x00); // no display test
   for (int e=1; e<=8; e++) {    // empty registers, turn all LEDs off
    DISP_SINGLE(e,0);
  }
  DISP_SINGLE(max7219_reg_intensity, 0x0f & 0x0f); // the first 0x0f is the value you can set

  randomSeed(analogRead(0));

  // Generate numbers
  generateRandomNumbers();
  updateDisplays();

  while(!module.getConfig()){
    module.interpretData();
  }
  module.sendReady();
}

void loop() {
  int button_pressed = -1;

  module.interpretData();

  if(!module.is_solved) {
    if(digitalRead(BUTTON1_PIN)) {
      button_pressed = 0;
    } else if(digitalRead(BUTTON2_PIN)) {
      button_pressed = 1;
    } else if(digitalRead(BUTTON3_PIN)) {
      button_pressed = 2;
    } else if(digitalRead(BUTTON4_PIN)) {
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