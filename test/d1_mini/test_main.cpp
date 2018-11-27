// compile only if in correct env/testing situation
#if defined(ARDUINO) && defined(UNIT_TEST)

#include <Arduino.h>
#include "unity.h"

// setup connects serial, runs test cases (upcoming)
void setup() {
  delay(2000);
  UNITY_BEGIN();

  // calls to tests will go here

  UNITY_END();
}

void loop() {
  // nothing to be done here.
}

#endif
