#include "OBDMessage.h"
#include <algorithm>

#define OBD_REQUEST_RESPONSE_OFFSET 8

bool OBDMessage::addMessageData(const CANMessage &message) {
  _id = message.id;

  bool needsFlowControl = false;
  switch(messageType(message)) {
    case SINGLE:
      clear();
      _size = message.data[0] & 0xF;
      addDataFrom(1, message.data);
      break;
    case FIRST:
      clear();
      _size = ((uint16_t)(message.data[0] & 0x0F) << 8) | message.data[1];
      addDataFrom(2, message.data);
      needsFlowControl = true;
      break;
    case CONSECUTIVE:
      addDataFrom(1, message.data);
      break;
  }

  _complete = _data.size() == _size;
  return needsFlowControl;
}

void OBDMessage::addDataFrom(uint8_t i, const uint8_t *data) {
  for(; _data.size() < _size && i < 8; i++) {
    _data.push_back(data[i]);
  }
}

OBDMessage::MESSAGE_TYPE OBDMessage::messageType(const CANMessage &message) {
  uint8_t headerByte = message.data[0];
  return (MESSAGE_TYPE)(headerByte >> 4);
}

void OBDMessage::clear() {
  // reuse the same message over and over, just clear it
  _data.clear();
  _size = 0;
  _complete = false;
}

CANMessage OBDMessage::flowControlMessage() {
  CANMessage msg;
  msg.id = _id - OBD_REQUEST_RESPONSE_OFFSET;
  msg.len = 8;
  msg.data[0] = FLOW << 4;
  return msg;
}
