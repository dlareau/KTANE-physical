/** @file KTANECommon.cpp
 *  @brief Headers and definitions for common KTANE functionality
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "KTANECommon.h"
#include <string.h>

void config_to_raw(config_t *config_t, raw_config_t *raw_config) {
  raw_config.ports = config.ports;
  raw_config.batteries = config.batteries;
  raw_config.indicators = config.indicators;
  memcpy(raw_config.serial, config.serial, 5);
  raw_config.serial6 = config.serial[5] - '0';
  raw_config.spacer1 = 1;
  raw_config.spacer2 = 1;
}

void raw_to_config(raw_config_t *raw_config, config_t *config_t) {
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

void KTANEModule::interpretData(){
  char out_message[MAX_MSG_LEN];
  if(_dserial.getData(out_message)) {
    if(out_message[0] == CONFIG && strlen(out_message) == 8) {
      raw_to_config((raw_config_t)(out_message + 1), &_config);
    }
  }
}

int KTANEModule::sendStrike() {
  char str[2] = {STRIKE, '\0'};
  return _dserial.sendData(str);
}

int KTANEModule::sendSolve() {
  char str[2] = {SOLVE, '\0'};
  return _dserial.sendData(str);
}

config_t *KTANEModule::getConfig() {
  return &_config;
}

int KTANEModule::getLitFRK() {
  return _config.indicators & 1;
}

int KTANEModule::getLitCAR() {
  return _config.indicators & 2;
}

int KTANEModule::getNumBatteries() {
  return _config.batteries;
}

int KTANEModule::getParallelPort() {
  return _config.ports & 1;
}

int KTANEModule::getRCAPort() {
  return _config.ports & 2;
}

int KTANEModule::getRJ45Port() {
  return _config.ports & 4;
}

char KTANEModule::getSerialDigit(int index) {
  if(index < 0 or index > 5){
    return (char)0;
  }
  return _config.serial[index];
}

int KTANEModule::serialContains(char c) {
  return !!((int)strchr(_config.serial, c));
}


KTANEController::KTANEController(DSerialMaster &dserial):_dserial(dserial) {
  memset(_strikes, 0, MAX_CLIENTS);
  memset(_solves, 0, MAX_CLIENTS);
}

void KTANEModule::interpretData() {
  char out_message[MAX_MSG_LEN];
  int client_id = _dserial.getData(out_message);
  if(client_id) {
    if(out_message[0] == STRIKE) {
      _strikes[client_id] = 1;
    } else if(out_message[0] == SOLVE) {
      _solves[client_id] = 1;
    }
  }
}

int KTANEModule::sendConfig(config_t *config) {
  char msg[9];
  int err = 0;
  uint8_t clients[MAX_CLIENTS];
  int num_clients = 0;
  num_clients = _dserial.getClients(clients);

  msg[0] = CONFIG;
  config_to_raw(config, msg+1);
  msg[8] = '\0';

  for(int i = 0; i < num_clients; i++) {
    if(!_dserial.sendData(clients[i], msg)) {
      err++;
    }
  }
  return (err == 0);
}

int KTANEModule::getStrikes() {
  int num_strikes = 0;
  for(int i = 0; i < MAX_CLIENTS; i++) {
    num_strikes += _strikes[i];
  }
  return num_strikes;
}

int KTANEModule::getSolves() {
  int num_solves = 0;
  for(int i = 0; i < MAX_CLIENTS; i++) {
    num_solves += _solves[i];
  }
  return num_solves;
}
