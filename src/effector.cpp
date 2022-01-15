#include "esp_attr.h"

#include "effector.h"

namespace tol {


bool IRAM_ATTR Interpolator::execute(Buffer& receiver, const Buffer& origin, const Buffer& target, float progress) {

  if(receiver._residuals == nullptr) return false; //avoid crash... tho should prob shuffle target to receiver or someting?

  uint32_t scale = std::clamp(receiver.getGain(), 0.0f, 1.0f) * 255UL + 1; // or 256?
  if(scale <= receiver.blackPoint) {
    memset(receiver.get(), 0x00, receiver.lengthBytes());
    memset(receiver._residuals, 0x00, receiver.lengthBytes());
    receiver.setDirty();
    return true;
  }

  // tho, in here I guess keep everything not reliant on overflow at u32? on 32bit arch...
  uint32_t mix24;
  uint16_t mix16, error;          // should we add a bit of resolution? since gets compounded... wait no cause cancels hah
  uint8_t mix8;                   // but if 0xFFFF * 0xFF we shift down 16 for mix16

  uint16_t targetWeight = (uint16_t)(progress * 255L) + 1, // 1-256 - why? dont we want progress 0 to be zero target influence? 1 * 255 >> 8 = 0 tho :)
           originWeight = 257 - targetWeight;              // 1-256. "max tot 255*256 + 255*1 = 256x256" // not true, 255*257 != 256*256. which is good tho bc it's 65535 so no overflow
  // a constant input 9 turns into 7 with this fwiw... 57 to 56, 252 to 252. why do lower values drop?
  // scale + >> 8 affecting even tho 

  uint8_t *pOrigin = *origin,   *pTarget = *target,
          *pData = receiver.get(), *pResiduals = receiver._residuals;

  for(uint16_t f=0; f < receiver.fieldCount(); f++) {
    for(uint8_t sub=0; sub < receiver.fieldSize(); sub++) {
      uint8_t destSub = receiver.subFieldForIndex(sub); // returns sub if no mapping. Best if done earlier stage tho... and even if inline better to prefetch and skip nullcheck each time?
      // uint8_t destSub = sub; // try do this bit in setCopy instead...
                                               // same with scale tbh - speed is all and dumb redoing same shit. So have multiple versions.

      mix24 = scale * (*pOrigin++ * originWeight + *pTarget++ * targetWeight); // involving scale and downshift will also lead to error, so try putting that in residuals?
      mix16 = mix24 >> 8; // shigt down 0xFFFFFF -> 0xFFFF
      mix8  = mix16 >> 8; //shift down 0xFFFF -> 0xFF
      error = *pResiduals + (mix16 - ((uint16_t)mix8 << 8))  // if err+16 overflows then 8 would restore yeah? the scales back upped one clipped..
                          + (mix24 - ((uint32_t)mix16 << 8)); // max 255 residual, 255 mix16, 255 mix8? so certainly more than one.
      // *(pData + destSub) = (error < 256)? mix8: mix8 + 1;
      if(mix8 >= receiver.noDitherPoint)
      // if(mix8 >= receiver.noDitherPoint && highestCanGo <= 255) // tho 
        *(pData + destSub) = (uint8_t)std::clamp((uint16_t)mix8 + (error / 256), 0, 255);
      else
        *(pData + destSub) = mix8;
      *pResiduals++ = error; // anything over 8bit capacity gets rolled over. but that only makes sense if looking <256...
      
      if(*(pData + destSub) <= receiver.blackPoint)
        *(pData + destSub) = 0;
      
    }
    pData += receiver.fieldSize();
  } // updated version seems much better! but still rocky
  
  receiver.setDirty();
  return true;
}



}
