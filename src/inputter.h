#pragma once

#include <Arduino.h>
#include <ArtnetnodeWifi.h>
#include "renderstage.h"

// should carry overlapping routines between various forms of output
class Inputter: public RenderStage {
  public:
    Inputter() {}
    Inputter(uint16_t bufferSize); // raw bytes = field is 1
    Inputter(uint8_t fieldSize, uint16_t fieldCount);
    virtual bool canRead() = 0;
    virtual bool read() = 0; // this is passed as eg ArtnetNode callback fn

  private:
    virtual uint16_t getIndexOfField(uint16_t position) = 0; //{ return position; }
    virtual uint16_t getFieldOfIndex(uint16_t field) = 0; // return field; }
};

