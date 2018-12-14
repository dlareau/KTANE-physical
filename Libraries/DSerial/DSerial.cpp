/** @file dserial.cpp
 *  @brief The DSerial library implementation
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "DSerial.h"
#include <string.h>
#include "stringQueue.h"

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
 */
int readPacket(Stream &s, char *buffer){
  static int in_packet = 0;
  static int index = 0;
  static char buf[MAX_MSG_LEN+1];
  static char data_parity = 0;
  static int escape_next = 0;
  char rc;
  int passed_parity = 0;

  while (s.available() > 0) {
    rc = s.read();
    if (rc == START) {
      in_packet = 1;
      data_parity = START;
      index = 0;
    }
    else if (in_packet == 1) {
      data_parity ^= rc;
      if (rc != END) {
        if(rc == ESC) { // next char was greater than 0x7F, add 0x80 to it.
          escape_next = 1;
        } else {
          buf[index] = rc | (escape_next << 7);
          escape_next = 0;
          index++;
        }
      }
      if(rc == END || index >= MAX_MSG_LEN){
        index--;
        buf[index] = '\0'; //purposefully overwrite parity byte.
        strcpy(buffer, buf);
        passed_parity = ((data_parity & 0x7F) == 0);
        index = 0;
        in_packet = 0;
        data_parity = 0;
        if(passed_parity){
          return 1;
        } else {
          return -1;
        }
      }
    }
  }
  return 0;
}

/** @brief Writes a packet to the specified stream.
 * 
 *  @param s        The stream object from which to read
 *  @param message  A pointer to the message to send, first byte should be
                      a client address, message should be null terminated,
                      however the null byte will not be sent over the wire.
 *  @return A status code indicating the whether or not the message was sent
 *            - Currently this function will always succeed
 */
int sendPacket(Stream &s, char *message){
  char data_parity = 0;
  if(strlen(message) == 0){
    return 0; // Should only happen on library failure
  }
  for (int i = 0; message[i] != 0; i++) {
    data_parity = data_parity ^ (uint8_t)message[i];
  }
  s.write(START);
  for (int i = 0; message[i] != 0; i++) {
    if((message[i] & 0x80) == 0) {
      s.write(message[i]);
    } else {  // special case for bytes over 0x7F
      s.write(ESC);
      s.write(message[i] & 0x7F);
    }
  }
  s.write((START ^ data_parity ^ END) & 0x7F);
  s.write(END);
  return 1;
}

/** @brief Creates a new DSerialMaster object
 * 
 *  @param port The underlying stream object used for communication.

 *  @return A new initialized DSerialMaster object
 */
DSerialMaster::DSerialMaster(Stream &port):_stream(port){
  _state = 0;
  _num_clients = 0;
  memset(_clients, 0, MAX_CLIENTS);
  stringQueueInit(&_in_messages, MAX_MASTER_QUEUE_SIZE);
  stringQueueInit(&_out_messages, MAX_MASTER_QUEUE_SIZE);
}

/** @brief sends a data string to the specified client.
 *
 *  Internally, this function enqueues the data to be written when convenient.
 *  This function fails when there is not enough memory to enqueue the message
 *  or when the queue is full.
 *
 *  @param client_id  The ID of the client to write to
 *  @param data       The data to write to the given client
 *  @return A status code indicating success or failure
 */
int DSerialMaster::sendData(uint8_t client_id, char *data){
  if(stringQueueIsFull(&_out_messages)){
    return 0;
  }
  char *new_message = (char*) malloc(MAX_MSG_LEN+2);
  if(new_message == NULL){
    return 0;
  }
  strcpy(new_message+2, data);
  new_message[0] = (char)client_id;
  new_message[1] = WRITE;
  stringQueueAdd(&_out_messages, new_message);
  return 1;
}

/** @brief Retrieve data if there is any to get
 *
 *  @param buffer A string to populate with the possible data
 *  @return The ID of the client that sent the message, 0 if no data.
 */
int DSerialMaster::getData(char *buffer){
  int client_id;
  char *message;
  if(stringQueueIsEmpty(&_in_messages)){
    return 0;
  }
  message = stringQueueRemove(&_in_messages);
  client_id = message[0];
  strcpy(buffer, message+1);
  free(message);
  return client_id;
}

/** @brief runs a client search
 *
 *  A client search consists of pinging each client address between 1 and 
 *  MAX_CLIENTS. If the client responds, then it gets put in our array.
 *
 *  @return The number of clients found
 */
int DSerialMaster::identifyClients() {
  unsigned long start_millis;
  char temp[MAX_MSG_LEN];
  char message[3] = {(char)1, PING, '\0'};
  _num_clients = 0;
  memset(_clients, 0, MAX_CLIENTS);

  while(_state != MASTER_WAITING){
    doSerial(); // RETURN_CODE?
  }

  for (int i = 1; i < MAX_CLIENTS; i++) {
    message[0] = (char)i;
    sendPacket(_stream, message);
    start_millis = millis();
    while(millis() - start_millis < TIMEOUT){
      int result = readPacket(_stream, temp);
      if(result > 0){
        _clients[_num_clients] = i;
        _num_clients++;
        break;
      }
    }
  }
  return _num_clients;
}

/** @brief gets the client array and number of clients
 *
 *  If the clients argument is NULL, the number of clients will still be
 *  returned, but it will not attempt to populate the array.
 *
 *  @param clients  a pointer to memory of at least MAX_CLIENTS size to put
                      the the clients into 
 *  @return The number of clients found
 */
