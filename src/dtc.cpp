#include "dtc.h"

void CodeReader::begin(CANChannel &channel) {
  can = &channel;
}

void CodeReader::start() {
  state = START_READING_CODES;
}

void CodeReader::process() {
  bool done;
  switch (state) {
    case START_READING_CODES:
      codes.clear();
      codeTypeBeingRead = DTC::STORED_DTC;
      readingCodesStart = millis();
      state = READ_CODE;
      break;

    case READ_CODE:
      transmitReadCodes(obdServiceForDTCType(codeTypeBeingRead));
      state = WAITING_FOR_CODES;
      break;

    case WAITING_FOR_CODES:
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

      if (obd.complete()) {
        parseCodes();
        return true;
      }
    }
  }

  return false;
}

void CodeReader::parseCodes() {
}
