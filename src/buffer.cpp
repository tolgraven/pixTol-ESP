#include "buffer.h"

template<class T>
void iBuffer<T>::setCopy(const T* const newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
  if(!copyLength) copyLength = lengthBytes(); //use internal size unless specified
  // also need to boundz readOffset tho lol. tbh shouldnt correct errors but throw...
  if(!newData || writeOffset >= lengthBytes() || copyLength > lengthBytes()) outOfBounds(); //XXX improve with info on request
  /* constrain(writeOffset, 0, length() - 1); */
  /* constrain(copyLength, 1, length() - readOffset - writeOffset -1); // -1 */
  memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
}
template<class T>
void iBuffer<T>::setCopy(const iBuffer<T>& newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
  // since this is getting a buffer it can also change whether copylength is oob etc...
  setCopy(newData.get(), copyLength, readOffset, writeOffset);
}

template<class T>
void iBuffer<T>::setPtr(T* const newData, uint16_t readOffset) {
  // should we just skip readOffset since passed raw ptr anyways? or better handle offset here
  // here's where smart ptrs needed - if we reset here, well...
  // BUT shouldnt really fuck with this anyways - every ptr wraps in a Buffer and to repoint, set other Buffer as target...
  if(ownsData) {
    delete[] data;
    ownsData = false;
  }
  data = newData + readOffset; //shouldnt need sizeof with ptr like this..
}
template<class T>
void iBuffer<T>::setPtr(const iBuffer<T>& newData, uint16_t readOffset) {
  setPtr(newData.get(), readOffset);
}
// dun work doing partial updates unless copying, obviously.
// this should surely be changed from bytes to fields??
// or have setRaw(T*...) / set(Buffer...)
template<class T>
void iBuffer<T>::set(T* const newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset, bool autoLock) {
  if(ownsData) {
    if(!copyLength) copyLength = length(); //use internal size unless specified
    constrain(copyLength, 1, length() - readOffset - writeOffset);
    /* copyLength = copyLength < length() - readOffset? */
    /*   /1* copyLength - readOffset: *1/ /1* would cheat us from readoffset... *1/ */
    /*   copyLength: */
    /*   length() - readOffset; */
    // ^ XXX also check writeoffset...
    memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
  } else
    data = newData + readOffset; // bc target shouldnt be offset... but needed for if buffer arrives in multiple dmx unis or whatever...  but make sure "read-offset" done in-ptr instead...
  // writeOffset doesn't make sense in normal ptr operation tho lol? i suck

  /* updatedBytes += copyLength; //or we count in fields prob? */
  /* if(updatedBytes >= length() && autoLock) lock(); */
  /* else dirty = true; //XXX maybe no good, might have updated with some garbage prev and redone? */
}
// BAD ALL THESE
template<class T>
void iBuffer<T>::set(const iBuffer<T>& from, uint16_t bytesLength, uint16_t readOffset, uint16_t writeOffset, bool autoLock) {
  return set(from.get(), bytesLength, readOffset, writeOffset, autoLock);
}
//will be real set() later. just means indexing by fields...
template<class T>
void iBuffer<T>::setByField(const iBuffer<T>& from, uint16_t numFields, uint16_t readOffsetField, uint16_t writeOffsetField, bool autoLock) {
  return set(from.get(), numFields * fieldSize(), readOffsetField * fieldSize(), writeOffsetField * fieldSize(), autoLock);
}
template<class T>
void iBuffer<T>::setField(const Field& source, uint16_t fieldIndex) { //XXX also set alpha etc - really copy entire field, but just grab data at ptr
  if(fieldIndex >= 0 && fieldIndex < _fieldCount) { // fill entire buffer. Or use sep method?
    memcpy(data + fieldIndex * _fieldSize, source.get(), _fieldSize * sizeof(T));
    updatedBytes += _fieldSize;
  } else outOfBounds();
}

template<class T>
T* iBuffer<T>::get(uint16_t byteOffset) const { //does raw offset make much sense ever? still, got getField.get() so...
  if(!this->data || byteOffset >= this->length()) outOfBounds();
  return this->data + byteOffset;
}
template<class T>
Field iBuffer<T>::getField(uint16_t fieldIndex) const {
  return Field(data, fieldSize(), fieldIndex); // create Fields on fly and return?
}

template<class T>
void iBuffer<T>::fill(const Field& source, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  /* lg.f("Fill", Log::DEBUG, "Fill %d, heap: %d", to, ESP.getFreeHeap()); */
  for(auto f = from; f <= to; f++) setField(source, f);
}
template<class T>
void iBuffer<T>::gradient(const Field& start, const Field& end, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  for(auto f = from; f <= to; f++) {
    Field current = Field::blend(start, end, (float)to - f / to);
    setField(current, f); //p sure this would crash...
  }
}

