/* Fault code reader for Carloop
 *
 * Reads OBD diagnostic trouble codes (DTC) at 500 kbit and outputs them
 * to the USB serial port and as Particle events.
 *
 * Send any key to the serial port to start reading codes, using screen
 * on Mac OSX and Linux, or PuTTY on Windows:
 * screen /dev/tty.usbmodem1411 (update for your port number)
 *
 * To read codes through the network run these 2 commands in different
 * terminals:
 * particle subscribe mine
 * particle call my_carloop readCodes
 *
 * Copyright 2016 Julien Vanier
 *
 * Distributed under the MIT license. See LICENSE.txt for more details.
 */

#include "application.h"
#include "carloop/carloop.h"
#include "dtc.h"

SYSTEM_THREAD(ENABLED);

Carloop<CarloopRevision2> carloop;

void transmitReadCodes(uint8_t service);
bool receiveReadCodes();

#define CODE_READ_DURATION 1000

struct DTC {
  enum Type {
    STORED_DTC,
    PENDING_DTC,
    CLEARED_DTC,

    NUM_TYPES
  } type;
  uint16_t code;
};

std::vector<DTC> codes;

enum StatesE {
  IDLE,
  START_READING_CODES,
  READ_CODE,
  WAITING_FOR_CODES,
} state;

enum DTC::Type codeTypeBeingRead;

unsigned long readingCodesStart;

int readCodes(String unused = "") {
  state = START_READING_CODES;
  return 0;
}

void setup() {
  Serial.begin(9600);
  carloop.begin();

  Particle.function("readCodes", readCodes);
}

void processSerial() {
  if (Serial.read() == 'r') {
    readCodes();
  }
}

void processReadingCodes() {
  bool done;
  switch (state) {
    case START_READING_CODES:
      codes.clear();
      codeTypeBeingRead = DTC::OBD_SERVICE_SHOW_STORED_DTCS;
      readingCodesStart = millis();
      state = READ_CODE;
      break;

    case READ_CODE:
      transmitReadCodes(codeTypeBeingRead);
      state = WAIT_FOR_RESPONSE;
      break;

    case WAIT_FOR_RESPONSE:
      done = receiveReadCodes();
  }


  //  receiveReadCodes();

  //  if(millis() - readingCodesStart > CODE_READ_DURATION) {
  //    readingCodes = false;
  //  }
  //}

  //readingCodesOld = readingCodes;

}

void loop() {
  processSerial();
  processReadingCodes();
}

// Broadcast to all OBD computers a request to read codes
void transmitReadCodes(uint8_t service) {
  CANMessage message;
  message.id = OBD_BROADCAST_ID;
  message.len = 8;
  message.data[0] = 1;
  message.data[1] = service;
  carloop.can().transmit(message);
}

bool receiveReadCodes() {
  CANMessage message;
  while(carloop.can().receive(message)) {
    if(message.id >= OBD_FIRST_ECU_RESPONSE &&
        message.id <= OBD_LAST_ECU_RESPONSE) {

    }
  }
}
