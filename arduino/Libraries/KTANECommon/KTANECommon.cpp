/** @file KTANECommon.cpp
 *  @brief Headers and definitions for common KTANE functionality
 *
 *  Possibly restart microcontroller on reset?
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "KTANECommon.h"
#include <string.h>

void config_to_raw(config_t *config, raw_config_t *raw_config) {
  raw_config->ports = config->ports;
  raw_config->batteries = config->batteries;
  raw_config->indicators = config->indicators;
  memcpy(raw_config->serial, config->serial, 5);
  raw_config->serial6 = config->serial[5] - '0';
  raw_config->spacer1 = 1;
  raw_config->spacer2 = 1;
}

void raw_to_config(raw_config_t *raw_config, config_t *config) {
  config->ports = raw_config->ports;
  config->batteries = raw_config->batteries;
  config->indicators = raw_config->indicators;
  memcpy(config->serial, raw_config->serial, 5);
  config->serial[5] = raw_config->serial6 + '0';
  config->serial[6] = '\0';
}

void delayWithUpdates(KTANEModule &module, unsigned int length) {
  unsigned long start_millis = millis();
  while(millis() - start_millis < length){
    module.interpretData();
  }
}

void delayWithUpdates(KTANEController &controller, unsigned int length) {
  unsigned long start_millis = millis();
  while(millis() - start_millis < length){
    controller.interpretData();
  }
}

KTANEModule::KTANEModule(DSerialClient &dserial):_dserial(dserial) {
  memset(&_config, 0, sizeof(config_t));
  _got_config = 0;
  _num_strikes = 0;
  _got_reset = 0;
  is_solved = 0;
}

void KTANEModule::interpretData(){
  char out_message[MAX_MSG_LEN];
  _dserial.doSerial();
  if(_dserial.getData(out_message)) {
    if(out_message[0] == CONFIG && strlen(out_message) == 8) {
      _got_config = 1;
      raw_to_config((raw_config_t *)(out_message + 1), &_config);
    } else if(out_message[0] == RESET) {
      is_solved = 0;
      _num_strikes = 0;
      _got_config = 0;
      memset(&_config, 0, sizeof(config_t));
      _got_reset = 1;
    } else if(out_message[0] == NUM_STRIKES) {
      _num_strikes = out_message[1];
    }
  }
}

int KTANEModule::sendStrike() {
  char str[2] = {STRIKE, '\0'};
  return _dserial.sendData(str);
}

int KTANEModule::sendSolve() {
  char str[2] = {SOLVE, '\0'};
  is_solved = 1;
  return _dserial.sendData(str);
}

int KTANEModule::sendReady() {
  char str[2] = {READY, '\0'};
  return _dserial.sendData(str);
}

config_t *KTANEModule::getConfig() {
  if(_got_config){
    return &_config;
  } else {
    return NULL;
  }
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

int KTANEModule::getNumStrikes() {
  return _num_strikes;
}

int KTANEModule::getReset() {
  if(_got_reset){
    _got_reset = 0;
    return 1;
  }
  return 0;
}

KTANEController::KTANEController(DSerialMaster &dserial):_dserial(dserial) {
  memset(_strikes, 0, MAX_CLIENTS);
  memset(_solves, 0, MAX_CLIENTS);
  memset(_readies, 0, MAX_CLIENTS);
}

void KTANEController::interpretData() {
  char out_message[MAX_MSG_LEN];
  _dserial.doSerial();
  int client_id = _dserial.getData(out_message);
  if(client_id) {
    if(out_message[0] == STRIKE) {
      _strikes[client_id] = 1;
      sendStrikes();
    } else if(out_message[0] == SOLVE) {
      _solves[client_id] = 1;
    } else if(out_message[0] == READY) {
      _readies[client_id] = 1;
    }
  }
}

int KTANEController::sendConfig(config_t *config) {
  char msg[9];
  int err = 0;
  uint8_t clients[MAX_CLIENTS];
  int num_clients = 0;
  num_clients = _dserial.getClients(clients);

  msg[0] = CONFIG;
  config_to_raw(config, (raw_config_t *)(msg+1));
  msg[8] = '\0';

  for(int i = 0; i < num_clients; i++) {
    if(!_dserial.sendData(clients[i], msg)) {
      err++;
    }
    _dserial.doSerial();
  }
  return (err == 0);
}

int KTANEController::getStrikes() {
  int num_strikes = 0;
  for(int i = 0; i < MAX_CLIENTS; i++) {
    num_strikes += _strikes[i];
  }
  return num_strikes;
}

int KTANEController::getSolves() {
  int num_solves = 0;
  for(int i = 0; i < MAX_CLIENTS; i++) {
    num_solves += _solves[i];
  }
  return num_solves;
}

int KTANEController::clientsAreReady() {
  int num_readies = 0;
  for(int i = 0; i < MAX_CLIENTS; i++) {
    num_readies += _readies[i];
  }
  return num_readies >= _dserial.getClients(NULL);
}

int KTANEController::sendReset() {
  char msg[2] = {RESET, '\0'};
  int err = 0;
  uint8_t clients[MAX_CLIENTS];
  int num_clients = 0;
  num_clients = _dserial.getClients(clients);

  for(int i = 0; i < num_clients; i++) {
    if(!_dserial.sendData(clients[i], msg)) {
      err++;
    }
    _dserial.doSerial();
  }
  return (err == 0);
}

int KTANEController::sendStrikes() {
  int num_strikes = getStrikes();
  char msg[2] = {RESET, '\0'};
  int err = 0;
  uint8_t clients[MAX_CLIENTS];
  int num_clients = 0;

  if(num_strikes > 0){
    num_clients = _dserial.getClients(clients);

    for(int i = 0; i < num_clients; i++) {
      if(!_dserial.sendData(clients[i], msg)) {
        err++;
      }
      _dserial.doSerial();
    }
    return (err == 0);
  }
  return 0;
}

