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
  enum Type : uint8_t {
    STORED_DTC,
    PENDING_DTC,
    CLEARED_DTC,

    NUM_TYPES
  } type;
  char letter;
  uint16_t code;

  DTC(Type type, char letter, uint16_t code)
    : type(type), letter(letter), code(code) {
  }
  DTC(Type type, uint16_t code);
  bool operator==(const DTC &other) const {
    return type == other.type && letter == other.letter && code == other.code;
  }
};

#define OBD_BROADCAST_ID 0x7DF
#define OBD_FIRST_ECU_RESPONSE 0x7E8
#define OBD_LAST_ECU_RESPONSE 0x7EF

#define OBD_SERVICE_SHOW_STORED_DTCS 0x03
#define OBD_SERVICE_SHOW_PENDING_DTCS 0x07
#define OBD_SERVICE_SHOW_CLEARED_DTCS 0x0A

#define OBD_SERVICE_RESPONSE_OFFSET 0x40

class CodeReader {
public:
  using CodesT = std::vector<DTC>;
  using TimeT = unsigned long;

  enum StatesE {
    IDLE,
    START_READING_CODES,
    READ_CODE,
    WAITING_FOR_CODES,
  };

  static const TimeT defaultTimeout = 500;

  CodeReader();

  // Call in setup()
  void begin(CANChannel &channel);

  // Call in loop()
  void process();

  void start();
  bool done() {
    return state == IDLE;
  }

  StatesE getState() const {
    return state;
  }

  const CodesT &getCodes() const {
    return codes;
  }

  bool getError() const {
    return error;
  }

  void setTimeout(TimeT timeout) {
    this->timeout = timeout;
  }

private:
  uint8_t obdServiceForDTCType(DTC::Type type);
  void transmitReadCodes(uint8_t service);
  bool receiveReadCodes();
  bool parseCodes();

  CANChannel *can;
  CodesT codes;
  OBDMessage obd;
  StatesE state;
  DTC::Type codeTypeBeingRead;
  TimeT readingCodesStart;
  TimeT timeout;
  bool error;
};
