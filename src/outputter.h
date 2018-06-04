#pragma once

#include <Arduino.h>
#include "renderstage.h"
// #include "modulator.h"

// should carry overlapping routines between various forms of output
class Outputter: public RenderStage {
  public:
    Outputter() {}
    Outputter(uint16_t bufferSize); // raw bytes = field is 1
    Outputter(uint8_t fieldSize, uint16_t fieldCount);
    virtual bool canEmit() = 0; // always check before trying to emit...
    virtual void emit(uint8_t* data, uint16_t length) = 0;

  private:
    virtual uint16_t getIndexOfField(uint16_t position) = 0; //{ return position; }
    virtual uint16_t getFieldOfIndex(uint16_t field) = 0; // return field; }
    // uint8_t* data;
    uint16_t size;
    uint16_t outputRate; // microseconds per byte or something? +
    uint16_t hz = 40;

};

