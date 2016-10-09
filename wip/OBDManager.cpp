#include "OBDManager.h"

OBDMessage *OBDManager::receiveCANMessage(const CANMessage &message) {
  auto it = std::find_if(_messages.begin(), _messages.end(), [this, &message](const OBDMessage &m) {
    return m.id() == message.id;
  });

  if(it == _messages.end()) {
    // add a new message
    _messages.emplace_back(OBDMessage(message.id));
    it = --_messages.end();
  }

  if(it->addMessageData(message)) {
    sendFlowControl(message.id);
  }

  if(it->complete()) {
    return &(*it);
  } else {
    return nullptr;
  }
}

void OBDManager::sendFlowControl(uint32_t recipient) {
  uint32_t sender = recipient - 8;

  CANMessage flowControl;
  flowControl.id = sender;
  flowControl.len = 8;
  flowControl.data[0] = 0x30;

  if(_sendCallback) {
    _sendCallback(flowControl);
  }
}