int DSerialMaster::getClients(uint8_t *clients){
  if(clients != NULL){
    memcpy(clients, _clients, _num_clients);
  }
  return _num_clients;
}

int DSerialMaster::doSerial(){
  static unsigned long last_millis;
  static uint8_t num_attempts;
  static uint8_t client_index = 0;
  static char    current_msg[MAX_MSG_LEN+1];
  char *msg_ptr;
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
    short_msg[0] = current_msg[0];
    short_msg[1] = NAK;
    sendPacket(_stream, short_msg);
    strcpy(current_msg, short_msg);
    return 1;
  }
  switch(_state){
    // WAITING state: ignore incoming, send waiting, otherwise poll.
    case MASTER_WAITING:
      if(!stringQueueIsEmpty(&_out_messages)){
        msg_ptr =  stringQueueRemove(&_out_messages);
        strcpy(current_msg, msg_ptr);
        free(msg_ptr);
        _state = MASTER_ACK;
      } else if(_num_clients > 0 && !stringQueueIsFull(&_in_messages)) {
        client_index = (client_index + 1) % _num_clients;
        short_msg[0] = (char)_clients[client_index];
        short_msg[1] = READ;
        strcpy(current_msg, short_msg);
        _state = MASTER_SENT;
      } else {
        return 1; // No data to send and no clients to poll
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
          stringQueueAdd(&_in_messages, buffer); // we're safe because of earlier check
          short_msg[1] = ACK;
          sendPacket(_stream, short_msg);
          strcpy(current_msg, short_msg);
          _state = MASTER_ACK;
        }
      }

      break;

    // ACK state: waiting for client ACK, dealing with timeouts and non-ACKs
    case MASTER_ACK:
      // Deal with packet
      if(result == 0){       // Timed out, send ACK again
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
          _state = MASTER_WAITING;
        } else {
          free(buffer);
          short_msg[1] = NAK;
          sendPacket(_stream, short_msg);
          strcpy(current_msg, short_msg);
        }
      }

      break;
  }
  return 1;
}

/** @brief Creates a new DSerialClient object
 * 
 *  @param port The underlying stream object used for communication.

 *  @return A new initialized DSerialClient object
 */
DSerialClient::DSerialClient(Stream &port, uint8_t client_number):_stream(port){
  _state = 0;
  _client_number = client_number;
  stringQueueInit(&_in_messages, MAX_CLIENT_QUEUE_SIZE);
  stringQueueInit(&_out_messages, MAX_CLIENT_QUEUE_SIZE);
}

/** @brief sends a data string to the master.
 *
 *  Internally, this function enqueues the data to be written when convenient.
 *  This function fails when there is not enough memory to enqueue the message
 *  or when the queue is full.
 *
 *  @param data The data to write to the given client
 *  @return A status code indicating success or failure
 */
int DSerialClient::sendData(char *data){
  if(stringQueueIsFull(&_out_messages)){
    return 0;
  }
  char *new_message = (char*) malloc(MAX_MSG_LEN+1);
  if(new_message == NULL){
    return 0;
  }
  strcpy(new_message+1, data);
  new_message[0] = (char)_client_number;
  stringQueueAdd(&_out_messages, new_message);
  return 1;
}

/** @brief Retrieve data if there is any to get
 *
 *  @param buffer A string to populate with the possible data
 *  @return A status code indicating whether data was retrieved
 */
int DSerialClient::getData(char *buffer){
  char *message;
  if(stringQueueIsEmpty(&_in_messages)){
    return 0;
  }
  message = stringQueueRemove(&_in_messages);
  strcpy(buffer, message+2);
  free(message);
  return 1;
}

int DSerialClient::doSerial(){
  static char    current_msg[MAX_MSG_LEN+1];
  char short_msg[3] = {(char)_client_number, '\0', '\0'};
  char *msg_ptr;

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
    return 1;
  }
  if(buffer[1] == NAK){
    free(buffer);
    sendPacket(_stream, current_msg);
    return 1;
  }
  switch(_state){
    // WAITING state: respond to any requests
    case CLIENT_WAITING:
      if(buffer[1] == READ){
        if(!stringQueueIsEmpty(&_out_messages)){
          msg_ptr =  stringQueueRemove(&_out_messages);
          strcpy(current_msg, msg_ptr);
          free(msg_ptr);
          _state = CLIENT_SENT;
        } else {
          short_msg[1] = ACK;
          strcpy(current_msg, short_msg);
        }
        free(buffer);
      } else if(buffer[1] == WRITE && !stringQueueIsFull(&_in_messages)) {
        stringQueueAdd(&_in_messages, buffer);
        short_msg[1] = ACK;
        strcpy(current_msg, short_msg);
      } else if(buffer[1] == PING) {
        short_msg[1] = ACK;
        strcpy(current_msg, short_msg);
        free(buffer);
      } else {
        return 0;
      }
      sendPacket(_stream, current_msg);

      break;

    // SENT state: look for and respond to ACK.
    case CLIENT_SENT:
      if(buffer[1] == ACK){ // Client ACK'd read request indicating no data
        _state = CLIENT_WAITING;
        short_msg[1] = ACK;
        strcpy(current_msg, short_msg);
        sendPacket(_stream, current_msg);
      }
      free(buffer);

      break;
  }
  return 1;
}