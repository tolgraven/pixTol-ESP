#include "strip.h"

int Strip::getPixelIndex(int pixel) { // XXX also handle matrix back-and-forth setups etc
    if(beFlipped) pixel = ledsInData-1 - pixel;
    int idx = pixel;
    if(beFolded) { // pixelidx differs from pixel recieved
      if(pixel % 2) idx = pixel/2;  // since every other pixel is from opposite end
      else          idx = ledsInData-1 - pixel/2;
    }
    return idx;
    // XXX remember mirror must be handled at later stage
}

int Strip::getPixelFromIndex(int idx) {
    int pixel = idx;
    if(beFolded) {
      if(idx % 2) pixel = idx*2;
      else        pixel = ledsInData-1 - idx*2;
    }
    if(beFlipped) pixel = ledsInData-1 - idx;
    return pixel;
    // XXX above aint fixed
}

