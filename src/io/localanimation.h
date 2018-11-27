#pragma once

#include <functional>
#include "renderstage.h"

// use for on-chip generated stuff. StatusLed/Ring breathing, strip standby and simple animations...
class LocalAnimation: public Generator {
  LocalAnimation(const String& name, uint8_t bitDepth, uint16_t pixels):
    Generator(name, bitDepth, pixels) {
    }
};


