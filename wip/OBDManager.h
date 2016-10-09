#include "OBDMessage.h"

class OBDManager {
  public:
    typedef void (*sendCANMessageCallback)(const CANMessage message);

    OBDManager(sendCANMessageCallback callback = nullptr)
      : _sendCallback(callback) {
    }
    OBDMessage *receiveCANMessage(const CANMessage &message);

  private:
    void sendFlowControl(uint32_t recipient);

    std::vector<OBDMessage> _messages;
    sendCANMessageCallback _sendCallback;
};

