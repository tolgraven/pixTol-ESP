#include "buffer.h"


// void PixelBuffer::interpolate(uint8_t *was, uint8_t *target, uint8_t progress) { // input buffers being interpolated, weighting (0-255) of second buffer in interpolation
//   uint8_t mix;
//   uint16_t e;
//   uint16_t weight2 = (uint16_t)progress + 1; // 1-256 //use float for progress convert here instead?
//   uint16_t weight1 = 257 - weight2;    // 1-256
//   uint8_t* bufPtr = bufdata;
//
//   for(uint16_t pixel = 0; pixel < fieldCount; pixel++, bufPtr += 4) { //why we hopping 4 from adaRGB thing? extra byte from??
//     for(uint8_t field = 0; field < fieldSize; field++) {
//       mix = (*was++ * weight1 + *target++ * weight2) >> 8; //Interpolate red from was and target based on weightings
//       // fracR is the fractional portion (0-255) of the 16-bit gamma- corrected value for a given red brightness...
//       // how far 'off' a given 8-bit brightness value is from its ideal. error is carried forward to next frame in errR
//       // buffer...added to the fracR value for the current pixel...
//       e = gamma.table.fraction[mix] + gamma.table.error[pixel];
//       // ...if this exceeds 255, resulting red value is bumped up to the next brightness level and 256 is
//       // subtracted from the error term before storing back in errR.  Diffusion dithering is the result.
//       buffer[0] = (e < 256) ? gamma.table.low[mix] : gamma.table.high[mix]; // If e exceeds 256, it *should* be reduced by 256 at this point..
//       gamma.table.error[pixel] = e; //rather than subtract, rely on truncation in 8-bit store op to it this implicitly. (e & 0xFF)
//     }
//   }
// }
//
//   void PixelBuffer::tickInterpolation(uint32_t elapsed) { // scheduler should keep track of timing, just tell how long since last tick...
//     static uint32_t lastKeyFrame = 0; // absolute time (in micros) when most recent pixel frame read.
//     static uint32_t interval = 0; // interval (also in cycles) between lastKeyFrame and one before .
//
//     uint32_t t = micros();
//     uint32_t elapsed = t - lastKeyFrame; // Elapsed since target keyframe recieved
//     uint8_t progress = (elapsed >= interval)? 255:
//                        (255L * elapsed / interval); // only sneakily avoids divide by zero, clear up code...
//
//     interpolate(bufdata, bufTarget, progress);
//     updates++;
//
//     static uint32_t updates = 0, priorSeconds = 0; // used for the updates-per-second estimate.
//     uint32_t seconds;
//     // logging bit, extract from here yo
//     if((seconds = (t / MILLION)) != priorSeconds) { // 1 sec elapsed?
//       LN.logf(__func__, LoggerNode::DEBUG, "%d updates/sec", updates);
//       priorSeconds = seconds;
//       updates = 0; // Reset counter
//   }
// }
