#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

#define MY_ADDRESS 2

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client);

void setup() {
  serial_port.begin(9600);
  while(!module.getConfig()){
    module.interpretData();
  }
}

void loop() {
  int is_solved = 0;
  module.interpretData();
  /*
  if(!is_solved){
    checkInputs();
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