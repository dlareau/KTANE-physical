#include "Arduino.h"
#include "kserial.h"
#include <string.h>

// Improvements:
// Switch queues to FIFO rather than LIFO
// Switch to all length based binary processing to allow nulls
// Check read/write packet return codes everywhere...

// returns 1 for good packets, -1 for malformed packet, and 0 for no new data
int readPacket(Stream s, char *out_message){
  static int in_packet = 0;
  static int index = 0;
  static char buf[MAX_MSG_LEN+1];
  static char data_parity = 0;
  char rc;


  // TODO: Add duplicate start or end detection
  while (s.available() > 0) {
    rc = s.read();

    if (in_packet == 1) {
      data_parity ^= rc;
      if (rc != END) {
        buf[index] = rc;
        index++;
      }
      if(rc == END || index >= MAX_MSG_LEN){
        buf[--index] = '\0'; //purposefully overwrite parity.
        strcpy(out_message, buf);
        index = 0;
        in_packet = 0;
        data_parity = 0;
        if(data_parity == 0){
          return 1;
        } else {
          return -1;
        }
      }
    }
    else if (rc == (char)START) {
      in_packet = 1;
      data_parity = START;
    }
  }
  return 0;
}

// Message does need to be null terminated, the null terminator will not be sent.
int sendPacket(Stream s, char *message){
  char data_parity = 0;
  for (int i = 0; i < strlen(message); i++) {
    data_parity = data_parity ^ (uint8_t)data[i];
  }
  _stream.print(START);
  _stream.print(message);
  _stream.print((START ^ data_parity ^ END));
  _stream.print(END);
  return 1;
}


KSerialMaser::KSerialMaster(Stream &port){
  _stream = port;
  _state = 0;
  _num_clients = 0;
  _num_in_messages = 0;
  _num_out_messages = 0;
  memset(_clients, 0, MAX_CLIENTS);
  memset(_in_messages, 0, MAX_CLIENTS);
  memset(_out_messages, 0, MAX_CLIENTS);
}

int KSerialMaster::sendData(uint8_t client_id, char *data){
  char *new_message = (char*) malloc(MAX_MSG_LEN+2);
  if(new_message == NULL){
    return 0;
  }
  strcpy(new_message+2, data);
  new_message[0] = (char)client_id;
  new_message[1] = (char)WRITE;
  out_messages[_num_out_messages++] = new_message;
  return 1;
}

int KSerialMaster::getData(char *buffer){
  if(_num_in_messages == 0){
    return 0;
  }
  strcpy(buffer, _in_messages[_num_in_messages-1]+1);
  free(_in_messages[_num_in_messages]);
  _num_in_messages--;
  return 1;
}

int KSerialMaster::getClients(uint8_t *clients){
  unsigned long start_millis;
  char temp[MAX_MSG_LEN];
  char message[3] = {(char)1, (char)PING, '\0'};

  while(_state != MASTER_WAITING){
    do_serial();
  }

  for (int i = 1; i < MAX_CLIENTS; i++) {
    message[0] = (char)i;
    sendPacket(_stream, message);
    start_millis; = millis();
    while(millis() - start_millis < TIMEOUT){
      int result = readPacket(_stream, temp);
      if(result > 0){
        _clients[_num_clients] = i;
        _num_clients++;
        break;
      }
    }
  }
  memcpy(clients, _clients, _num_clients);
  return _num_clients;
}

int KSerialMaster::do_serial(){
  static unsigned long last_millis;
  static uint8_t num_attempts;
  static uint8_t client_index = 0;
  static char    last_msg[MAX_MSG_LEN+1];
  char short_msg[3] = {(char)_clients[client_index], '\0', '\0'};
  int result;

  switch(_state){
    case MASTER_WAITING:
      if(_num_out_messages > 0){
        sendPacket(_stream, _out_messages[_num_out_messages-1]);
        strcpy(last_msg, _out_messages[_num_out_messages-1]);
        free(_out_messages[--_num_out_messages])
        num_attempts = 0;
        _state = MASTER_ACK;
        last_millis = millis();
      } else {
        client_index = (client_index + 1) % _num_clients;
        short_msg[0] = (char)_clients[client_index];
        short_msg[1] = (char)READ;
        sendPacket(_stream, short_msg);
        strcpy(last_msg, short_msg);
        num_attempts = 0;
        _state = MASTER_SENT;
        last_millis = millis();
      }

      break;

    case MASTER_SENT: // Assumed mid-read
      char *buffer = (char*) malloc(MAX_MSG_LEN+1);
      if(buffer == NULL){ // Fail if buffer allocation failed
        return 0;
      }

      result = readPacket(_stream, buffer);

      if(result != 1){ // Free buffer if we didn't get a useful packet
        free(buffer);
      }

      // Deal with packet
      if(result == -1) {            // Bad data, send NAK.
        short_msg[1] = (char)NAK;
        sendPacket(_stream, short_msg);
        strcpy(last_msg, short_msg);
      } else if(result == 0){       // Timed out, send READ again
        if(millis() - last_millis > TIMEOUT) {
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, last_msg);
          num_attempts++;
        }
      } else if(result == 1) {      // Useful packet
        if(buffer[1] == ACK){
          free(buffer);
          _state = MASTER_WAITING;
        } else {
          _in_messages[_num_in_messages++] = buffer;
          short_msg[1] = (char)ACK;
          sendPacket(_stream, short_msg);
          strcpy(last_msg, short_msg);
          _state = MASTER_ACK;
        }
      }

      break;

    case MASTER_ACK:
      char *buffer = (char*) malloc(MAX_MSG_LEN+1);
      if(buffer == NULL){ // Fail if buffer allocation failed
        return 0;
      }

      result = readPacket(_stream, buffer);

      if(result != 1){ // Free buffer if we didn't get a useful packet
        free(buffer);
      }

      // Deal with packet
      if(result == -1) {            // Bad data, send NAK.
        short_msg[1] = (char)NAK;
        sendPacket(_stream, short_msg);
        strcpy(last_msg, short_msg);
      } else if(result == 0){       // Timed out, send READ again
        if(millis() - last_millis > TIMEOUT) {
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, last_msg);
          num_attempts++;
        }
      } else if(result == 1) {      // Useful packet
        if(buffer[1] == ACK){
          free(buffer);
          _state = _WAITING;
        } else {
          short_msg[1] = (char)NAK;
          sendPacket(_stream, short_msg);
          strcpy(last_msg, short_msg);
        }
      }

      break;
  }
}


