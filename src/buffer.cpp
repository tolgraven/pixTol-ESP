#include "buffer.h"

template<class T, class T2>
void iBuffer<T, T2>::setCopy(const T* const newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
  if(!copyLength) copyLength = lengthBytes(); //use internal size unless specified
  // also need to boundz readOffset tho lol. tbh shouldnt correct errors but throw...
  // if(!newData || writeOffset >= lengthBytes() || copyLength > lengthBytes())
  //   return; //XXX improve with info on request. and fucking decide whether to accomodate (take what's possible)
  // or fail asap. crash > weirdo behavior no...
  memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
  setDirty();
}
template<class T, class T2>
void iBuffer<T, T2>::setCopy(const iBuffer<T, T2>& newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
  // since this is getting a buffer it can also change whether copylength is oob etc...
  setCopy(newData.get(), copyLength, readOffset, writeOffset);
}

template<class T, class T2>
void iBuffer<T, T2>::setPtr(T* const newData, uint16_t readOffset) { // skip readOffset passed.. better handle offset here
  if(ownsData) {
    delete[] data;
    ownsData = false;
  }
  data = newData + readOffset; //shouldnt need sizeof with ptr like this..
  setDirty();
}
template<class T, class T2>
void iBuffer<T, T2>::setPtr(const iBuffer<T, T2>& newData, uint16_t readOffset) {
  setFieldCount(newData.fieldCount() - readOffset);
  setFieldSize(newData.fieldSize());
  setPtr(newData.get(), readOffset);
}
// dun work doing partial updates unless copying, obviously.
// this should surely be changed from bytes to fields??
// or have setRaw(T*...) / set(Buffer...)
template<class T, class T2>
void iBuffer<T, T2>::set(T* const newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset, bool autoLock) {
  if(ownsData) {
    // if(!copyLength) copyLength = length(); //use internal size unless specified
    // std::clamp(copyLength, (uint16_t)1, uint16_t(length() - readOffset - writeOffset)); //XXX check why last one somehow promoted to int when all 3 involved are u16??
    // copyLength = std::clamp((int)copyLength, 1, length() - readOffset - writeOffset); //XXX check why last one somehow promoted to int when all 3 involved are u16??
    // copyLength = std::clamp(copyLength? copyLength: length(), 1, length() - readOffset - writeOffset); //XXX check why last one somehow promoted to int when all 3 involved are u16??
    // copyLength = std::clamp(copyLength? copyLength: length(), 1, length() - (readOffset + writeOffset)); //XXX check why last one somehow promoted to int when all 3 involved are u16??
    copyLength = std::clamp(int(copyLength? copyLength: length()), 1, length() - readOffset - writeOffset); // so u - u = i, sily
    // ^ XXX also check writeoffset... or dont check and let wrong things crash cmon
    memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
  } else {
    data = newData + readOffset; // bc target shouldnt be offset... but needed for if buffer arrives in multiple dmx unis or whatever...  but make sure "read-offset" done in-ptr instead...
  }

  setDirty();
}
template<class T, class T2>
void iBuffer<T, T2>::set(const iBuffer<T, T2>& from, uint16_t bytesLength, uint16_t readOffset, uint16_t writeOffset, bool autoLock) {
  set(from.get(), bytesLength, readOffset, writeOffset, autoLock);
}
template<class T, class T2>
void iBuffer<T, T2>::setByField(const iBuffer<T, T2>& from, uint16_t numFields, uint16_t readOffsetField, uint16_t writeOffsetField, bool autoLock) {
  setCopy(from.get(), numFields * fieldSize(), readOffsetField * fieldSize(), writeOffsetField * fieldSize());
}
template<class T, class T2>
void iBuffer<T, T2>::setField(const Field& source, uint16_t fieldIndex) { //XXX also set alpha etc - really copy entire field, but just grab data at ptr
  if(fieldIndex >= 0 && fieldIndex < fieldCount()) { // fill entire buffer. Or use sep method?
    memcpy(data + fieldIndex * fieldSize(), source.get(), fieldSize() * sizeof(T));
  }
  setDirty();
}

