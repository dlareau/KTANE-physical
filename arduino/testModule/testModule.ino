#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define MY_ADDRESS 2

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client);

int button_state = 0;
int last_button_state = 0;

void setup() {
  serial_port.begin(9600);
  Serial.begin(9600);

  // BEGIN TEMP COMMANDS
  pinMode(2, INPUT);
  // END TEMP COMMANDS

  Serial.print("B");
  while(!module.getConfig()){
    module.interpretData();
  }
  Serial.print("A");
  //randomizeModule();
  module.sendReady();
  Serial.print("S");
}

void loop() {
  module.interpretData();

  // TEMP
  button_state = digitalRead(2);
  if (button_state != last_button_state && button_state) {
    last_button_state = button_state;
    module.sendStrike();
    Serial.print("E");
    delay(5);
  }

  /*
  if(!module.is_solved){
    checkInputs();
    they_solved_it = checkIfSolved();
    updateOutputs();
    if(they_solved_it) {
      module.sendSolve();
      turnSolvedLightGreen();
    }
    if(they_messed_up) {
      module.sendStrike();
      flashSolvedLightRed();
    }
  }
  */
}