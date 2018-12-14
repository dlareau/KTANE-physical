/** @file dserial.h
 *  @brief Headers and definitions for the DSerial Library
 *
 *  The DSerial library adds a layer-3/4 interface intended to be used on top
 *  of a serial UART multi-drop bus. DSerial adds addressing and message 
 *  acknowledgment in a master/client configuration. Clients are polled for
 *  data. 
 *
 *  The library is divided into Master and Client classes. In order for constant
 *  data flow, all communicating parties should call "doSerial" often. Functions
 *  in the DSerial library do not block and all but getClients should be
 *  execute relatively quickly.
 *
 *  Currently, only bytes with values between 1 and 127 inclusive are allowed 
 *  to be sent via this protocol. Bytes above 127 will work, but could cause 
 *  errors if they are reserved bytes (see #define section below) and support
 *  for sending 0's is hopefully coming soon.
 *
 *  Definitions:
 *    - Packet: Data in the form of {START}{MESSAGE}{PARITY}{END}
 *      - Currently, the first byte of the message is the client address
 *    - Valid addresses for clients are between 1 and MAX_CLIENTS
 *      - MAX_CLIENTS can be at most 126.
 *
 *  The overall interaction method with this library should be through the 
 *  sendData and getData methods on the master and client objects. Unlike the
 *  notion of a message used internally, these strings do not contain the id
 *  of the intended recipient.
 *
 *  (Internally however, a "message" always contains the destination client id
 *   as the first byte)
 *
 *  At because it is impossible to know which client a corrupted packet was
 *  intended for, clients to not NAK corrupt packets, instead the transaction
 *  will just time out and the master node will retry the transaction. This 
 *  takes longer, but because the whole library is non-blocking it's fine.
 *
 *  Future improvements:
 *    - Switch to all length based binary processing to allow nulls
 *    - Have the client address be broken out into the packet datatype.
 *
 *  Current transaction structure:
 *    Client -> Master:
 *      1 M: {READ}
 *      2 C: {DATA}
 *      3 M: {ACK}
 *      4 C: {ACK}
 *    
 *    Client -> Master (If no data):
 *      1 M: {READ}
 *      2 C: {ACK}
 *    
 *    Master -> Client:
 *      1 M: {WRITE}{DATA}
 *      2 C: {ACK}
 *
 *  @author Dillon Lareau (dlareau)
 */

#pragma once
#include "Arduino.h"
#include "stringQueue.h"

// Control characters
// All of the form 0x80 + (most appropriate ascii character)
#define ACK (char)0x86
#define NAK (char)0x95
#define START (char)0x82
#define END (char)0x83
#define WRITE (char)0xD7
#define READ (char)0xD2
#define NO_DATA (char)0xB0
#define PING (char)0xB1
#define ESC (char)0x9B

#define TIMEOUT 50
#define MAX_CLIENTS 16
#define MAX_MSG_LEN 16
#define MAX_MASTER_QUEUE_SIZE 40
#define MAX_CLIENT_QUEUE_SIZE 20
#define MAX_RETRIES 3

#define MASTER_WAITING 0
#define MASTER_SENT 1
#define MASTER_ACK 2
#define CLIENT_WAITING 0
#define CLIENT_SENT 1

int readPacket(Stream *s, char *buffer);
int sendPacket(Stream *s, char *message);

class DSerialMaster {
  public:
    DSerialMaster(Stream &port);
    int sendData(uint8_t client_id, char *data);
    int getData(char *buffer);
    int doSerial();
    int identifyClients();
    int getClients(uint8_t *clients);

  private:
    Stream   &_stream;
    uint8_t   _state;
    stringQueue_t _in_messages;
    stringQueue_t _out_messages;
    uint8_t   _num_clients;
    uint8_t   _clients[MAX_CLIENTS];
};

class DSerialClient {
  public:
    DSerialClient(Stream &port, uint8_t client_number);
    int sendData(char *data);
    int getData(char *buffer);
    int doSerial();

  private:
    Stream   &_stream;
    uint8_t   _state;
    stringQueue_t _in_messages;
    stringQueue_t _out_messages;
    uint8_t   _client_number;
};
