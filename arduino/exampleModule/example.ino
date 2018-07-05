#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>

NeoICSerial serial_port;
DSerialClient client(serial_port, MY_ADDRESS);
KTANEModule module(client, 3, 4);

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  å
  while(!module.getConfig()){
    module.interpretData();
  }

  /*
    Do setup here
  */

  module.sendReady();
}

void loop() {
  module.interpretData();

  if(!module.is_solved){
    /*
    checkInputs();
    if(they_solved_it) {
      module.win();
    }
    if(they_messed_up) {
      module.strike();
    }
    updateOutputs();
    */
  }
}