template<class T, class T2>
iBuffer<T, T2> iBuffer<T, T2>::getSubsection(uint16_t startField, uint16_t endField) const {
  auto temp = *this;
  temp.setPtr(get(startField));
  if(endField == 0) endField = fieldCount() - 1;
  temp.setFieldCount(endField - startField);
  return temp;
}

template<class T, class T2>
void iBuffer<T, T2>::fill(const Field& source, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  for(auto f = from; f <= to; f++) setField(source, f);
}
template<class T, class T2>
void iBuffer<T, T2>::gradient(const Field& start, const Field& end, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  for(auto f = from; f <= to; f++) {
    Field current = Field::blend(start, end, (float)to - f / to);
    setField(current, f); //p sure this would crash...
  }
}

template<class T, class T2>
void iBuffer<T, T2>::rotate(int count, uint16_t first, uint16_t last) {
  count = count % fieldCount();
  if(count == 0) return;
  auto steps = abs(count);
  if(last == 0 || last >= fieldCount())
    last = fieldCount() - 1; // 0 means max.

  /* int starts[2] = {last - (steps-1), first}; //depending on rotation direction */
  /* T* pFirst = get() + fieldSize() * (fwd? starts[0]: starts[1]); */
  //but as they then head in sep dirs will have to add further offset tho. keep up work later...
  // uint8_t temp[steps * fieldSize()]; //store overflow in temp so can wrap...
  // memcpy(temp, pFirst, steps);
  iBuffer<T, T2>* temp = new iBuffer<T, T2>("temp rotate" + id(), fieldSize(), steps); //copy whats needed
  auto readOffset = count > 0? (last - (steps-1)): first;
  temp->setByField(*this, steps, readOffset); //fix set so skip all the multi bull

  shift(count, first, last); // shift data
  // pFirst = get() + fieldSize() * (fwd? starts[1]: starts[0]); //update origin
  // copyFields(pFirst, temp, steps); // move temp back
  auto writeOffset = count > 0? first: (last - (steps-1));
  setByField(*temp, steps, 0, writeOffset);
  /* lock(false); */
  delete temp;
}

template<class T, class T2>
void iBuffer<T, T2>::shift(int count, uint16_t first, uint16_t last) {
  if(last == 0 || last >= fieldCount())
    last = fieldCount() - 1; // 0 means max.
  uint16_t fromIndex = count < 0? first - count: first; // copy source index. goes up when moving backwards, as earlier discarded. always first, if moving ahead.
  if(fromIndex > fieldCount()) return; // for now, should zero out entire tho i guess
  uint16_t toIndex = count < 0? first: first + count; // move fwd, it's just first+count. back, first?
  uint16_t numFields = min(last - toIndex + 1, last - fromIndex + 1); //limited by both space copied to, and from

  memmove(get() + toIndex*fieldSize(), get() + fromIndex*fieldSize(), numFields * fieldSize());
  // 0 1 2 3 4 5 orig -1
  // t b
  // 1 2 3 4 5
  //
  // 0 1 2 3 4 5 orig +1
  // b t
  //   0 1 2 3 4
  // if(base < 0) { //cant touch before buffer so cleave
  // }
  // T* pFirst = get() + first*fieldSize();
  // T* pFront = get() + base*fieldSize();

  // moveFields(pFirst, pFront, numFields);
  //something about memmove()
} // intentional no dirty

// template<class T, class T2>
// iBuffer<T, T2>& iBuffer<T, T2>::scaledTo(uint16_t fieldCount) {
//   // resize/rescale buffer.
//   // try basic anti aliasing along strip..
//   // also good for anim? small Generators can scale up and down for movment...
// }

template<class T, class T2>
void iBuffer<T, T2>::setDithering(bool on) {
  if(on) {
    setDithering(false); // ensure clean - why tho heh
    _residuals = new T[length()];
    memset(_residuals, 0, lengthBytes());
  } else {
    delete [] _residuals;
    _residuals = nullptr;
  }
}

template<class T, class T2>
void iBuffer<T, T2>::lock(bool state) { // finalize keyframe?, at that point can be flushed or interpolated against
  // .... uh some checks
  setDirty(!state);
}

