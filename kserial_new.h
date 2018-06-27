#pragma once
#include "Arduino.h"

#define ACK 0x86
#define NAK 0x95
#define START 0x82
#define END 0x83
#define WRITE 0x57
#define READ 0x52
#define NO_DATA 0xB0
#define PING 0xB1

#define TIMEOUT 50
#define MAX_CLIENTS 16
#define MAX_MSG_LEN 8
#define MAX_QUEUE_SIZE 18
#define NUM_RETRIES 3

#define MASTER_WAITING 0
#define MASTER_SENT 1
#define MASTER_ACK 2
#define CLIENT_WAITING 0
#define CLIENT_SENT 1

int readPacket(Stream s, char *out_message);
int sendPacket(Stream s, char *message);

class KSerialMaster {
  public:
    KSerialMaster(Stream &port);
    int sendData(uint8_t client_id, char *data);
    int getData(char *buffer);
    int doSerial();
    int getClients(uint8_t *clients);

  private:
    Stream    _stream;
    uint8_t   _state;
    char     *_in_messages[MAX_QUEUE_SIZE];
    int       _num_in_messages;
    char     *_out_messages[MAX_QUEUE_SIZE];
    int       _num_out_messages;
    uint8_t   _num_clients;
    uint8_t   _clients[MAX_CLIENTS];
};

class KSerialClient {
  public:
    KSerialClient(Stream &port, uint8_t client_number);
    int sendData(char *data);
    int getData(char *buffer);
    int doSerial();

  private:
    Stream    _stream;
    uint8_t   _state;
    char     *_in_messages[MAX_QUEUE_SIZE];
    int       _num_in_messages;
    char     *_out_messages[MAX_QUEUE_SIZE];
    int       _num_out_messages;
    uint8_t   _client_number;
};

/*
Addresses:      0: Master
            1-127: Clients
Bytes greater than 127 are control packets.

Packet: {START}{ADDRESS}{MESSAGE}{PARITY}{END}

Decode Method:
  Read starting at a start bit, ending at an end bit.
  A start bit after a start bit before an end bit restarts the message.
  An end bit after an end bit before a start bit is ignored.
  Once you have received a full packet, check parity, 

When master sees corrupted packet, if data, send NAK.
When client sees corrupted packet, it just ignores it.

Master FSM:
WAITING:
  Anything => {Null, WAITING}
SENT_READ:
  ACK => {Null, WAITING}
  DATA => {ACK, ACK_WAITING}
  TIMEOUT => {READ, SENT_READ}
  BAD => {NAK, SENT_READ}
ACK_WAITING:
  ACK => {Null, WAITING}
  TIMEOUT => {ACK, ACK_WAITING}
  BAD => {NAK, ACK_WAITING}

Client FSM:
ALL TIMES:
  BAD => {Null, CURR_STATE}
  NAK => {Last Packet, CURR_STATE}
WAITING:
  READ => {DATA, SENT_DATA}
  WRITE/DATA => {ACK, WAITING}
SENT_DATA:
  ACK => {ACK, WAITING}

Client -> Master:
  1 M: {READ}
  2 C: {DATA}
  3 M: {ACK}
  4 C: {ACK}

Client -> Master (If no data):
  1 M: {READ}
  2 C: {ACK}

Master -> Client:
  1 M: {WRITE}{DATA}
  2 C: {ACK}
*/