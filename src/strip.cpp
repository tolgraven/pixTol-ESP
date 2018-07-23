#include "strip.h"

// uint16_t Strip::getIndexOfField(uint16_t position) { // XXX also handle matrix back-and-forth setups etc
//     if(beFlipped) position = ledsInData-1 - position;
//     int idx = position;
//     if(beFolded) { // pixelidx differs from position recieved
//       if(position % 2) idx = position/2;  // since every other position is from opposite end
//       else          idx = ledsInData-1 - position/2;
//     }
//     return idx;
//     // XXX remember mirror must be handled at later stage
// }
//
// uint16_t Strip::getFieldOfIndex(uint16_t field) {
//     int pixel = field;
//     if(beFolded) {
//       if(field % 2) pixel = field*2;
//       else        pixel = ledsInData-1 - field*2;
//     }
//     if(beFlipped) pixel = ledsInData-1 - field;
//     return pixel;
//     // XXX above aint fixed
// }
//
