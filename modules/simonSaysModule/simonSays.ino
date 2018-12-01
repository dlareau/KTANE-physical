#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

#define MAX_NUM_STAGES 5
#define RED 1
#define YELLOW 2
#define GREEN 3
#define BLUE 4
#define SPEAKER_PIN 2

int led_pins[4] = {15, 10, 11, 5};
int button_pins[4] = {14, 7, 12, 6};

unsigned long last_button_action = 0;
int button_state = 0;
int old_button_state = 0;
int button_stage = 0;
int stage;
int num_stages;
int stage_colors[MAX_NUM_STAGES];
int mapping[2][3][4] = {
  { // No Vowel
    {BLUE, RED, GREEN, YELLOW}, // No Strikes
    {RED, GREEN, YELLOW, BLUE}, // One Strike
    {YELLOW, RED, BLUE, GREEN}, // Two Strikes
  },
  { // Vowel
    {BLUE, GREEN, YELLOW, RED}, // No Strikes
    {YELLOW, RED, BLUE, GREEN}, // One Strike
    {GREEN, BLUE, YELLOW, RED}, // Two Strikes
  },
};

void update_lights(){
  static unsigned long old_millis = 0;
  static int light_stage = 0;

  if(light_stage >= ((stage + 1)*2)) {
    if(millis() - old_millis > 700) {
      old_millis = millis();
      light_stage = 0;
    }
  } else {
    if((light_stage % 2 == 0) && (millis() - old_millis > 300)) {
      digitalWrite(led_pins[stage_colors[light_stage/2]], HIGH);
      old_millis = millis();
      light_stage++;
    } else if((light_stage % 2 == 1) && (millis() - old_millis > 700)) {
      digitalWrite(led_pins[stage_colors[light_stage/2]], LOW);
      old_millis = millis();
      light_stage++;
    }
  }
}

int get_button(){
  int button_pressed = 0;
  if(digitalRead(button_pins[0])) {
    button_pressed = RED;
  } else if(digitalRead(button_pins[1])) {
    button_pressed = YELLOW;
  } else if(digitalRead(button_pins[2])) {
    button_pressed = GREEN;
  } else if(digitalRead(button_pins[3])) {
    button_pressed = BLUE;
  }
  return button_pressed;
}

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  randomSeed(analogRead(A5));
  num_stages = random(3, MAX_NUM_STAGES + 1);

  pinMode(button_pins[0], INPUT);
  pinMode(button_pins[1], INPUT);
  pinMode(button_pins[2], INPUT);
  pinMode(button_pins[3], INPUT);
  pinMode(led_pins[0], OUTPUT);
  pinMode(led_pins[1], OUTPUT);
  pinMode(led_pins[2], OUTPUT);
  pinMode(led_pins[3], OUTPUT);
  digitalWrite(led_pins[0], LOW);
  digitalWrite(led_pins[1], LOW);
  digitalWrite(led_pins[2], LOW);
  digitalWrite(led_pins[3], LOW);

  while(!module.getConfig()){
    module.interpretData();
  }

  for(int i = 0; i < num_stages; i++) {
    stage_colors[i] = random(0, 4);
  }
  stage = 0;

  module.sendReady();
}

void loop() {
  module.interpretData();
  if(!module.is_solved){
    int vowel = module.serialContainsVowel();
    int strikes = module.getNumStrikes();
    update_lights();
    if(millis()-last_button_action > 10) {
      button_state = get_button();
      last_button_action = millis();
    }
    if(button_state != old_button_state) {
      old_button_state = button_state;

      if(button_state != 0){
        if(button_state == mapping[vowel][strikes][stage_colors[button_stage]]) {
          if(button_stage == stage) {
            stage++;
            button_stage = 0;
            // tone(SPEAKER_PIN, 140, 150);
            // delayWithUpdates(module, 200);
            // tone(SPEAKER_PIN, 340, 150);
            // delayWithUpdates(module, 150);
            // noTone(SPEAKER_PIN);
          } else {
            button_stage++;
          }
          if(stage == num_stages) {
            module.win();
          }
        } else {
          module.strike();
        }
      }
    }
  }
}