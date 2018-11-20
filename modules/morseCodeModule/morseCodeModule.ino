#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include "morse.h"

#define BUTTON_L_PIN 5
#define BUTTON_R_PIN 6
#define BUTTON_TX_PIN 9
#define MORSE_LED_PIN 10

#define LOAD_PIN 14
#define DATA_IN_PIN 15
#define CLOCK_PIN 16
#define DISP_SINGLE(x,y) maxSingle((x), (y), LOAD_PIN, CLOCK_PIN, DATA_IN_PIN)

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

int constants[22] = {
  0b11110110, // 0
  0b11000000, // 1
  0b01010111, // 2
  0b11000111, // 3
  0b11100001, // 4
  0b11100001, // 5
  0b11100001, // 6
  0b11100001, // 7
  0b11100001, // 8
  0b11100001, // 9
};

int goal_freq;
int selected_freq = 0;
uint8_t morse_bits[8];
int morse_index;
int morse_length;

unsigned long last_char_time = 0;

unsigned long last_button_time = 0;

void setMorseBit(uint8_t *bits, int index, int val) {
  uint8_t and_mask, or_mask;
  and_mask = ~(1 << (index % 8));
  or_mask = ((!!val) << (index % 8));
  bits[index/8] = (bits[index/8] & and_mask) | or_mask;
}

int getMorseBit(uint8_t *bits, int index) {
  uint8_t mask;
  mask = (1 << (index % 8));
  return bits[index/8] & mask;
}

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  
  // while(!module.getConfig()){
  //   module.interpretData();
  // }
  randomSeed(analogRead(A5));
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_IN_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(BUTTON_L_PIN, INPUT);
  pinMode(BUTTON_R_PIN, INPUT);
  pinMode(BUTTON_TX_PIN, INPUT);
  pinMode(MORSE_LED_PIN, OUTPUT);

  morse_index = 0;
  goal_freq = random(0, 16);
  for(unsigned int i = 0; i < strlen(words[goal_freq]); i++) {
    char *morse_desc = morse[words[goal_freq][i] - 'a'];
    for(unsigned int j = 0; j < strlen(morse_desc); j++) {
      if(morse_desc[j] == '.') {
        setMorseBit(morse_bits, morse_index++, 1);
        setMorseBit(morse_bits, morse_index++, 0);
      } else if(morse_desc[j] == '-') {
        setMorseBit(morse_bits, morse_index++, 1);
        setMorseBit(morse_bits, morse_index++, 1);
        setMorseBit(morse_bits, morse_index++, 1);
        setMorseBit(morse_bits, morse_index++, 0);
      }
    }
    setMorseBit(morse_bits, morse_index++, 0);
    setMorseBit(morse_bits, morse_index++, 0);
  }
  setMorseBit(morse_bits, morse_index++, 0);
  setMorseBit(morse_bits, morse_index++, 0);
  setMorseBit(morse_bits, morse_index++, 0);
  setMorseBit(morse_bits, morse_index++, 0);
  morse_length = morse_index;
  morse_index = 0;

  // module.sendReady();
}

void doMorse() {
  Serial.println("morse");
  if(millis() - last_char_time >= DOT_TIME){
    last_char_time = millis();
    digitalWrite(MORSE_LED_PIN, getMorseBit(morse_bits, morse_index));
    morse_index = (morse_index + 1) % morse_length;
  }
}

void loop() {
  // module.interpretData();
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

    DISP_SINGLE(0, constants[3]);
    DISP_SINGLE(1, constants[freqs[selected_freq][0] - '0']);
    DISP_SINGLE(2, constants[freqs[selected_freq][1] - '0']);
    DISP_SINGLE(3, constants[freqs[selected_freq][2] - '0']);

    if(digitalRead(BUTTON_TX_PIN)) {
      if(selected_freq == goal_freq) {
        module.win();
      } else {
        digitalWrite(MORSE_LED_PIN, LOW);
        module.strike();
      }
    }
  }
}