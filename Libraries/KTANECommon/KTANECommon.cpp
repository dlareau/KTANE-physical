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

unsigned long config_to_seed(config_t *config){
  unsigned long retval = 0;
  int i;

  for(i = 5; i >= 0; i--){
    retval = (retval << 6) + (int)(config->serial[i] - ' ');
  }
  retval = retval * (3 + config->ports);
  retval = retval * (5 + config->batteries);
  retval = retval * (7 + config->indicators);
  return retval;
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

void putByte(byte data, int clock_pin, int data_pin) {
  byte i = 8;
  byte mask;
  while(i > 0) {
    mask = 0x01 << (i - 1);      // get bitmask
    digitalWrite(clock_pin, LOW);   // tick
    if (data & mask){            // choose bit
      digitalWrite(data_pin, HIGH);// send 1
    }else{
      digitalWrite(data_pin, LOW); // send 0
    }
    digitalWrite(clock_pin, HIGH);   // tock
    --i;                         // move to lesser bit
  }
}

void maxSingle(byte reg, byte col, int load_pin, int clock_pin, int data_pin) {
  //maxSingle is the "easy"  function to use for a single max7219
  digitalWrite(load_pin, LOW);        // begin
  putByte(reg, clock_pin, data_pin);  // specify register
  putByte(col, clock_pin, data_pin);  // put data
  digitalWrite(load_pin, LOW);        // and load da stuff
  digitalWrite(load_pin,HIGH);
}

void(* softwareReset) (void) = 0;

KTANEModule::KTANEModule(DSerialClient &dserial, int green_led_pin, 
                         int red_led_pin):_dserial(dserial) {
  memset(&_config, 0, sizeof(config_t));
  _red_led_pin = red_led_pin;
  _green_led_pin = green_led_pin;
  pinMode(_green_led_pin, OUTPUT);
  pinMode(_red_led_pin, OUTPUT);
  _got_config = 0;
  _num_strikes = 0;
  _got_reset = 0;
  is_solved = 0;
}

void KTANEModule::interpretData(){
  char out_message[MAX_MSG_LEN];
  unsigned long start_millis;
  
  _dserial.doSerial();
  if(_dserial.getData(out_message)) {
    if(out_message[0] == CONFIG && strlen(out_message) == 8) {
      _got_config = 1;
      raw_to_config((raw_config_t *)(out_message + 1), &_config);
    } else if(out_message[0] == RESET) {
      // All of the stuff before softwareReset() is currently useless
      //  but is kept in case the hard-reset call is removed.
      is_solved = 0;
      _num_strikes = 0;
      _got_config = 0;
      memset(&_config, 0, sizeof(config_t));
      _got_reset = 1;
      digitalWrite(_green_led_pin, LOW);

      // Delay for a small bit to allow client to ACK the reset.
      start_millis = millis();
      while(millis() - start_millis < 300){
        _dserial.doSerial();
      }
      softwareReset();
    } else if(out_message[0] == NUM_STRIKES) {
      _num_strikes = out_message[1];
    }
  }
}

// TODO: make non-blocking
// currently will block non-communication code for 500ms
int KTANEModule::strike() {
  int result = sendStrike();
  digitalWrite(_red_led_pin, HIGH);
  delayWithUpdates(*this, 500);
  digitalWrite(_red_led_pin, LOW);
  return result;
}

int KTANEModule::sendStrike() {
  char str[2] = {STRIKE, '\0'};
  return _dserial.sendData(str);
}

int KTANEModule::win() {
  digitalWrite(_green_led_pin, HIGH);
  return sendSolve();
}

int KTANEModule::sendSolve() {
  char str[2] = {SOLVE, '\0'};
  is_solved = 1;
  return _dserial.sendData(str);
}

// TODO: maybe make non-blocking
int KTANEModule::sendReady() {
  char str[2] = {READY, '\0'};
  int result = _dserial.sendData(str);
  if(result){
    digitalWrite(_red_led_pin, LOW);
    digitalWrite(_green_led_pin, HIGH);
    delayWithUpdates(*this, 300);
    digitalWrite(_green_led_pin, LOW);
  }
  return result;
}

int KTANEModule::sendDebugMsg(char *msg) {
  return _dserial.sendData(msg);
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

int KTANEModule::serialContainsVowel() {
  int serial_length = strlen(_config.serial);
  char temp;
  for(int i = 0; i < serial_length; i++) {
    temp = _config.serial[i];
    if(temp == 'A' || temp == 'E' || temp == 'I' || temp == 'O' || temp == 'U') {
      return 1;
    }
  }
  return 0;
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
      _strikes[client_id] = _strikes[client_id] + 1;
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
  char msg[3] = {NUM_STRIKES, (char)num_strikes, '\0'};
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

