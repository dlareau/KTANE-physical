/** @file dserial.cpp
 *  @brief The DSerial library implementation
 *
 *  Future improvements:
 *    - Switch queues to FIFO rather than LIFO
 *    - Switch to all length based binary processing to allow nulls
 *    - Check read/write packet return codes everywhere...
 *    - Have the address be broken out into the packet datatype.
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "dserial.h"
#include <string.h>

/** @brief Reads a packet from the specified stream if one is available
 *
 *  If the data in the stream contains a full packet, the packet will be put
 *  into the return buffer and the return code will indicate it's validity.
 *  If there is not enough data in the stream buffer for a full packet, the
 *  function not populate the buffer and the return code will indicate no new
 *  packet.
 *
 *  Returned data DOES include the address as the first byte of the buffer.
 *
 *  @param s      The stream object from which to read
 *  @param buffer A pointer to the buffer to populate with the possible packet
 *  @return A status code indicating the status of the packet:
 *            0 - No new packet, buffer is returned empty.
 *            1 - New packet in buffer, packet is valid.
 *           -1 - New packet in buffer, packet failed parity check.
 *
 *  @bug Function does not currently out-of-order start/end bytes well. 
 */
int readPacket(Stream &s, char *buffer){
  static int in_packet = 0;
  static int index = 0;
  static char buf[MAX_MSG_LEN+1];
  static char data_parity = 0;
  char rc;

  while (s.available() > 0) {
    rc = s.read();

    if (in_packet == 1) {
      data_parity ^= rc;
      if (rc != END) {
        buf[index] = rc;
        index++;
      }
      if(rc == END || index >= MAX_MSG_LEN){
        buf[--index] = '\0'; //purposefully overwrite parity byte.
        strcpy(buffer, buf);
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

/** @brief Writes a packet to the specified stream.
 *
 *  @param s        The stream object from which to read
 *  @param message  A pointer to the message to send, first byte should be
                      a client address
 *  @return A status code indicating the whether or not the message was sent
 *            - Currently this function will always succeed
 */
// Message does need to be null terminated, the null terminator will not be sent.
int sendPacket(Stream &s, char *message){
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


DSerialMaster::DSerialMaster(Stream &port){
  _stream = port;
  _state = 0;
  _num_clients = 0;
  _num_in_messages = 0;
  _num_out_messages = 0;
  memset(_clients, 0, MAX_CLIENTS);
  memset(_in_messages, 0, MAX_CLIENTS);
  memset(_out_messages, 0, MAX_CLIENTS);
}

int DSerialMaster::sendData(uint8_t client_id, char *data){
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

int DSerialMaster::getData(char *buffer){
  if(_num_in_messages == 0){
    return 0;
  }
  strcpy(buffer, _in_messages[_num_in_messages-1]+1);
  free(_in_messages[_num_in_messages]);
  _num_in_messages--;
  return 1;
}

int DSerialMaster::getClients(uint8_t *clients){
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

int DSerialMaster::doSerial(){
  static unsigned long last_millis;
  static uint8_t num_attempts;
  static uint8_t client_index = 0;
  static char    current_msg[MAX_MSG_LEN+1];
  char short_msg[3] = {(char)_clients[client_index], '\0', '\0'};

  // Read stream for input
  char *buffer = (char*) malloc(MAX_MSG_LEN+1);
  if(buffer == NULL){ // Fail if buffer allocation failed
    return 0;
  }
  int result = readPacket(_stream, buffer);
  if(result != 1){ // Free buffer if we didn't get a useful packet
    free(buffer);
  }
  if(result == -1) {            // Bad data, send NAK.
    short_msg[0] = last_msg[0];
    short_msg[1] = (char)NAK;
    sendPacket(_stream, short_msg);
    strcpy(current_msg, short_msg);
    continue;
  }

  switch(_state){
    // WAITING state: ignore incoming, send waiting, otherwise poll.
    case MASTER_WAITING:
      if(_num_out_messages > 0){
        strcpy(current_msg, _out_messages[_num_out_messages-1]);
        free(_out_messages[--_num_out_messages])
        _state = MASTER_ACK;
      } else {
        client_index = (client_index + 1) % _num_clients;
        short_msg[0] = (char)_clients[client_index];
        short_msg[1] = (char)READ;
        strcpy(current_msg, short_msg);
        _state = MASTER_SENT;
      }
      sendPacket(_stream, current_msg);
      num_attempts = 0;
      last_millis = millis();

      break;

    // SENT state: assumed mid-read, deal with timeout/valid read.
    case MASTER_SENT:

      if(result == 0){
        if(millis() - last_millis > TIMEOUT) { // Timed out, send READ again
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, current_msg);
          num_attempts++;
        }
      } else if(result == 1) { // Useful packet
        if(buffer[1] == ACK){ // Client ACK'd read request indicating no data
          free(buffer);
          _state = MASTER_WAITING;
        } else {
          _in_messages[_num_in_messages++] = buffer;
          short_msg[1] = (char)ACK;
          sendPacket(_stream, short_msg);
          strcpy(current_msg, short_msg);
          _state = MASTER_ACK;
        }
      }

      break;

    // ACK state: waiting for client ACK, dealing with timeouts and non-ACKs
    case MASTER_ACK:
      // Deal with packet
      if(result == 0){       // Timed out, send READ again
        if(millis() - last_millis > TIMEOUT) {
          if(num_attempts >= MAX_RETRIES){
            _state = MASTER_WAITING;
            return 0;
          }
          sendPacket(_stream, current_msg);
          num_attempts++;
        }
      } else if(result == 1) {      // Useful packet
        if(buffer[1] == ACK){
          free(buffer);
          _state = _WAITING;
        } else {
          free(buffer);
          short_msg[1] = (char)NAK;
          sendPacket(_stream, short_msg);
          strcpy(current_msg, short_msg);
        }
      }

      break;
  }
  return 1;
}


DSerialClient::DSerialClient(Stream &port, uint8_t client_number){
  _stream = port;
  _state = 0;
  _client_number = client_number;
  _num_in_messages = 0;
  _num_out_messages = 0;
  memset(_in_messages, 0, MAX_CLIENTS);
  memset(_out_messages, 0, MAX_CLIENTS);
}

int DSerialClient::sendData(char *data){
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

int DSerialMaster::getData(char *buffer){
  if(_num_in_messages == 0){
    return 0;
  }
  strcpy(buffer, _in_messages[_num_in_messages-1]+2);
  free(_in_messages[_num_in_messages]);
  _num_in_messages--;
  return 1;
}

int DSerialClient::doSerial(){
  static unsigned long last_millis;
  static char    current_msg[MAX_MSG_LEN+1];
  char short_msg[3] = {(char)MASTER_ID, '\0', '\0'};

  // Read stream for input
  char *buffer = (char*) malloc(MAX_MSG_LEN+1);
  if(buffer == NULL){ // Fail if buffer allocation failed
    return 0;
  }
  int result = readPacket(_stream, buffer);
  if(result != 1){ // Free buffer if we didn't get a useful packet
    free(buffer);
    return 1;
  }
  if(buffer[0] != _client_number){
    free(buffer);
    return 1
  }
  if(buffer[1] == NAK){
    free(buffer);
    sendPacket(_stream, current_msg);
    return 1;
  }

  switch(_state){
    // WAITING state: respond to any requests
    case CLIENT_WAITING:
      if(buffer[1] == READ and _num_out_messages > 0){
        strcpy(current_msg, _out_messages[_num_out_messages-1]);
        free(_out_messages[--_num_out_messages])
        _state = CLIENT_SENT;
      } else if(buffer[1] == WRITE) {
        _in_messages[_num_in_messages++] = buffer;
        short_msg[1] = (char)ACK;
        strcpy(current_msg, short_msg);
      }
      sendPacket(_stream, current_msg);

      break;

    // SENT state: look for and respond to ACK.
    case CLIENT_SENT:
      if(buffer[1] == ACK){ // Client ACK'd read request indicating no data
        _state = CLIENT_WAITING;
        short_msg[1] = (char)ACK;
        strcpy(current_msg, short_msg);
        sendPacket(_stream, current_msg);
      }
      free(buffer);

      break;
  }
}