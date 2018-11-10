#pragma once
#include "inputter.h"

// use for on-chip generated stuff. StatusLed/Ring breathing, strip standby and simple animations...
class LocalAnimation: public Inputter {
  LocalAnimation(const String& name, uint8_t bitDepth, uint16_t pixels):
    Inputter(name, bitDepth, pixels) {

    }
};

//some easing methods to implement?
//linear
// inQuad outQuad inOutQuad
// inCubic outCubic inOutCubic
// inQuart outQuart inOutQuart
// inQuint outQuint inOutQuint
// inSine outSine inOutSine
// inExpo outExpo inOutExpo
// inCirc outCirc inOutCirc
// inElastic outElastic inOutElastic
// inBack outBack inOutBack
// inBounce outBounce inOutBounce
