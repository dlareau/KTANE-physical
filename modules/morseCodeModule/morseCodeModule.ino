#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include "morse.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#define BUTTON_R_PIN A3
#define BUTTON_L_PIN A2
#define BUTTON_TX_PIN A1
#define MORSE_LED_PIN 5

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);
Adafruit_7segment matrix = Adafruit_7segment();

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
  matrix.begin(0x70);
  
  while(!module.getConfig()){
    module.interpretData();
  }
  randomSeed(config_to_seed(module.getConfig()));
  pinMode(BUTTON_L_PIN, INPUT_PULLUP);
  pinMode(BUTTON_R_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TX_PIN, INPUT_PULLUP);
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

  module.sendReady();
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
  module.interpretData();
  doMorse();

  if(!module.is_solved){
    if(!digitalRead(BUTTON_L_PIN) && (millis() - last_button_time > 250)) {
      last_button_time = millis();
      selected_freq--;
      if(selected_freq < 0) {
        selected_freq = 0;
      }
    }
    if(!digitalRead(BUTTON_R_PIN) && (millis() - last_button_time > 250)) {
      last_button_time = millis();
      selected_freq++;
      if(selected_freq > 15){
        selected_freq = 15;
      }
    }


    matrix.writeDigitNum(0, 3);
    matrix.writeDigitNum(1, freqs[selected_freq][0] - '0');
    matrix.writeDigitNum(3, freqs[selected_freq][1] - '0');
    matrix.writeDigitNum(4, freqs[selected_freq][2] - '0');
    matrix.writeDisplay();

    if(!digitalRead(BUTTON_TX_PIN)) {
      if(selected_freq == goal_freq) {
        module.win();
      } else {
        digitalWrite(MORSE_LED_PIN, LOW);
        module.strike();
      }
    }
  }
}