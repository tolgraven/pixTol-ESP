#pragma once

#include <Arduino.h>
#include "modulator.h"

// should carry overlapping routines between various forms of output
class Outputter {
  public:
    Outputter();
    void onKeyFrame(uint8_t* data, uint16_t length);
    bool flush();

    bool addModulator(Modulator mod);
  private:
    uint8_t* data;
    int frameLength;

    // Modulator[] effects;

};

