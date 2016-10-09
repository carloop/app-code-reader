/* Run these tests on a laptop
 */
#include "catch.hpp"
#include "../src/dtc.h"
using namespace std;

TEST_CASE("CodeReader read codes", "") {
  CodeReader reader;
  CANChannel can;
  CANMessage tx;

  reader.begin(can);
  reader.start();

  // Sets up variables for reading
  reader.process();

  // Sends first request
  reader.process();
  REQUIRE(can.getTx(tx) == true);
  REQUIRE(tx == CANMessage(0x7DF, { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

  // Wait a little bit for receive message
  reader.process(); reader.process(); reader.process();

  // Get one code: P0415
  can.addRx(CANMessage(0x7e8, { 0x03, 0x43, 0x04, 0x15, 0x00, 0x00, 0x00, 0x00 }));
  reader.process();

}

// TODO: test with flow control
// TODO: test with timeout
