#pragma once
#include "stdint.h"
#include <deque>
#include <iostream>

struct CANMessage
{
   uint32_t id;
   uint8_t  size;
   bool     extended;
   bool     rtr;
   uint8_t  len;
   uint8_t  data[8];

   CANMessage()
     : id { 0 },
       size { sizeof(CANMessage) },
       extended { false },
       rtr { false },
       len { 0 },
       data { 0 }
   {
   }

   /* Test interface */
   template <int N> CANMessage(uint32_t id, const int (&data)[N]) : CANMessage() {
     this->id = id;
     this->len = N;
     for (int i = 0; i < 8; i++) {
       this->data[i] = N > i ? (uint8_t)data[i] : 0;
     }
   }
   bool operator==(const CANMessage &other) const {
     bool result = id == other.id &&
       size == other.size &&
       extended == other.extended &&
       rtr == other.rtr &&
       len == other.len;

     for (int i = 0; i < 8; i++) {
       result = result && data[i] == other.data[i];
     }

     return result;
   }
};

// For CANMessage toString()
std::ostream& operator << (std::ostream &os, CANMessage const &value);

class CANChannel {
public:
  /* Public interface */
  bool receive(CANMessage &message);
  void transmit(const CANMessage &message);

  /* Test interface */
  void addRx(const CANMessage &message);
  bool getTx(CANMessage &message);
private:
  using QueueT = std::deque<CANMessage>;
  void addToQueue(QueueT &queue, const CANMessage &message);
  bool removeFromQueue(QueueT &queue, CANMessage &message);
  QueueT rxQueue;
  QueueT txQueue;
};
