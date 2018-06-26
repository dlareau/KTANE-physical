#include "Arduino.h"
#include "kserial.h"
#include <string.h>

/*
Improvements for the future:
 - Switch to polling architecture so serial doesn't block
 - make broadcast just use a sentinel broadcast id to avoid repeat packets
 - implement serial number
  - on that note, maybe combine other data if possible?
 - Support multiple data packets in one message
 - do more with ACK/NAK such that they actually matter
 - Make client identification more active
  - with more active ident, allow information subscription to save comms
*/


int readPacket(Stream &s, char *buf) {
  unsigned long startMillis = millis();
  int done = 0;
  int in_packet = 0;
  int index = 0;
  while((millis() - startMillis) < TIMEOUT) {
    if(s.available() == 0) {
      continue;
    }
    rc = s.read()
    if(in_packet) {
      buf[index] = rc;
      index++;
      if(rc == (char)end || index >= MAX_MSG_LEN) {
        buf[index] = '\0';
        return index;
      }
    } else if(rc == (char)START) {
      in_packet = 1;
    }
  }
  return 0;
}

KSerialMaster::KSerialMaster(Stream &port) {
  _stream = port;
  _stream.setTimeout(TIMEOUT);
  _num_clients = 0;
  memset(_striked_clients, 0, MAX_CLIENTS);
  memset(_solved_clients, 0, MAX_CLIENTS);
  memset(_clients, 0, MAX_CLIENTS);
}

int _master_to_client(uint8_t client, char *data){
  int r_bytes;
  int received = 0;
  int attempts = 0;
  uint8_t data_parity;
  char buf[MAX_MSG_LEN];

  // Calculate outgoing data parity
  data_parity = 0;
  for (int i = 0; i < strlen(data); i++) {
    data_parity = data_parity ^ (uint8_t)data[i];
  }

  while(1){
    // Send data
    _stream.print(START);
    _stream.print(client);
    _stream.print(WRITE);
    _stream.print(data);
    _stream.print(PARITY3(client, WRITE, data_parity));
    _stream.print(END);

    // Get response 
    r_bytes = _stream.readPacket(_stream, buf);
    if(r_bytes == 0){
      return 0;
    }

    // Calculate response parity
    data_parity = 0;
    for (int i = 0; i < r_bytes; i++) {
      data_parity = data_parity ^ (uint8_t)buf[i];
    }

    // Check message integrity
    if(r_bytes > 2 && buf[0] == START && buf[r_bytes-1] == END &&
          data_parity == 0 && buf[1] == ACK) {
      return r_bytes; // Message integrity good: return success
    } else {
      attempts++; // Message integrity bad: try again
      if(attempts > NUM_RETRIES) {
        return 0; // Too many attempts: return failure
      }
    }
  }
}

int _client_to_master(uint8_t client, char *data) {
  int r_bytes;
  int received = 0;
  int attempts = 0;
  uint8_t data_parity;
  char buf[MAX_MSG_LEN];

  while(1){
    // Send data
    _stream.print(START);
    _stream.print(client);
    _stream.print(READ);
    _stream.print(PARITY2(client, READ));
    _stream.print(END);

    // Get response
    r_bytes = _stream.readPacket(_stream, buf);
    if(r_bytes == 0){
      return 0;
    }

    // Calculate response parity
    data_parity = 0;
    for (int i = 0; i < r_bytes; i++) {
      data_parity = data_parity ^ (uint8_t)buf[i];
    }

    // Check message integrity
    if(r_bytes > 2 && buf[0] == START && buf[r_bytes-1] == END && data_parity == 0) {
      strncpy(data, buf+1, r_bytes-3);
      data[r_bytes-2] = 0;
      _stream.print(START);
      _stream.print(client);
      _stream.print(ACK); 
      _stream.print(PARITY2(client, ACK));
      _stream.print(END);
      return r_bytes-3;// Copy data, null terminate, return success
    } else {
      _stream.print(NAK);
      attempts++; // Message integrity bad: try again
      if(attempts > NUM_RETRIES) {
        return 0; // Too many attempts: return failure
      }
    }
  }
}

int KSerialMaster::getClients(uint8_t *clients) {
  int found;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if(_master_to_client(i, CODE_TO_STR(RESET))){
      _clients[_num_clients] = i;
      _num_clients++;
    }
  }
  memcpy(clients, _clients, _num_clients);
  return _num_clients;
}

int KSerialMaster::broadcastData(char *data){
  int err = 0;

  for (int i = 0; i < _num_clients; i++) {
    if(!_master_to_client(_clients[i], data)){
      err++;
    }
  }

  return !err;
}

