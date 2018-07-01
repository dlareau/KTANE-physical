#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include <string.h>

NeoICSerial serial_port;
DSerialMaster master(serial_port);
KTANEController controller(master);

config_t config;

void setup() {
  serial_port.begin(9600);
  Serial.begin(9600);

  // BEGIN TEMP COMMANDS
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  // END TEMP COMMANDS

  // userSetClockTime();
  // indentifyExternalConfig();
  config.ports = 3;
  config.batteries = 1;
  config.indicators = 0;
  strncpy(config.serial, "KTANE1", 6);
  config.serial[6] = '\0';
  // displayControllableConfig();
  delay(1000);
  master.identifyClients();
  // controller.sendReset() followed by some doSerial?
  controller.sendConfig(&config);
  while(!controller.clientsAreReady()) {
    controller.interpretData();
  }
}

void loop() {
  //int strikes;
  controller.interpretData();

  // TEMPORARY
  if(controller.getStrikes()) {
    digitalWrite(2, HIGH);
  }

  // int time = updateTimer();
  // int strikes = controller.getStrikes();

  // updateStrikeLEDS(strikes);

  // if(time == 0 || strikes >= 3){
  //   youLose();
  // }

  // if(controller.getSolves() >= NUM_MODULES) {
  //   youWin();
  // }
}