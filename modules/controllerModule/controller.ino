#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// Defines
#define DIG4(x) (((x) & 0b11101110) | (((x) & 1) << 4) | (((x) & 0b10000) >> 4))
#define DOT(x) ((x) | 0b10000)
#define DATA_PIN 4
#define LOAD_PIN 2
#define CLOCK_PIN 3
#define STRIKE_1_PIN A0
#define STRIKE_2_PIN A1
#define STRIKE_3_PIN A2
#define SPEAKER_PIN 5
#define SOLVE_TIME (240000) //4*60*1000

// Constants
byte digits[12] = {
    0b11101110, // 0
    0b00000110, // 1
    0b10101011, // 2
    0b10100111, // 3
    0b01000111, // 4
    0b11100101, // 5
    0b11101101, // 6
    0b00100110, // 7
    0b11101111, // 8
    0b01100111, // 9
    0b01100111, // L
    0b01100111  // E
};

int brightness = 4;
int win_melody[] = {};
int win_melody_durations[] = {};
int win_melody_len = 5;
int lose_melody[] = {};
int lose_melody_durations[] = {};
int lose_melody_len = 4;


byte max7219_reg_decodeMode  = 0x09;
byte max7219_reg_intensity   = 0x0a;
byte max7219_reg_scanLimit   = 0x0b;
byte max7219_reg_shutdown    = 0x0c;
byte max7219_reg_displayTest = 0x0f;

// Objects
Adafruit_AlphaNum4 alpha1 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha2 = Adafruit_AlphaNum4();

config_t config;
NeoICSerial serial_port;
DSerialMaster master(serial_port);
KTANEController controller(master);

// Globals
int strikes = 0;
int solves = 0;
unsigned long dest_time;
int num_modules;

void playMelody(int *melody, int* durations, int melody_len) {
  for (int thisNote = 0; thisNote < melody_len; thisNote++) {

    int noteDuration = 1000 / durations[thisNote];
    tone(SPEAKER_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    int pauseBetweenNotes = noteDuration * 1.30;
    delayWithUpdates(controller, pauseBetweenNotes);

    // stop the tone playing:
    noTone(SPEAKER_PIN);
  }
}

void updateClock()

void youLose() {
  // Play lose music
  playMelody(lose_melody, lose_melody_durations, lose_melody_len);
  alpha1.clear();
  alpha2.clear();
  alpha1.writeDigitAscii(2, ' ');
  alpha1.writeDigitAscii(3, 'B');
  alpha2.writeDigitAscii(0, 'O');
  alpha2.writeDigitAscii(1, 'O');
  alpha2.writeDigitAscii(2, 'M');
  alpha2.writeDigitAscii(3, ' ');
  alpha1.writeDisplay();
  alpha2.writeDisplay();

  // Stop clock
  while(1){;}
}

void youWin() {
  // Play win music
  playMelody(win_melody, win_melody_durations, win_melody_len);

  // Stop clock
  while(1){;}
}

void setup() {
  // Serial setup
  serial_port.begin(19200);
  Serial.begin(19200);

  // LED/Speaker setup
  pinMode(STRIKE_1_PIN,  OUTPUT);
  pinMode(STRIKE_2_PIN,  OUTPUT);
  pinMode(STRIKE_3_PIN,  OUTPUT);
  pinMode(SPEAKER_PIN,   OUTPUT);

  // Config selection
  // Get all of this from ESP8266
  config.ports = 3;
  config.batteries = 1;
  config.indicators = 0;
  strncpy(config.serial, "KTANE1", 6);
  config.serial[6] = '\0';

  // Clock 7-segment setup
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN,  OUTPUT);
  pinMode(LOAD_PIN,   OUTPUT);

  maxSingle(max7219_reg_scanLimit, 0x07, LOAD_PIN, CLOCK_PIN, DATA_PIN);      
  maxSingle(max7219_reg_decodeMode, 0x00, LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(max7219_reg_shutdown, 0x01, LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(max7219_reg_displayTest, 0x00, LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(max7219_reg_intensity, 0x0f & 0x0f, LOAD_PIN, CLOCK_PIN, DATA_PIN);

  maxSingle(1, digits[0], LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(2, digits[3], LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(3, digits[1], LOAD_PIN, CLOCK_PIN, DATA_PIN);
  maxSingle(4, DIG4(digits[2]), LOAD_PIN, CLOCK_PIN, DATA_PIN);

  // Serial alphanumeric setup
  alpha1.begin(0x71);
  alpha2.begin(0x70);
  
  alpha1.clear();
  alpha2.clear();
  alpha1.setBrightness(brightness);
  alpha2.setBrightness(brightness);
  alpha1.writeDigitAscii(2, config.serial[0]);
  alpha1.writeDigitAscii(3, config.serial[1]);
  alpha2.writeDigitAscii(0, config.serial[2]);
  alpha2.writeDigitAscii(1, config.serial[3]);
  alpha2.writeDigitAscii(2, config.serial[4]);
  alpha2.writeDigitAscii(3, config.serial[5]);
  alpha1.writeDisplay();
  alpha2.writeDisplay();

  delay(1000);
  num_modules = master.identifyClients();

  // controller.sendReset() followed by some interpretData?
  controller.sendConfig(&config);
  while(!controller.clientsAreReady()) {
    controller.interpretData();
  }

  dest_time = millis() + SOLVE_TIME;
}

void loop() {
  controller.interpretData();

  if(millis() > dest_time) {
    youLose();
  } else {
    // Update clock
    unsigned long diff_time = dest_time - millis();
    int seconds = (diff_time / 1000)%60;
    int minutes = diff_time / 60000;
    maxSingle(1, digits[minutes/10], LOAD_PIN, CLOCK_PIN, DATA_PIN);
    maxSingle(3, DOT(digits[minutes%10]), LOAD_PIN, CLOCK_PIN, DATA_PIN);
    maxSingle(4, DIG4(DOT(digits[seconds/10])), LOAD_PIN, CLOCK_PIN, DATA_PIN);
    maxSingle(2, digits[seconds%10], LOAD_PIN, CLOCK_PIN, DATA_PIN);
  }

  if(strikes < controller.getStrikes()){
    tone(5, 340, 150);
    delayWithUpdates(controller, 200);
    tone(5, 140, 150);
    delayWithUpdates(controller, 150);
    noTone(5);
    strikes = controller.getStrikes();
  }

  if(solves < controller.getSolves()){
    tone(5, 140, 150);
    delayWithUpdates(controller, 200);
    tone(5, 340, 150);
    delayWithUpdates(controller, 150);
    noTone(5);
    solves = controller.getSolves();
  }

  digitalWrite(STRIKE_1_PIN, strikes >= 1);
  digitalWrite(STRIKE_2_PIN, strikes >= 2);
  digitalWrite(STRIKE_3_PIN, strikes >= 3);

  if(strikes >= 3){
    youLose();
  }

  if(controller.getSolves() >= num_modules) {
    youWin();
  }
}