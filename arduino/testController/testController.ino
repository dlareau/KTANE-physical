#include "DSerial.h"
#include "KTANECommon.h"
#include <NeoICSerial.h>
#include <string.h>

NeoICSerial serial_port;
DSerialMaster master(serial_port);
KTANEController controller(master);

config_t config;
int strikes = 0;
int solves = 0;

void setup() {
  serial_port.begin(19200);
  Serial.begin(19200);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

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
  controller.interpretData();

  // int time = updateTimer();
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

  // updateStrikeLEDS(strikes);

  // if(time == 0 || strikes >= 3){
  //   youLose();
  // }

  // if(controller.getSolves() >= NUM_MODULES) {
  //   youWin();
  // }
}