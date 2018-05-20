#ifndef PIXTOL_PIXELBUFFER
#define PIXTOL_PIXELBUFFER

#include <Arduino.h>

class PixelBuffer {
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* data=NULL); // 
    void update(uint8_t* data, uint16_t length=512, uint16_t offset=0);
    void finalize(); // lock in keyframe
    uint8_t* get(uint16_t length=512, uint16_t offset=0);

    
    
    void rotate(int pixels);
    void shift(int pixels);


  private:
    // uint8_t data[512];
    uint8_t* data;
    int size;


};


#endif

