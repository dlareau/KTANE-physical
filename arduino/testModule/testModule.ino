#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  serial_port.begin(19200);
  Serial.begin(19200);

  while(!module.getConfig()){
    module.interpretData();
  }
  //randomizeModule();
  module.sendReady();
}

void loop() {
  module.interpretData();
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delayWithUpdates(module, 1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delayWithUpdates(module, 1000);                       // wait for a second
  // TEMP


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