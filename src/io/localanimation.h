#pragma once
#include "inputter.h"

// use for on-chip generated stuff. StatusLed/Ring breathing, strip standby and simple animations...
class LocalAnimation: public Inputter {
  LocalAnimation(const String& name, uint8_t bitDepth, uint16_t pixels):
    Inputter(name, bitDepth, pixels) {

    }
};
