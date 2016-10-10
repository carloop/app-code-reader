/* State machines for reading and clearing OBD-II diagnostic fault codes (DTC)
 *
 * There are 3 types of fault codes in a car: stored, pending and
 * cleared. Typically a fault must occur in several drive cycles before
 * being stored and turning on the malfunction indicator light (MIL).
 * When a problem is resolved or a clear codes command is sent, the
 * codes become cleared.
 *
 * Reference: https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_03
 *
 * Copyright 2016 1000 Tools, Inc
 *
 * Distributed under the MIT license. See LICENSE.txt for more details.
 */

#pragma once
#ifdef UNIT_TEST
#include "../tests/application_test.h"
#else
#include "application.h"
#endif

#include <vector>
#include <stdint.h>
#include "OBDMessage.h"

// Trouble codes look like P0103 and U0300
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

// OBD requests can be made to one specific electronic control unit (ECU)
// or broadcast to all. The ID of the CAN message with the request
// determines the recipient.
// For example, the engine control unit is adressed as 0x7E8
#define OBD_BROADCAST_ID 0x7DF
#define OBD_FIRST_ECU_RESPONSE 0x7E8
#define OBD_LAST_ECU_RESPONSE 0x7EF

// An ECU responds to a message with an ID 8 less than the request
// For example, the engine control unit responds with 0x7E0 (8 less than 0x7E8)
#define OBD_REQUEST_RESPONSE_OFFSET 8

// There are many OBD-II services that ECUs respond to.
#define OBD_SERVICE_SHOW_STORED_DTCS 0x03
#define OBD_SERVICE_CLEAR_DTCS 0x04
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

  // Call in setup()
  void begin(CANChannel &channel, TimeT timeout = defaultTimeout);

  // Call in loop()
  void process();

  // Call to start reading codes
  void start();

  // true when code reading is done
  bool done() {
    return state == IDLE;
  }

  // To check the state of the reading process in more details
  StatesE getState() const {
    return state;
  }

  // The list of codes read from the ECU
  const CodesT &getCodes() const {
    return codes;
  }

  // true if there was a timeout or other error reading codes
  bool getError() const {
    return error;
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

class CodeClearer {
public:
  using TimeT = unsigned long;

  enum StatesE {
    IDLE,
    CLEAR_CODES,
    WAITING_FOR_CLEAR,
  };

  static const TimeT defaultTimeout = 500;

  // Call in setup()
  void begin(CANChannel &channel, TimeT timeout = defaultTimeout);

  // Call in loop()
  void process();

  // Call to start clearing codes
  void start();

  // true when code clearing is done
  bool done() {
    return state == IDLE;
  }

  // To check the state of the clearing process in more details
  StatesE getState() const {
    return state;
  }

  // true if there was a timeout or other error clearing codes
  bool getError() const {
    return error;
  }

private:
  void transmitClearCodes();
  bool receiveClearCodes();

  CANChannel *can;
  OBDMessage obd;
  StatesE state;
  TimeT clearingCodesStart;
  TimeT timeout;
  bool error;
};
