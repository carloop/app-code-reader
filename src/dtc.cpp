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
#include "dtc.h"

// Uncomment DEBUG_DTC to get debug info on Serial while reading codes
// #define DEBUG_DTC

#ifdef DEBUG_DTC
  #ifdef UNIT_TEST
    #include "stdio.h"
    #define DEBUG_PRINT(format, ...) printf(format "\n", ##__VA_ARGS__)
  #else
    #define DEBUG_PRINT(format, ...) Serial.printlnf(format, ##__VA_ARGS__)
  #endif
#else
  #define DEBUG_PRINT(...)
#endif

DTC::DTC(DTC::Type type, uint16_t code) {
  this->type = type;
  // The first 2 bits identify the letter of the fault code
  uint8_t letterCode = code >> 14;
  switch(letterCode) {
    case 0: this->letter = 'P'; break;
    case 1: this->letter = 'C'; break;
    case 2: this->letter = 'B'; break;
    case 3: this->letter = 'U'; break;
  }
  // The remaining 14 bits are the code number (always printed in hex)
  this->code = code & 0x3FFF;
}

void CodeReader::begin(CANChannel &channel, CodeReader::TimeT timeout) {
  DEBUG_PRINT("Initializing code reader");
  this->timeout = timeout;
  can = &channel;
}

void CodeReader::start() {
  if (state == IDLE) {
    DEBUG_PRINT("Start reading codes");
    state = START_READING_CODES;
  }
}

// State machine to read codes
// - Wait for the command to start
// - For each type of code (stored, pending, cleared)
//   - Send the read code command
//   - Wait for the full response (may by in multiple CAN frames)
//   - Store the codes
// - If there is a timeout, stop with an error
void CodeReader::process() {
  bool done;
  switch (state) {
    case START_READING_CODES:
      codes.clear();
      error = false;
      codeTypeBeingRead = DTC::STORED_DTC;
      readingCodesStart = millis();
      state = READ_CODE;
      break;

    case READ_CODE:
      DEBUG_PRINT("Reading codes with service %02X", obdServiceForDTCType(codeTypeBeingRead));
      transmitReadCodes(obdServiceForDTCType(codeTypeBeingRead));
      state = WAITING_FOR_CODES;
      break;

    case WAITING_FOR_CODES:
      if (millis() - readingCodesStart > timeout) {
        DEBUG_PRINT("Timeout while reading codes");
        state = IDLE;
        error = true;
        break;
      }

      done = receiveReadCodes();
      if (done) {
        codeTypeBeingRead = (DTC::Type)((int)codeTypeBeingRead + 1);
        if (codeTypeBeingRead >= DTC::NUM_TYPES) {
          state = IDLE;
        } else {
          state = READ_CODE;
        }
      }
      break;
    default:
      state = IDLE;
      break;
  }
}

// Map from our internal numbering of codes to OBD service numbers
uint8_t CodeReader::obdServiceForDTCType(DTC::Type type) {
  switch(type) {
    case DTC::STORED_DTC:  return OBD_SERVICE_SHOW_STORED_DTCS;
    case DTC::PENDING_DTC: return OBD_SERVICE_SHOW_PENDING_DTCS;
    case DTC::CLEARED_DTC: return OBD_SERVICE_SHOW_CLEARED_DTCS;
    default:               return 0;
  }
}

// Read trouble codes from all ECUs:
// CAN 7DF: 01 03 00 00 00 00 00 00
void CodeReader::transmitReadCodes(uint8_t service) {
  if (!can) {
    return;
  }

  CANMessage message;
  message.id = OBD_BROADCAST_ID;
  message.len = 8;
  message.data[0] = 1;
  message.data[1] = service;
  can->transmit(message);

  // Get our OBD message ready to receive the response
  obd.clear();
}

