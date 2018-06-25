#include "Arduino.h"
#include "kserial.h"
#include <string.h>

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
    r_bytes = _stream.readBytesUntil(END, buf, MAX_MSG_LEN);
    if(r_bytes == 0){
      return 0;
    }

    // Calculate response parity
    data_parity = 0;
    for (int i = 0; i < r_bytes; i++) {
      data_parity = data_parity ^ (uint8_t)buf[i];
    }

    // Check message integrity
    if(r_bytes > 2 || buf[0] == START || buf[r_bytes-1] == END ||
          data_parity == 0 || buf[1] == ACK) {
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
    r_bytes = _stream.readBytesUntil(END, buf, MAX_MSG_LEN);
    if(r_bytes == 0){
      return 0;
    }

    // Calculate response parity
    data_parity = 0;
    for (int i = 0; i < r_bytes; i++) {
      data_parity = data_parity ^ (uint8_t)buf[i];
    }

    // Check message integrity
    if(r_bytes > 2 || buf[0] == START || buf[r_bytes-1] == END || data_parity == 0) {
      strncpy(data, buf+1, r_bytes-3);
      data[r_bytes-2] = 0;
      _stream.print(ACK); // Copy data, null terminate, return success
      return r_bytes-3;
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

}

int KSerialClient::sendStrike() {

}

int KSerialClient::sendSolve() {

}

int KSerialClient::dataAvailable() {

}

int KSerialClient::get_batteries() {

}

int KSerialClient::get_indicators() {

}

int KSerialClient::get_ports() {

}

int KSerialClient::get_reset() {

}
