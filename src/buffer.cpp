#include "buffer.h"

namespace tol {

template<class T, class T2>
void iBuffer<T, T2>::setCopy(const T* const newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
  if(!copyLength) copyLength = length() - readOffset - writeOffset;
  memcpy(data + writeOffset * sizeof(T), newData + readOffset * sizeof(T), copyLength * sizeof(T)); // watch out offset, if using to eg skip past fnChannels then breaks
  setDirty();
}
template<class T, class T2>
void iBuffer<T, T2>::setCopy(const iBuffer<T, T2>& newData, uint16_t copyLength, uint16_t readOffset, uint16_t writeOffset) {
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
    copyLength = std::clamp(int(copyLength? copyLength: length()), 1, length() - readOffset - writeOffset); // so u - u = i, sily
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
void iBuffer<T, T2>::setField(const iField<T, T2>& source, uint16_t fieldIndex) { //XXX also set alpha etc - really copy entire field, but just grab data at ptr
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
iBuffer<T, T2> iBuffer<T, T2>::concat(const iBuffer& rhs) {
  iBuffer temp(id() + " + " + rhs.id(), fieldSize(),
                fieldCount() + rhs.fieldCount());
  temp.setCopy(*this);
  temp.setCopy(rhs, 0, 0, this->fieldCount());
  return temp;
}

template<class T, class T2>
void iBuffer<T, T2>::fill(const iField<T, T2>& source, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  for(auto f = from; f <= to; f++)
    setField(source, f);
}
template<class T, class T2>
void iBuffer<T, T2>::fill(const T& source, uint16_t from, uint16_t to) {
  for(int i=0; i < length(); i++) { // more better
    data[i] = source;
  }
}

template<class T, class T2>
void iBuffer<T, T2>::gradient(const iField<T, T2>& start, const iField<T, T2>& end, uint16_t from, uint16_t to) {
  if(!to || to >= fieldCount()) to = fieldCount() - 1; // also check from tho I guess..
  for(auto f = from; f <= to; f++) {
    iField<T, T2> current = iField<T, T2>::blend(start, end, (float)to - (float)f / to);
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

  iBuffer<T, T2>* temp = new iBuffer<T, T2>("temp rotate" + id(), fieldSize(), steps); //copy whats needed
  auto readOffset = count > 0? (last - (steps-1)): first;
  temp->setByField(*this, steps, readOffset); //fix set so skip all the multi bull

  shift(count, first, last); // shift data
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
  setDirty(!state); // .... uh some checks
}

template<class T, class T2>
T iBuffer<T, T2>::averageInBuffer(uint16_t startField, uint16_t endField) { //calculate avg brightness for buffer. Or put higher and averageValue?
  T2 tot = 0;
  endField = endField <= fieldCount() && endField != 0? endField: fieldCount();
  for(auto i = startField * fieldSize(); i < length(); i++)
    tot += data[i]; //bypasses field thing but seems lack enough ram for that anyways
  return tot / (endField - startField);
}

template<class uint8_t, class uint16_t>
void iBuffer<uint8_t, uint16_t>::applyGain() {
  float gain = std::clamp(getGain(), 0.0f, 1.0f); //in this case. want overdrive too tho.
  uint8_t brightness = gain * 255;
  if(brightness <= blackPoint) brightness = 0; //until dithering...

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

template<>
void iBuffer<float, float>::applyGain() {
  for(auto i=0; i < fieldCount(); i++) {
    data[i] *= _gain;
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
void iBuffer<T, T2>::blendUsingEnvelopeB(const iBuffer<T, T2>& origin, const iBuffer<T, T2>& target,
                                         float progress, BlendEnvelope* e) {
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

    iField<T, T2> fOrigin = origin.getField(i);
    iField<T, T2> fTarget = target.getField(i);
    iField<T, T2> fCurrent = this->getField(i); // oh wait now there turns constructor -> assignment copy constructor?
    fCurrent = fOrigin;
    fCurrent.blend(fTarget, (fTarget > fOrigin? aScale: rScale)); // get Field for our buffer, put origin in it, blend with target...
  }
  setDirty();
}


template class iBuffer<uint8_t, int16_t>; //was this for againj
template class iBuffer<uint16_t, int32_t>;
template class iBuffer<int16_t, int32_t>;
template class iBuffer<float>;

}
