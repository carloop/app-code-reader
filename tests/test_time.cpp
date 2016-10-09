#include "test_time.h"

unsigned long testMillis = 0;
unsigned long millis() {
  return testMillis;
}

void timeTravel(unsigned long newMillis) {
  testMillis = newMillis;
}
