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

int readCodes(String unused = "");
void publishCodes();
void processSerial();
void processReadingCodes();
void publishCodes();

Carloop<CarloopRevision2> carloop;

CodeReader reader;

void setup() {
  Serial.begin(9600);
  carloop.begin();
  reader.begin(carloop.can());

  Particle.function("readCodes", readCodes);
}

int readCodes(String unused) {
  Serial.println("Reading codes...");
  Particle.publish("codes/start", PRIVATE);
  reader.start();
  return 0;
}

void loop() {
  processSerial();
  processReadingCodes();
}

void processSerial() {
  if (Serial.read() == 'r') {
    readCodes();
  }
}

void processReadingCodes() {
  reader.process();

  static bool previouslyDone = true;
  bool done = reader.done();
  if (done && !previouslyDone) {
    publishCodes();
  }
  previouslyDone = done;
}

void publishCodes() {
  if (reader.getError()) {
    Serial.println("Error while reading codes. Is Carloop connected to a car with the ignition on?");
    Particle.publish("codes/error", PRIVATE);
    return;
  }

  auto &codes = reader.getCodes();

  if (codes.empty()) {
    Serial.println("No fault codes. Fantastic!");
  } else {
    Serial.printlnf("Read %d codes");
  }

  String result;
  for (auto it = codes.begin(); it != codes.end(); ++it) {
    auto code = *it;
    String codeStr;
    codeStr += code.letter;
    codeStr += String::format("%04X", code.code);

    Serial.print(codeStr);
    switch (code.type) {
      case DTC::STORED_DTC:
        Serial.print(" (current issue)");
        codeStr += 's';
        break;
      case DTC::PENDING_DTC:
        Serial.print(" (pending issue)");
        codeStr += 'p';
        break;
      case DTC::CLEARED_DTC:
        Serial.print(" (cleared issue)");
        codeStr += 'c';
        break;
    }
    Serial.println("");

    if (result.length() > 0) {
      result += ",";
    }
    result += codeStr;
  }

  Particle.publish("codes/result", result, PRIVATE);
}

