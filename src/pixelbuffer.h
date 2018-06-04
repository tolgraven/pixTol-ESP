#pragma once

#include <Arduino.h>
#include <NeoPixelBrightnessBus.h>
#include "buffer.h"

class PixelBuffer: public Buffer {
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataptr=nullptr); // 
    void blendWith(PixelBuffer buffer, float amount); // later add methods like overlay, darken etc
    
    // all below should actually be modulators, but do we wrap them like this for easy access? maybe
    // void rotate(int x, int y=0);
    // void shift(int x, int y=0);
    // void adjustHue(int degrees);
    // void adjustSaturation(int amount);
    // void adjustLightness(int amount); //makes sense?
    // void setGain(float amount=1.0f); // like brightness but at buffer (not strip) level, plus can go higher than full, to overdrive pixels...
    // void fill(ColorObject color);
    // void setGeometry(int16_t* howeveryoudoit);
    //
    PixelBuffer operator+(const PixelBuffer& b);
    PixelBuffer operator-(const PixelBuffer& b); // darken?
    PixelBuffer operator*(const PixelBuffer& b); // average?
    PixelBuffer operator/(const PixelBuffer& b); // 

  private:
};