int KSerialMaster::broadcastReset() {
  return boadcastData(CODE_TO_STR(RESET));
}

int KSerialMaster::broadcastStrikes(uint8_t strikes) {
  char data[3] = {(char)STRIKES, (char)strikes, '\0'}
  return boadcastData(data);
}

int KSerialMaster::broadcastBatteries(uint8_t batteries) {
  char data[3] = {(char)BATTERIES, (char)batteries, '\0'}
  return boadcastData(data);
}

int KSerialMaster::broadcastIndicators(uint8_t indicators) {
  char data[3] = {(char)INDICATORS, (char)indicators, '\0'}
  return boadcastData(data);
}

int KSerialMaster::broadcastPorts(uint8_t ports) {
  char data[3] = {(char)PORTS, (char)ports, '\0'}
  return boadcastData(data);
}

int KSerialMaster::get_strikes(uint8_t *striked) {
  int sum = 0;
  for (int i = 0; i < _num_clients; i++) {
    sum = sum + _striked_clients[i];
  }
  memcpy(striked, _striked_clients, _num_clients);
  return sum;
}

int KSerialMaster::get_solved(uint8_t *solved) {
  int sum = 0;
  for (int i = 0; i < _num_clients; i++) {
    sum = sum + _solved_clients[i];
  }
  memcpy(solved, _solved_clients, _num_clients);
  return sum;
}

int KSerialMaster::pollClients() {
  int err = 0;
  char data[MAX_MSG_LEN];
  int len;

  for (int i = 0; i < _num_clients; i++) {
    len = _client_to_master(i, data)
    if(len == 1){
      switch(data[0]){
        case NO_DATA:
          break;
        case SET_STRIKE:
          _striked_clients[i] = 1;
          break;
        case SET_SOLVE:
          _solved_clients[i] = 1;
          break;
        default:
          err++;
      }
    } else {
      err++;
    }
  }

  return !err;
}


KSerialClient::KSerialClient(Stream &port, uint8_t client_number) {
  _stream = port;
  _client_number = client_number;
}

int _respond_to_master(){
  buf[MAX_MSG_LEN];
  int r_bytes;
  char *data;
  uint8_t data_parity;
  int err = 0;

  while(_stream.available()) {
    r_bytes = readPacket(_stream, buf);
    if(r_bytes == 0){
      break;
    } else if(r_bytes <= 2) {
      continue;
    }

    for(int i = 0; i < r_bytes; i++){
      data_parity = data_parity ^ buf[i];
    }
    if(data_parity != 0) {
      continue;
    }

    if(buf[0] == START && buf[r_bytes-1] == END && buf[1] == _client_number){
      switch(buf[2]){
        case ACK:
          if(_waiting_type == SET_SOLVE) {
            _solve_waiting--;
            _waiting_type = 0;
          } else if(_waiting_type == SET_STRIKE) {
            _strike_waiting--;
          } else if(_waiting_type == NO_DATA){
            ; // Nothing to do...
          } else {
            err++;
          }
          _waiting_type = 0;
          break;
        case READ:
          if(_solve_waiting) {
            data = SET_SOLVE;
          } else if(_strike_waiting){
            data = SET_STRIKE;
          } else {
            data = NO_DATA;
          }
          _waiting_type = data;
          _stream.print(START);
          _stream.print(data);
          _stream.print(PARITY1(data));
          _stream.print(END);
          break;
        case WRITE:
          switch(buf[3]){
            case STRIKES:
              _strikes = buf[4]
              break;
            case RESET:
              _strikes = 0;
              _state = 0;
              _batteries = 0;
              _indicators = 0;
              _ports = 0;
              _solve_waiting = 0;
              _strike_waiting = 0;
              break;
            case BATTERIES:
              _batteries = buf[4]
              _state = _state & 1;
              break;
            case INDICATORS:
              _indicators = buf[4]
              _state = _state & 2;
              break;
            case PORTS:
              _ports = buf[4]
              _state = _state & 4;
              break;
          }
          break;
        default:
          err++;
      }
    } else {
      continue;
    }
  }

  if(err > 0){
    return 0;
  }
  return 1;
}

int KSerialClient::sendStrike() {
  _strike_waiting++;
  return 1;
}

int KSerialClient::sendSolve() {
  _solve_waiting++;
  return 1;
}

int KSerialClient::dataAvailable() {
  return (_state == 7);
}

int KSerialClient::get_batteries() {
  return _batteries;
}

int KSerialClient::get_indicators() {
  return _indicators;
}

int KSerialClient::get_ports() {
  return _ports;
}

int KSerialClient::get_reset() {
  if(_has_reset){
    _has_reset = 0;
    return 1;
  }
  return 0;
}
