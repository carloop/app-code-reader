#pragma once
#ifdef UNIT_TEST
#include "../tests/application_test.h"
#else
#include "application.h"
#endif

#include <vector>
#include <stdint.h>
#include "OBDMessage.h"

struct DTC {
  enum Type {
    STORED_DTC,
    PENDING_DTC,
    CLEARED_DTC,

    NUM_TYPES
  } type;
  uint16_t code;
};

#define OBD_BROADCAST_ID 0x7DF
#define OBD_FIRST_ECU_RESPONSE 0x7E8
#define OBD_LAST_ECU_RESPONSE 0x7EF

#define OBD_SERVICE_SHOW_STORED_DTCS 0x03
#define OBD_SERVICE_SHOW_PENDING_DTCS 0x07
#define OBD_SERVICE_SHOW_CLEARED_DTCS 0x0A

class CodeReader {
public:
  using CodesT = std::vector<DTC>;

  enum StatesE {
    IDLE,
    START_READING_CODES,
    READ_CODE,
    WAITING_FOR_CODES,
  };

  // Call in setup()
  void begin(CANChannel &channel);

  // Call in loop()
  void process();

  void start();

private:
  uint8_t obdServiceForDTCType(DTC::Type type);
  void transmitReadCodes(uint8_t service);
  bool receiveReadCodes();
  void parseCodes();

  CANChannel *can;
  CodesT codes;
  OBDMessage obd;
  StatesE state;
  DTC::Type codeTypeBeingRead;
  unsigned long readingCodesStart;
};