template<class T, class T2>
T iBuffer<T, T2>::averageInBuffer(uint16_t startField, uint16_t endField) { //calculate avg brightness for buffer. Or put higher and averageValue?
  // any way get type of type with trmplates?
  // ints vs floats...`
  uint32_t tot = 0;
  endField = endField <= fieldCount()? endField: fieldCount();
  for(auto i = startField * fieldSize(); i < length(); i++) tot += data[i]; //bypasses field thing but seems lack enough ram for that anyways
  return tot / (endField - startField);
}


template<class T, class T2>
void iBuffer<T, T2>::applyGain() {
  float gain = std::clamp(getGain(), 0.0f, 1.0f); //in this case. want overdrive too tho.
  uint8_t brightness = gain * 255;
  /* if(brightness <= totalBrightnessCutoff) brightness = 0; //until dithering... */

  /* uint16_t scale = (((uint16_t)brightness << 8) - 1) / 255; */
  // well this is useless in this context lol. only makes sense if shifting upwards/not 255 div
  if (brightness == 0) {
    memset(data, 0x0, lengthBytes());
    return; /* scale = 0; // Avoid divide by 0 */
  } else if(brightness == 255) {
    return;
  }
  uint16_t scale = brightness;
  uint8_t* ptr = get();
  uint8_t* ptrEnd = ptr + lengthBytes();
  while (ptr != ptrEnd) {
    uint16_t value = *ptr;
    *ptr++ = (value * scale) >> 8;
  }
}
// iBuffer<int>& iBuffer<T, T2>::difference(iBuffer& other, float fraction) const {
// DiffBuffer& difference(iBuffer& other, float fraction) const {
//   T* otherData = other.get();
//   DiffBuffer diffBuf(fieldSize(), fieldCount());
//   for(uint16_t i=0; i < fieldCount(); i++) {
//     for(uint8_t j=0; j < fieldSize(); j++) {
//       int value = data[i+j] - otherData[i+j]; //value < 0? 0: value > 255? 255: value;
//       diffBuf.set(&value, i+j);
//     }
//   }
//   return diffBuf;
// }
// below obv should be buffer level
// void iBuffer<T, T2>::copyFields(T* destination, const T* source, uint16_t count) {
// void iBuffer<T, T2>::moveFields(int direction, const T* source, uint16_t count) {
//   memmove()
//
//   // T* end = destination + count*fieldSize();
//   // while(destination < end) {
//   //   for(auto i=0; i<fieldSize(); i++)
//   //     *destination++ = *source++; // then NeoPixelBus has fancy packing 4-byte in uint32 and shit, but surely overoptimization??
//   // } // what's wrong with memcpy anyways?
// }
// void iBuffer<T, T2>::moveFieldsDec(T* destination, const T* source, uint16_t count) {
// // use memmove instead to start...
//   T* end = destination + count*fieldSize();
//   const T* srcEnd = source + count*fieldSize();
//   while(end > destination) {
//     for(auto i=0; i<fieldSize(); i++) *--end = *--srcEnd;
//   }
// }

template<class T, class T2>
void iBuffer<T, T2>::blendUsingEnvelopeB(const iBuffer<T, T2>& origin, const iBuffer<T, T2>& target, float progress, BlendEnvelope* e) {
  // later fix so eg pre scale buffers if different size and whatnot
  float  attack = e? e->A(progress): progress,
        release = e? e->R(progress): progress;
  uint8_t aScale = attack * 255L,
          rScale = release * 255L;
  // for(uint16_t f=0; f < fieldCount(); f++) {
  //   for(uint8_t sub=0; sub < fieldSize(); sub++) {
  //   }
  // }
  for(uint16_t i=0; i < fieldCount(); i++) { // XXX this needs tighening up. Skip fields lol put directly...

    Field fOrigin = origin.getField(i);
    Field fTarget = target.getField(i);
    Field fCurrent = this->getField(i); // oh wait now there turns constructor -> assignment copy constructor?
    fCurrent = fOrigin;
    fCurrent.blend(fTarget, (fTarget > fOrigin? aScale: rScale)); // get Field for our buffer, put origin in it, blend with target...
  }
  setDirty();
}

