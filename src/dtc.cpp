#include "dtc.h"

DTC::DTC(DTC::Type type, uint16_t code) {
  this->type = type;
  uint8_t letterCode = code >> 14;
  switch(letterCode) {
    case 0: this->letter = 'P'; break;
    case 1: this->letter = 'C'; break;
    case 2: this->letter = 'B'; break;
    case 3: this->letter = 'U'; break;
  }
  this->code = code & 0x3FFF;
}

CodeReader::CodeReader() {
  timeout = defaultTimeout;
}

void CodeReader::begin(CANChannel &channel) {
  can = &channel;
}

void CodeReader::start() {
  if (state == IDLE) {
    state = START_READING_CODES;
  }
}

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
      transmitReadCodes(obdServiceForDTCType(codeTypeBeingRead));
      state = WAITING_FOR_CODES;
      break;

    case WAITING_FOR_CODES:
      if (millis() - readingCodesStart > timeout) {
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
  }
}

uint8_t CodeReader::obdServiceForDTCType(DTC::Type type) {
  switch(type) {
    case DTC::STORED_DTC:  return OBD_SERVICE_SHOW_STORED_DTCS;
    case DTC::PENDING_DTC: return OBD_SERVICE_SHOW_PENDING_DTCS;
    case DTC::CLEARED_DTC: return OBD_SERVICE_SHOW_CLEARED_DTCS;
    default:               return 0;
  }
}

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

  obd.clear();
}

bool CodeReader::receiveReadCodes() {
  if (!can) {
    return false;
  }

  CANMessage message;
  if (can->receive(message)) {
    if (message.id == OBD_FIRST_ECU_RESPONSE) {
      bool needsFlowControl = obd.addMessageData(message);

      if (needsFlowControl) {
        can->transmit(obd.flowControlMessage());
      }

      if (obd.complete() && parseCodes()) {
        return true;
      }
    }
  }

  return false;
}

bool CodeReader::parseCodes() {
  auto it = obd.data().begin(); 
  auto end = obd.data().end();
  
  // First byte is the response code
  if (it != end) {
    uint8_t response = *it;
    ++it;

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
    ++it;
  }

  // join pairs of data bytes and transform into DTC
  while (it != end) {
    uint8_t msb = *it;
    ++it;
    if (it != end) {
      uint8_t lsb = *it;
      ++it;

      DTC code(codeTypeBeingRead, ((uint16_t)msb << 8) | lsb);
      codes.push_back(code);
    }
  }
  return true;
}
