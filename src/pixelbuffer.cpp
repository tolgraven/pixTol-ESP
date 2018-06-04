#include "pixelbuffer.h"

PixelBuffer::PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataptr)
    size(bytesPerPixel * numPixels),
    data(dataptr) {

}


// would these go here or do i have yet another class for renderer? nahh
    enum Functions { // or as char array? so can loop over and advertise...
      Dimmer = 1, Strobe = 2, Hue = 3,
      Attack = 4, Release = 5,
      Bleed = 6, Noise = 7,
      RotateFwd = 8, RotateBack = 9,
      DimmerAttack = 10, DimmerRelease = 11
    };
    const char* functions[12] = {
      "Dimmer", "Strobe", "Hue",
      "Attack", "Release",
      "Bleed", "Noise",
      "RotateFwd", "RotateBack",
      "DimmerAttack", "DimmerRelease", ""};
    // either way need a struct or something to keep state. or even a function class
    // so can add new ones simply... they each have an update fn to do their thing to render loop?
    // and prop to balance updator sources' priority, get frozen, run on own LFO etc...)
    // XXX array/vector of Modulators here