// int iR = (pixelPrev[0] * icPrev + pixelNext[0] * icNext) >> 16;
// off 32bit (but small) weights
//  iR += pResidual[0];
//  int r8 = __USAT(iR + 0x80, 16) >> 8;  //arm instruction clamping fucker...
//  pResidual[0] = iR - (r8 * 257);
//  so residual fully gets added back and then retrieved every frame
//  anf ofc aspect to more in 16 and rounding not clipping much more natural! but necessites signed ints
//
// blabla doubling up strips for added resolution -> hell yeah
// but also, stay on 32 bits further.  use all the space. now  8 i/o, 8 weight, 8 scale.  go like 10 weight, 10 scale?
// also carry over scale error same way we do pixels? tho then when higher applied next time existing residual is all wrong.
template<class uint8_t, class int16_t>
void iBuffer<uint8_t, int16_t>::
interpolate(const iBuffer<uint8_t, int16_t>& origin,
            const iBuffer<uint8_t, int16_t>& target,
            float progress, BlendEnvelope* e) { // input buffers being interpolated, weighting (0-255) of second buffer in interpolation

  // if(_residuals == nullptr) { // no error correction buffers. but passing e just for this seems dumb. blend and temporal dithering-interp are different things so should never be called unless got em!
  //   blendUsingEnvelopeB(origin, target, progress, e);
  //   return;
  // }
  if(_residuals == nullptr) setDithering(true); //avoid crash... but just fix callers/document reqs tbh

  uint32_t scale = std::clamp(getGain(), 0.0f, 1.0f) * 255UL; // or 256, or 257 or whatever tf it is lol
  if(scale == 0) {
    memset(this->get(), 0x00, lengthBytes());
    memset(_residuals, 0x00, lengthBytes());
    return;
  }

  // tho, in here I guess keep everything not reliant on overflow at u32?
  uint16_t mix16, error;          // should we add a bit of resolution? since gets compounded... wait no cause cancels hah
  uint8_t mix8;                   // but if 0xFFFF * 0xFF we shift down 16 for mix16

  uint16_t targetWeight = (uint16_t)(progress * 255L) + 1, // 1-256
           originWeight = 257 - targetWeight;              // same. max tot 255*256 + 255*1 = 256x256. men tappar kunna gå på framen hmm

  uint8_t *pOrigin = *origin, *pTarget = *target, *pData = this->get(), *pResiduals = _residuals;

  for(uint16_t f=0; f < fieldCount(); f++) {
    for(uint8_t sub=0; sub < fieldSize(); sub++) {
      uint8_t destSub = subFieldForIndex(sub); // returns sub if no mapping. Best if done earlier stage tho... and even if inline better to prefetch and skip nullcheck each time?
                                               // same with scale tbh - speed is all and dumb redoing same shit. So have multiple versions.
      mix16 = (scale * (*pOrigin++ * originWeight + *pTarget++ * targetWeight)) >> 8;
      mix8  = mix16 >> 8; //shift down 0xFFFF -> 0xFF
      error = *pResiduals + mix16 - ((uint16_t)mix8 << 8);  // if err+16 overflows then 8 would restore yeah? the scales back upped one clipped..
      *(pData + destSub) = (error < 256)? mix8: mix8 + 1;
      *pResiduals++ = error; // anything over 8bit capacity gets rolled over. but that only makes sense if looking <256...
    }
    pData += fieldSize();
  }
  // for black most important is only going Field level. No harm in 3-4-5-6 when other colors also in.
  // I think prob skip here for now...
  /* if(blackPoint != 0) { // tho guess (apart from going 0/twice) other thing might be like, 4-7 bump to 8, 3 be 0. */
  /*   pData = this->get(); // Even more jarring cutoff then tho. also shift in some white.. */
  /*   uint8_t* dataEnd = pData + lengthBytes(); //also this'll fuck up residuals. anyways */
  /*   while(pData != dataEnd) { */
  /*     if(*pData++ < blackPoint) *(pData - 1) = 0; */
  /*   } */
  /* } */
  setDirty();
}


template class iBuffer<uint8_t, int16_t>; //was this for againj
/* template class iBuffer<int>; */
/* template class iBuffer<uint16_t>; */
/* template class iBuffer<float>; */

