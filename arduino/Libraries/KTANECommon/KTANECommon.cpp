/** @file KTANECommon.cpp
 *  @brief Headers and definitions for common KTANE functionality
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "KTANECommon.h"
#include <string.h>

void config_to_raw(raw_config_t *raw_config, config_t *config_t){
  raw_config.ports = config.ports;
  raw_config.batteries = config.batteries;
  raw_config.indicators = config.indicators;
  memcpy(raw_config.serial, config.serial, 5);
  raw_config.serial6 = config.serial[5] - '0';
  raw_config.spacer1 = 1;
  raw_config.spacer2 = 1;
}

void raw_to_config(raw_config_t *raw_config, config_t *config_t){
  config.ports = raw_config.ports;
  config.batteries = raw_config.batteries;
  config.indicators = raw_config.indicators;
  memcpy(config.serial, raw_config.serial, 5);
  config.serial[5] = raw_config.s5 + '0';
  config.serial[6] = '\0';
}

KTANEModule::KTANEModule(DSerialClient &dserial):_dserial(dserial) {
  memset(_config, 0, sizeof(config_t));
}

void KTANEModule::ModuleInterpretData(){

}

int KTANEModule::sendStrike(){
  char str[2] = {(char)STRIKE, '\0'};
  return _dserial.sendData(str);
}

int KTANEModule::sendSolve(){
  char str[2] = {(char)SOLVE, '\0'};
  return _dserial.sendData(str);
}

config_t *KTANEModule::getConfig(){
  return &_config;
}

int KTANEModule::getLitFRK(){
  return _config.indicators & 1;
}

int KTANEModule::getLitCAR(){
  return _config.indicators & 2;
}

int KTANEModule::getNumBatteries(){
  return _config.batteries;
}

int KTANEModule::getParallelPort(){
  return _config.ports & 1;
}

int KTANEModule::getRCAPort(){
  return _config.ports & 2;
}

int KTANEModule::getRJ45Port(){
  return _config.ports & 4;
}

char KTANEModule::getSerialDigit(int index){
  if(index < 0 or index > 5){
    return (char)0;
  }
  return _config.serial[index];
}

int KTANEModule::serialContains(char c){
  return !!((int)strchr(_config.serial, c));
}


KTANEController::KTANEController(DSerialMaster &dserial):_dserial(dserial) {

}

void KTANEModule::ControllerInterpretData(){

}

int KTANEModule::sendConfig(config_t *config){
  for(int i = 0; i < )
  return _dserial.sendData()
}

int KTANEModule::getStrikes(){

}

int KTANEModule::getSolves(){

}
