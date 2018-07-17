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

int led_pins[4] = {15, 10, 11, 5};
int button_pins[4] = {14, 7, 12, 6};

int stage;
int num_stages = random(3, MAX_NUM_STAGES + 1);
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

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  randomSeed(analogRead(A5));

  pinMode(button_pins[0], INPUT);
  pinMode(button_pins[1], INPUT);
  pinMode(button_pins[2], INPUT);
  pinMode(button_pins[3], INPUT);
  pinMode(led_pins[0], OUTPUT);
  pinMode(led_pins[1], OUTPUT);
  pinMode(led_pins[2], OUTPUT);
  pinMode(led_pins[3], OUTPUT);

  // while(!module.getConfig()){
  //   module.interpretData();
  // }

  for(int i = 0; i < num_stages; i++) {
    stage_colors[i] = random(0, 4);
  }
  stage = 0;

  // module.sendReady();
}

void loop() {
  // module.interpretData();
  int button_pressed = 0;

  if(!module.is_solved){
    if(digitalRead(button_pins[0])) {
      button_pressed = RED;
    } else if(digitalRead(button_pins[1])) {
      button_pressed = YELLOW;
    } else if(digitalRead(button_pins[2])) {
      button_pressed = GREEN;
    } else if(digitalRead(button_pins[3])) {
      button_pressed = BLUE;
    }


    int vowel = 0;//module.serialContainsVowel();
    int strikes = 0;// module.getNumStrikes();
    int light_color = stage_colors[stage];
    update_lights();
    // I don't check button sequences, only last button, need to check all.
    if(button_pressed == mapping[vowel][strikes][light_color]) {
      stage++;
      if(stage == num_stages){
        module.win();
      }
    } else if(button_pressed != 0) {
      module.strike();
    }
  }
}