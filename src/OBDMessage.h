#pragma once
#ifdef UNIT_TEST
#include "../tests/application_test.h"
#else
#include "application.h"
#endif
#include <vector>

class OBDMessage {
public:
  using DataT = std::vector<uint8_t>;

  uint32_t id() const {
    return _id;
  }

  uint16_t size() const {
    return _size;
  }

  uint32_t complete() const {
    return _complete;
  }

  const DataT &data() const {
    return _data;
  }

  void clear();

  bool addMessageData(const CANMessage &message);

  CANMessage flowControlMessage();
    
private:
  enum MESSAGE_TYPE {
    SINGLE,
    FIRST,
    CONSECUTIVE,
    FLOW
  };

  MESSAGE_TYPE messageType(const CANMessage &message);
  void addDataFrom(uint8_t i, const uint8_t *data);

  uint32_t _id;
  uint16_t _size;
  bool _complete;
  DataT _data;
};