KSerialClient::KSerialClient(Stream &port, uint8_t client_number){
  _stream = port;
  _state = 0;
  _client_number = client_number;
  _num_in_messages = 0;
  _num_out_messages = 0;
  memset(_in_messages, 0, MAX_CLIENTS);
  memset(_out_messages, 0, MAX_CLIENTS);
}

int KSerialClient::sendData(char *data){
  if(_num_out_messages >= MAX_QUEUE_SIZE){
    return 0;
  }
  char *new_message = (char*) malloc(MAX_MSG_LEN+1);
  if(new_message == NULL){
    return 0;
  }
  strcpy(new_message+1, data);
  new_message[0] = (char)client_id;
  out_messages[_num_out_messages++] = new_message;
  return 1;
}

int KSerialMaster::getData(char *buffer){
  if(_num_in_messages == 0){
    return 0;
  }
  strcpy(buffer, _in_messages[_num_in_messages-1]+2);
  free(_in_messages[_num_in_messages]);
  _num_in_messages--;
  return 1;
}

int KSerialClient::do_serial(){
  static unsigned long last_millis;
  static uint8_t num_attempts;
  static char    last_msg[MAX_MSG_LEN+1];
  char short_msg[3] = {(char)1, '\0', '\0'};
  int result;

  switch(_state){
    // !!! In this one and in other do_serial, (needs camelcase), pull readPacket out to top.

    case MASTER_WAITING:
      if(/* check for master */) {
        // Respond to master
      }
      // Code for sending requested data
      sendPacket(_stream, _out_messages[_num_out_messages-1]);
      strcpy(last_msg, _out_messages[_num_out_messages-1]);
      free(_out_messages[--_num_out_messages])
      num_attempts = 0;
      _state = CLIENT_WAITING;
      last_millis = millis();

      break;

    case MASTER_SENT: // Assumed mid-read
      char *buffer = (char*) malloc(MAX_MSG_LEN+1);
      if(buffer == NULL){ // Fail if buffer allocation failed
        return 0;
      }

      result = readPacket(_stream, buffer);

      if(result != 1){ // Free buffer if we didn't get a useful packet
        free(buffer);
      }

      // Deal with packet
      if(result == -1) {            // Bad data, send NAK.
        short_msg[1] = (char)NAK;
        sendPacket(_stream, short_msg);
        strcpy(last_msg, short_msg);
      } else if(result == 0){       // Timed out, send READ again
        if(millis() - last_millis > TIMEOUT) {
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, last_msg);
          num_attempts++;
        }
      } else if(result == 1) {      // Useful packet
        if(buffer[1] == ACK){
          free(buffer);
          _state = MASTER_WAITING;
        } else {
          _in_messages[_num_in_messages++] = buffer;
          short_msg[1] = (char)ACK;
          sendPacket(_stream, short_msg);
          strcpy(last_msg, short_msg);
          _state = MASTER_ACK;
        }
      }

      break;

    case MASTER_ACK:
      char *buffer = (char*) malloc(MAX_MSG_LEN+1);
      if(buffer == NULL){ // Fail if buffer allocation failed
        return 0;
      }

      result = readPacket(_stream, buffer);

      if(result != 1){ // Free buffer if we didn't get a useful packet
        free(buffer);
      }

      // Deal with packet
      if(result == -1) {            // Bad data, send NAK.
        short_msg[1] = (char)NAK;
        sendPacket(_stream, short_msg);
        strcpy(last_msg, short_msg);
      } else if(result == 0){       // Timed out, send READ again
        if(millis() - last_millis > TIMEOUT) {
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, last_msg);
          num_attempts++;
        }
      } else if(result == 1) {      // Useful packet
        if(buffer[1] == ACK){
          free(buffer);
          _state = _WAITING;
        } else {
          short_msg[1] = (char)NAK;
          sendPacket(_stream, short_msg);
          strcpy(last_msg, short_msg);
        }
      }

      break;
  }
}