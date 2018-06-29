/** @file KTANECommon.h
 *  @brief Headers and definitions for common KTANE functionality
 *
 *  @author Dillon Lareau (dlareau)
 */

#pragma once
#include "Arduino.h"
#include "DSerial.h"

// Serial number tools:
#define IS_ODD(x) ((x) & 1)
#define IS_EVEN(x) (!((x) & 1))
#define IS_VOWEL(x) ((x)=='A' || (x)=='E' || (x)=='I' || (x)=='O' || (x)=='U')
#define IS_NUMBER(x) ((x) >= '0' && (x) <= '9')
#define IS_LETTER(x) ((x) >= 'A' && (x) <= 'Z')

// Data prefix codes:
#define STRIKE (char)0xC0
#define SOLVE (char)0xC1
#define CONFIG (char)0xC2

typedef struct raw_config_st {
  // Byte 0
  unsigned int spacer1: 2;
  unsigned int ports : 3;
  unsigned int batteries: 3;

  // Bytes 1-5
  char serial[5]
  
  // Byte 6
  unsigned int spacer2: 3;
  unsigned int serial6: 3;
  unsigned int indicators : 2;
}raw_config_t;

typedef struct config_st {
  unsigned int ports : 3;
  unsigned int batteries: 3;
  unsigned int indicators : 2;
  char         serial[7];
}config_t;

void config_to_raw(raw_config_t *raw_config, config_t *config_t);
void raw_to_config(raw_config_t *raw_config, config_t *config_t);

class KTANEModule {
  public:
    KTANEModule(DSerialClient &dserial);
    void ModuleInterpretData();
    int sendStrike();
    int sendSolve();
    config_t *getConfig();
    int getLitFRK();
    int getLitCAR();
    int getNumBatteries();
    int getParallelPort();
    int getRCAPort();
    int getRJ45Port();
    char getSerialDigit(int index);
    int serialContains(char c);
  private:
    DSerialClient &_dserial;
    config_t _config;
};

class KTANEController {
  public:
    KTANEController(DSerialMaster &dserial);
    void ControllerInterpretData();
    int sendConfig(config_t *config);
    int getStrikes();
    int getSolves();

  private:
    DSerialMaster &_dserial;
    uint8_t _strikes[MAX_CLIENTS];
    uint8_t _solves[MAX_CLIENTS];
};
