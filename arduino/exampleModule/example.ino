#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client);

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

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);

  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  while(!module.getConfig()){
    module.interpretData();
  }

  /*
    Do setup here
  */

  module.sendReady();
  digitalWrite(3, HIGH);
  delayWithUpdates(module, 1000);
  digitalWrite(3, LOW);
}

void loop() {
  module.interpretData();

  if(!module.is_solved){
    /*
    checkInputs();
    if(they_solved_it) {
      module.sendSolve();
      youWin();
    }
    if(they_messed_up) {
      module.sendStrike();
      youLose();
    }
    updateOutputs();
    */
  }
}