bool CodeReader::receiveReadCodes() {
  if (!can) {
    return false;
  }

  CANMessage message;
  if (can->receive(message)) {
    // if the received CAN message is from the ECU we expect

    if (message.id == OBD_FIRST_ECU_RESPONSE) {

      DEBUG_PRINT("Got response %03x: %02x %02x %02x %02x %02x %02x %02x %02x", message.id, message.data[0], message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6], message.data[7]);

      // Add the data to our OBD message
      bool needsFlowControl = obd.addMessageData(message);

      if (needsFlowControl) {
        DEBUG_PRINT("Sending flow control");
        can->transmit(obd.flowControlMessage());
      }

      // Parse code when we have all the data
      if (obd.complete() && parseCodes()) {
        return true;
      }
    }
  }

  return false;
}

// Turn an array of data bytes into an array of diagnostic trouble codes
bool CodeReader::parseCodes() {
  auto it = obd.data().begin(); 
  auto end = obd.data().end();
  
  // First byte is the response code
  // A postive response is 0x40 larger than the request
  // For a request for reading trouble codes 0x03 has positive response 0x43
  // Negative responses are 0x80 and above
  if (it != end) {
    uint8_t response = *it;
    ++it;

    DEBUG_PRINT("Response code %02x", response);
    if (response != obdServiceForDTCType(codeTypeBeingRead) + OBD_SERVICE_RESPONSE_OFFSET) {
      // negative response, or response for another service
      // restart wait
      obd.clear();
      return false;
    }
  }

  // Second byte is the number of codes.
  // We don't need to store it
  if (it != end) {
    DEBUG_PRINT("Number of codes %02x", *it);
    ++it;
  }

  // Each trouble code is 2 bytes
  // Join pairs of data bytes and transform into DTC
  while (it != end) {
    uint8_t msb = *it;
    ++it;
    if (it != end) {
      uint8_t lsb = *it;
      ++it;

      DTC code(codeTypeBeingRead, ((uint16_t)msb << 8) | lsb);
      codes.push_back(code);
      DEBUG_PRINT("Code %04x", code.code);
    }
  }
  return true;
}


void CodeClearer::begin(CANChannel &channel, CodeClearer::TimeT timeout) {
  this->timeout = timeout;
  can = &channel;
}

void CodeClearer::start() {
  if (state == IDLE) {
    DEBUG_PRINT("Start clearing codes");
    state = CLEAR_CODES;
  }
}

// State machine to clear codes
// - Wait for the command to start
// - Send the clear code command
// - Wait for the full response (should alwasy be 1 CAN frame)
// - If there is a timeout, stop with an error
void CodeClearer::process() {
  bool done;
  switch (state) {
    case CLEAR_CODES:
      DEBUG_PRINT("Clearing codes");
      error = false;
      clearingCodesStart = millis();
      transmitClearCodes();
      state = WAITING_FOR_CLEAR;
      break;

    case WAITING_FOR_CLEAR:
      if (millis() - clearingCodesStart > timeout) {
        DEBUG_PRINT("Timeout while clearing codes");
        state = IDLE;
        error = true;
        break;
      }

      done = receiveClearCodes();
      if (done) {
        state = IDLE;
      }
      break;
    default:
      state = IDLE;
      break;
  }
}

// Clear codes
// CAN 7DF: 01 04 00 00 00 00 00 00
void CodeClearer::transmitClearCodes() {
  if (!can) {
    return;
  }

  CANMessage message;
  message.id = OBD_BROADCAST_ID;
  message.len = 8;
  message.data[0] = 1;
  message.data[1] = OBD_SERVICE_CLEAR_DTCS;

  can->transmit(message);
  obd.clear();
}

bool CodeClearer::receiveClearCodes() {
  if (!can) {
    return false;
  }

  CANMessage message;
  if (can->receive(message)) {
    // if the received CAN message is from the ECU we expect
    if (message.id == OBD_FIRST_ECU_RESPONSE) {
      DEBUG_PRINT("Got response %03x: %02x %02x %02x %02x %02x %02x %02x %02x", message.id, message.data[0], message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6], message.data[7]);

      // Add the data to our OBD message
      bool needsFlowControl = obd.addMessageData(message);

      if (needsFlowControl) {
        DEBUG_PRINT("Sending flow control");
        can->transmit(obd.flowControlMessage());
      }

      // When we have all the data, don't bother checking the response
      // and always report success
      if (obd.complete()) {
        return true;
      }
    }
  }

  return false;
}
