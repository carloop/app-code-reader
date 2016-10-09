#include "test_can.h"
#include <iomanip>

std::ostream& operator << (std::ostream &os, CANMessage const &value) {
  os << "CANMessage(";
  os << std::hex << std::setfill('0') << std::setw(3) << value.id << ", ";
  os << "{";
  for (int i = 0; i < value.len; i++) {
    if (i != 0) {
      os << ", ";
    }
    os << std::hex << std::setfill('0') << std::setw(2) << (int)value.data[i];
  }
  os << "})";
  return os;
}

bool CANChannel::receive(CANMessage &message) {
  return removeFromQueue(rxQueue, message);
}

void CANChannel::transmit(const CANMessage &message) {
  addToQueue(txQueue, message);
}

void CANChannel::addRx(const CANMessage &message) {
  addToQueue(rxQueue, message);
}

bool CANChannel::getTx(CANMessage &message) {
  return removeFromQueue(txQueue, message);
}

void CANChannel::addToQueue(QueueT &queue, const CANMessage &message) {
  queue.push_back(message);
}

bool CANChannel::removeFromQueue(QueueT &queue, CANMessage &message) {
  if (queue.empty()) {
    return false;
  }

  message = queue.front();
  queue.pop_front();

  return true;
}

