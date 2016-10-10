/* Run these tests on a laptop
 */
#include "catch.hpp"
#define DEBUG_DTC
#include "../src/dtc.h"
#include <iomanip>
using namespace std;


// For DTC toString()
std::ostream& operator << (std::ostream &os, DTC const &value) {
  os << "DTC(";
  switch (value.type) {
    case DTC::STORED_DTC: os << "STORED_DTC"; break;
    case DTC::PENDING_DTC: os << "PENDING_DTC"; break;
    case DTC::CLEARED_DTC: os << "CLEARED_DTC"; break;
    default: os << "?"; break;
  }
  os << ", ";
  os << value.letter;
  os << std::hex << std::setfill('0') << std::setw(4) << value.code << ")";
  return os;
}

TEST_CASE("CodeReader read codes") {
  CodeReader reader = CodeReader();
  CANChannel can;
  CANMessage tx;

  reader.begin(can);
  reader.start();

  // Sets up variables for reading
  reader.process();

  // Sends request for stored DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get one code: P0415
  can.addRx(CANMessage(0x7e8, { 0x04, 0x43, 0x01, 0x04, 0x15, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  auto &codes = reader.getCodes();
  REQUIRE(codes.size() == 1);
  auto codesIter = codes.begin();
  REQUIRE(*codesIter == DTC(DTC::STORED_DTC, 'P', 0x415));

  // No other messages transmitted meanwhile
  REQUIRE(can.getTx(tx) == false);

  // Sends request for pending DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get one code:P0103
  can.addRx(CANMessage(0x7e8, { 0x04, 0x47, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  REQUIRE(codes.size() == 2);
  codesIter = codes.begin();
  ++codesIter; // skip past 1st code
  REQUIRE(*codesIter == DTC(DTC::PENDING_DTC, 'P', 0x103));

  // No other messages transmitted meanwhile
  REQUIRE(can.getTx(tx) == false);

  // Sends request for pending DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get one code:C0001
  can.addRx(CANMessage(0x7e8, { 0x04, 0x4A, 0x01, 0x50, 0x01, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  REQUIRE(codes.size() == 3);
  codesIter = codes.begin();
  ++codesIter; // skip past 1st code
  ++codesIter; // skip past 2nd code
  REQUIRE(*codesIter == DTC(DTC::CLEARED_DTC, 'C', 0x1001));

  // We are done
  REQUIRE(reader.done() == true);
  REQUIRE(reader.getError() == false);
}

TEST_CASE("CodeReader read multiple codes") {
  CodeReader reader = CodeReader();
  CANChannel can;
  CANMessage tx;

  reader.begin(can);
  reader.start();

  // Sets up variables for reading
  reader.process();

  // Sends request for stored DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get three code: P0415, P1001, P0103
  can.addRx(CANMessage(0x7e8, { 0x10, 0x08, 0x43, 0x03, 0x04, 0x15, 0x10, 0x01 }));
  reader.process();

  // Expect flow control
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7E0, { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Send rest of codes
  can.addRx(CANMessage(0x7e8, { 0x21, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  auto &codes = reader.getCodes();
  REQUIRE(codes.size() == 3);
  auto codesIter = codes.begin();
  REQUIRE(*codesIter == DTC(DTC::STORED_DTC, 'P', 0x415));
  ++codesIter;
  REQUIRE(*codesIter == DTC(DTC::STORED_DTC, 'P', 0x1001));
  ++codesIter;
  REQUIRE(*codesIter == DTC(DTC::STORED_DTC, 'P', 0x103));

  // No other messages transmitted meanwhile
  REQUIRE(can.getTx(tx) == false);

  // Sends request for pending DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get no codes
  can.addRx(CANMessage(0x7e8, { 0x02, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  REQUIRE(codes.size() == 3);

  // No other messages transmitted meanwhile
  REQUIRE(can.getTx(tx) == false);

  // Sends request for pending DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get no code
  can.addRx(CANMessage(0x7e8, { 0x02, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
  reader.process();

  // Verify DTC was stored properly
  REQUIRE(codes.size() == 3);

  // We are done
  REQUIRE(reader.done() == true);
  REQUIRE(reader.getError() == false);
}

TEST_CASE("CodeReader timeout") {
  CodeReader reader = CodeReader();
  CANChannel can;
  CANMessage tx;

  timeTravel(0);

  reader.begin(can);
  reader.start();

  // Sets up variables for reading
  reader.process();

  // Sends request for stored DTC
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Timeout while waiting for response
  timeTravel(1100 /* ms */);

  // Process timeout
  reader.process();

  // We are done
  REQUIRE(reader.done() == true);
  REQUIRE(reader.getError() == true);
}

TEST_CASE("CodeClearer clear codes") {
  CodeClearer clearer = CodeClearer();
  CANChannel can;
  CANMessage tx;

  clearer.begin(can);
  clearer.start();

  // Sends request to clear codes
  clearer.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  clearer.process(); clearer.process(); clearer.process();

  // Acknowledge
  can.addRx(CANMessage(0x7e8, { 0x01, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
  clearer.process();

  // We are done
  REQUIRE(clearer.done() == true);
  REQUIRE(clearer.getError() == false);
}

TEST_CASE("CodeClearer timeout") {
  CodeClearer clearer = CodeClearer();
  CANChannel can;
  CANMessage tx;

  timeTravel(0);

  clearer.begin(can);
  clearer.start();

  // Sends request to clear codes
  clearer.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Timeout while waiting for response
  timeTravel(1100 /* ms */);

  // Process timeout
  clearer.process();

  // We are done
  REQUIRE(clearer.done() == true);
  REQUIRE(clearer.getError() == true);
}
