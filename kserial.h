#pragma once
#include "Arduino.h"

#define ACK 0x86
#define NAK 0x95
#define START 0x82
#define END 0x83
#define WRITE 0x57
#define READ 0x52
#define NO_DATA 0xB0
#define SET_STRIKE 0xB1
#define SET_SOLVE 0xB2
#define STRIKES 0xB3
#define RESET 0xB4
#define BATTERIES 0xB5
#define INDICATORS 0xB6
#define PORTS 0xB7

#define IS_CONTROL(x) ((x) & 0x80)
#define PARITY1(a) (START ^ (a) ^ END)
#define PARITY2(a, b) (START ^ (a) ^ (b) ^ END)
#define PARITY3(a, b, c) (START ^ (a) ^ (b) ^ (c) ^ END)
#define CODE_TO_STR(c) (char[2]){(char)(c), '\0'}

#define TIMEOUT 50
#define MAX_CLIENTS 16
#define MAX_MSG_LEN 7
#define NUM_RETRIES 3

class KSerialMaster {
  public:
    KSerialMaster(Stream &port);
    int getClients(uint8_t *clients);
    int sendReset(uint8_t client);
    int broadcastReset();
    int broadcastStrikes(uint8_t strikes);
    int broadcastBatteries(uint8_t batteries);
    int broadcastIndicators(uint8_t indicators);
    int broadcastPorts(uint8_t ports);
    int get_strikes(uint8_t *);
    int get_solved(uint8_t *);
    int pollClients();

  private:
    Stream    _stream;
    uint8_t   _num_clients;
    uint8_t   _clients[MAX_CLIENTS];
    uint8_t   _striked_clients[MAX_CLIENTS];
    uint8_t   _solved_clients[MAX_CLIENTS];
    int       _master_to_client(uint8_t client, char *data);
    int       _client_to_master(uint8_t client, char *data);
    int       _broadcast_data(char * data);
};

class KSerialClient {
  public:
    KSerialClient(Stream &port, uint8_t client_number);
    int sendStrike();
    int sendSolve();
    int dataAvailable();
    int get_batteries();
    int get_indicators();
    int get_ports();
    int get_reset();

  private:
    Stream    _stream;
    uint8_t   _client_number;
    uint8_t   _solve_waiting;
    uint8_t   _strike_waiting;
    uint8_t   _state;
    uint8_t   _batteries;
    uint8_t   _indicators;
    uint8_t   _ports;
    int       _respond_to_master();
};

/*
Client -> Master:
- Master sends "{START}{ADDRESS}R{PARITY}{END}"
- Client sends "{START}{DATA}{PARITY}{END}"
- Master sends "{START}{ACK/NAK}{PARITY}{END}"
- If NAK, repeat steps 2 and 3.

Master -> Client:
- Master sends "{START}{ADDRESS}W{DATA}{PARITY}{END}"
- Client sends "{START}{ACK/NAK}{PARITY}{END}"
- If NAK, repeat steps 1 and 2.

defenitions format: (Control characters: 0b1xxxxxxx)
- {START}: 0x82
- {END}: 0x83
- {ACK}: 0x86
- {NAK}: 0x95
- {DATA}: 0xB0              (No Data)
        | 0xB1              (Strike from client)
        | 0xB2              (Solve from client)
        | 0xB3{NUM_STRIKES} (Broadcast numstrikes)
        | 0xB4              (Reset client)
        | 0xB5{BATTERIES}   (Number of batteries)
        | 0xB6{INDICATORS}  (Number of batteries)
        | 0xB7{PORTS}       (Number of batteries)
*/