template<class T>
void iBuffer<T>::rotate(int count, uint16_t first, uint16_t last) {
  if(!count) return;
  bool fwd = count > 0;
  auto steps = abs(count);
  last = last? last: fieldCount(); // 0 means max. why fc and not -1?
  int starts[2] = {last - (steps-1), first}; //depending on rotation direction
  T* pFirst = get() + fieldSize() * (fwd? starts[0]: starts[1]);
  //but as they then head in sep dirs will have to add further offset tho. keep up work later...
  // uint8_t temp[steps * fieldSize()]; //store overflow in temp so can wrap...
  // memcpy(temp, pFirst, steps);
  iBuffer<T>* temp = new iBuffer<T>("temp rotate" + id(), fieldSize(), steps); //copy whats needed
  temp->setByField(*this, steps, (fwd? starts[0]: starts[1])); //fix set so skip all the multi bull

  shift(count, first, last); // shift data
  // pFirst = get() + fieldSize() * (fwd? starts[1]: starts[0]); //update origin
  // copyFields(pFirst, temp, steps); // move temp back
  setByField(*temp, steps, 0, (fwd? starts[1]: starts[0]));
  lock(false);
  delete temp;
}

template<class T>
void iBuffer<T>::shift(int count, uint16_t first, uint16_t last) {
  last = last && last >= first? last: fieldCount(); // 0 means max
  uint16_t fromIndex = count < 0? first - count: first; // copy source index. goes up when moving backwards, as earlier discarded. always first, if moving ahead.
  if(fromIndex > fieldCount()) return; // for now, should zero out entire tho i guess
  // uint16_t numFields = last - fromIndex + 1;
  uint16_t toIndex = count < 0? first: first + count; // move fwd, it's just first+count. back, first?
  // uint16_t numFields = last - toIndex + 1;
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

template<class T>
void iBuffer<T>::setDithering(bool on) {
  if(on) {
    setDithering(false); // ensure clean - why tho heh
    _residuals = new T[length()];
    memset(_residuals, 0, lengthBytes());
  } else {
    delete [] _residuals;
    _residuals = nullptr;
  }
    /* _residuals = new T*[fieldSize()]; */
    /* /1* _residuals = new uint16_t*[fieldSize()]; *1/ */
    /* for(uint8_t sub = 0; sub < fieldSize(); sub++) { */
    /*   _residuals[sub] = new T[fieldCount()]; */
    /*   memset(_residuals[sub], 0, fieldCount()); */
    /*   /1* _residuals[sub] = new uint16_t[fieldCount()]; *1/ */
    /*   /1* memset(_residuals[sub], 0, fieldCount() * 2); //for 8 / 16 *1/ */
    /* } */

  /* } else { */
    /* if(_residuals) { */
    /*   for(uint8_t sub = 0; sub < fieldSize(); sub++) { */
    /*     delete [] _residuals[sub]; */
    /*   } */
    /*   delete [] _residuals; */
    /*   _residuals = nullptr; */
    /* } */
  /* } */
}

template<class T>
void iBuffer<T>::lock(bool state) { // finalize keyframe?, at that point can be flushed or interpolated against
  if(state) {
    dirty = false;
    updatedBytes = 0;
  } else {
    dirty = true;
  }
}

/* T iBuffer<T>::averageInBuffer(uint16_t startField, uint16_t endField) { //calculate avg brightness for buffer. Or put higher and averageValue? */
/*   uint32_t tot = 0; */
/*   endField = endField <= fieldCount()? endField: fieldCount(); */
/*   for(auto i = startField * fieldSize(); i < length(); i++) tot += data[i]; //bypasses field thing but seems lack enough ram for that anyways */
/*   return tot / (endField - startField); */
/* } */


template<class T>
void iBuffer<T>::applyGain() {
  float gain = constrain(getGain(), 0, 1.0); //in this case. want overdrive too tho.
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
// iBuffer<int>& iBuffer<T>::difference(iBuffer& other, float fraction) const {
// DiffBuffer& difference(iBuffer& other, float fraction) const {
//   T* otherData = other.get();
//   DiffBuffer diffBuf(_fieldSize, _fieldCount);
//   for(uint16_t i=0; i < _fieldCount; i++) {
//     for(uint8_t j=0; j < _fieldSize; j++) {
//       int value = data[i+j] - otherData[i+j]; //value < 0? 0: value > 255? 255: value;
//       diffBuf.set(&value, i+j);
//     }
//   }
//   return diffBuf;
// }
// below obv should be buffer level
// void iBuffer<T>::copyFields(T* destination, const T* source, uint16_t count) {
// void iBuffer<T>::moveFields(int direction, const T* source, uint16_t count) {
//   memmove()
//
//   // T* end = destination + count*fieldSize();
//   // while(destination < end) {
//   //   for(auto i=0; i<fieldSize(); i++)
//   //     *destination++ = *source++; // then NeoPixelBus has fancy packing 4-byte in uint32 and shit, but surely overoptimization??
//   // } // what's wrong with memcpy anyways?
// }
// void iBuffer<T>::moveFieldsDec(T* destination, const T* source, uint16_t count) {
// // use memmove instead to start...
//   T* end = destination + count*fieldSize();
//   const T* srcEnd = source + count*fieldSize();
//   while(end > destination) {
//     for(auto i=0; i<fieldSize(); i++) *--end = *--srcEnd;
//   }
// }

template<class T>
void iBuffer<T>::blendUsingEnvelopeB(const iBuffer<T>& origin, const iBuffer<T>& target, float progress, BlendEnvelope* e) {
  // later fix so eg pre scale buffers if different size and whatnot
  float  attack = e? e->A(progress): progress,
        release = e? e->R(progress): progress;
  uint8_t aScale = attack * 255L,
          rScale = release * 255L;
  for(uint16_t f=0; f < fieldCount(); f++) {
    for(uint8_t sub=0; sub < fieldSize(); sub++) {
    }
  }
  for(uint16_t i=0; i < fieldCount(); i++) { // XXX this needs tighening up. Skip fields lol put directly...

    Field fOrigin = origin.getField(i);
    Field fTarget = target.getField(i);
    Field fCurrent = this->getField(i); // oh wait now there turns constructor -> assignment copy constructor?
    fCurrent = fOrigin;
    fCurrent.blend(fTarget, (fTarget > fOrigin? aScale: rScale)); // get Field for our buffer, put origin in it, blend with target...
  }
}

  //fade:
  // int iR = (pixelPrev[0] * icPrev + pixelNext[0] * icNext) >> 16;
  // off 32bit (but small) weights
  //  iR += pResidual[0];
  //  int r8 = __USAT(iR + 0x80, 16) >> 8;  //arm instruction clamping fucker...
  //  pResidual[0] = iR - (r8 * 257);
  //  so residual fully gets added back and then retrieved every frame
  //  anf ofc aspect to more in 16 and rounding not clipping
  //  much more natural! but necessites signed ints
  //
  // dont get why she's using 0xFFFF+1 as scale off millis timing though??
  // -> because why not, use spare bitslol
  //
  // blabla doubling up strips for added resolution -> hell yeah
  // but also, stay on 32 bits further.
  // use all the space. now  8 i/o, 8 weight, 8 scale.
  // go like 10 weight, 10 scale?
  // also carry over scale error same way we do pixels? tho then when higher applied
  // next time existing residual is all wrong.

template<class uint8_t>
void iBuffer<uint8_t>::interpolate(const iBuffer<uint8_t>& origin, const iBuffer<uint8_t>& target,
                                   float progress, BlendEnvelope* e) { // input buffers being interpolated, weighting (0-255) of second buffer in interpolation
  if(_residuals == nullptr) { // no error correction buffers
    blendUsingEnvelopeB(origin, target, progress, e);
    return;
  }

  uint32_t scale = constrain(getGain(), 0.0, 1.0) * 255L; // or 256, or 257 or whatever tf it is lol
  if(scale == 0) {
    memset(get(), 0x00, lengthBytes());
    memset(_residuals, 0x00, lengthBytes());
    return;
  } /* lg.fEvery(300, 3, __func__, Log::DEBUG, "gain: %.3f, scale: %u\n", gain, scale); */

  uint16_t mix16, error;          // should we add a bit of resolution? since gets compounded... wait no cause cancels hah
  uint8_t mix8;                   // but if 0xFFFF * 0xFF we shift down 16 for mix16

  uint16_t targetWeight = (uint16_t)(progress * 255L) + 1, // 1-256
           originWeight = 257 - targetWeight;              // same. max tot 255*256 + 255*1 = 256x256. men tappar kunna gå på framen hmm

  uint8_t *originPtr = origin.get(), *targetPtr = target.get(),
          *dataPtr = this->get(), *residualsPtr = _residuals;

  for(uint16_t f=0; f < fieldCount(); f++) {
    for(uint8_t sub=0; sub < fieldSize(); sub++) {
      uint8_t destSub = subFieldForIndex(sub); // returns sub if no mapping. Best if done earlier stage but...
                                               // same with scale tbh - speed is all and dumb redoing same shit. So have multiple versions.
      mix16 = (scale * (*originPtr++ * originWeight + *targetPtr++ * targetWeight)) >> 8;
      mix8 = mix16 >> 8; //shift down 0xFFFF -> 0xFF
      error = *residualsPtr + mix16 - ((uint16_t)mix8 << 8);  // if err+16 overflows then 8 would restore yeah? the scales back upped one clipped..
      *(dataPtr + destSub) = (error < 256)? mix8: mix8 + 1;
      *residualsPtr++ = error; // anything over 8bit capacity gets rolled over. but that only makes sense if looking <256...
    }
    dataPtr += fieldSize(); //, residualsPtr += fieldSize();
  }
  /* if(blackPoint != 0) { // tho guess (apart from going 0/twice) other thing might be like, 4-7 bump to 8, 3 be 0. */
  /*   dataPtr = this->get(); // Even more jarring cutoff then tho. also shift in some white.. */
  /*   uint8_t* dataEnd = dataPtr + lengthBytes(); //also this'll fuck up residuals. anyways */
  /*   while(dataPtr != dataEnd) { */
  /*     if(*dataPtr++ < blackPoint) *(dataPtr - 1) = 0; */
  /*   } */
  /* } */
}


template class iBuffer<uint8_t>;
/* template class iBuffer<int>; */
/* template class iBuffer<uint16_t>; */
/* template class iBuffer<float>; */


