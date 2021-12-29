#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

enum FieldSize: uint8_t {
  SINGLE = 1, DMX = 1, ONE_BYTE = 1, WHITE = 1, UV = 1, //etc
  DMX_FINE = 2, TWO_BYTE = 2,
  RGB = 3, RGBW = 4
};
/* enum class Blend: uint8_t { */
/*   Overlay = 0, Htp, Lotp, Average, Add, Subtract, Multiply, Divide */
/* }; // not suitable tho?: "SCALE" - like live "modulate", would need extra var setting zero-point (1.0 = existing value is maximum possible, 0.5 can reach double etc). Putting such meta-params on oscillators etc = likely exciting */
// struct FieldFlags { //in case need optimize to pack individual flags/whatever into common bitfield, then would put in Buffer...
//   uint16_t data; //2-3 bits size, 4 mode, 8 alpha,
//   // FieldFlags(uint16_t fieldCount): data(new uint8_t[fieldCount / 2]) { //assuming 8 modes = 4 bits = 2 fields per byte
//   FieldFlags(uint8_t fieldSize, uint8_t blendMode, float alpha) { //assuming 8 modes = 4 bits = 2 fields per byte
//       //code to set/read individual indexes of bitfield
//   }
// };
template<class T, class T2=T>
class iField {
// make template? should be as lean as possible packing bits, ie BlendMode fits in half a byte...
// so a field of fields probably makes more sense than individual objects
// no premature optimization tho, make it fancy then trim later. But need one per field _per incoming stream_, plus end dest... lots
// same type should store value + metadata, whether field is single float or 4-byte pixel
  protected:
  T* data = nullptr; //skip dataptr and only have offset and size?  then gotta pass baseptr for literally everything so kinda shit and pointless but would save loads of memory...
  size_t _size = 1; // uint8_t _size;
  bool ownsData = false;
  // float alpha = 1.0; //uint8_t alpha = 255; //switch to int if too slow/big?
  // Blend mode = Blend::Overlay; // could also have pointers for both alpha and mode, so optional and then always returning values 1.0 / OVERLAY? or better yet "DEFER" = Buffer-wide setting handles...
  // OR
  // struct Alpha {
  //     uint8_t alpha = 255;
  //     Blend mode = Blend::OVERLAY;
  //     // OR
  //     // using blendFlags_t = uint16_t; and *fancy math to use four bits for mode flag and 12 for alpha??*
  //   Blend(float fractionAlpha = 1.0, Blend mode = Blend::Overlay): mode(mode) {
  //     fractionAlpha = fractionAlpha >= 1.0? 1.0: fractionAlpha <= 0.0? 0.0: fractionAlpha;
  //     alpha = fractionAlpha*255;
  //   }
  // }; Alpha alpha;
  using LIM = typename std::numeric_limits<T>; // or have static like iBuffer. per-type so...

  public:
  iField(size_t size): iField(nullptr, size, 0, false) { } // noo go gets to messy if we fuck with own allocs or smart ptrs
  iField(T& value):
    data(new T[1]), _size(1), ownsData(true) {
      *data = value;
  } // noo go gets to messy if we fuck with own allocs or smart ptrs
  // iField(args...): iField()
  iField(T* bufferStart, size_t size, uint16_t fieldIndex = 0, bool copy = false):
    _size(size) {
      // XXX these make no sense. looks like copy makes it set *bufferStart to whats at fieldIndex...
      if      (!copy && bufferStart) { setPtr(bufferStart, fieldIndex);
      } else if(copy && bufferStart) { setCopy(bufferStart + fieldIndex);
      } else if(!bufferStart) {
          ownsData = true;
          data = new T[size];
      }
  }
  iField(const iField& rhs): iField(rhs.get(), rhs.size(), 0, true) {  } // p f dangerous copy constructor unless actually copies, so...
  ~iField() { if(ownsData) delete[] data; }; // ~iField() { if(ownsData) delete[] data; }; //how goz dis when we dont created array, just using ptr?

  bool operator==(const iField& rhs) const { return memcmp(rhs.get(), data, _size * sizeof(T)) == 0; } //XXX add alpha check n shite
  bool operator!=(const iField& rhs) const { return !(*this == rhs); }
  bool operator<(const iField& rhs)  const { return (average() <  rhs.average()); }
  bool operator<=(const iField& rhs) const { return (average() <= rhs.average()); }
  bool operator>(const iField& rhs)  const { return (average() >  rhs.average()); }
  bool operator>=(const iField& rhs) const { return (average() >= rhs.average()); }

  T& operator[](uint8_t index) const { return *get(index); } //eh prob not so reasonable heh. but if want to be able to do f[0] = 5; f[1] = 30;
  iField& operator=(const iField& rhs) { setCopy(rhs.get()); return *this; }

  iField& operator+(const T delta) { value(delta, true); return *this; } // lighten
  iField& operator+(const iField& rhs) { return add(rhs); }
  iField& operator++() { return value(1); } // lighten

  iField& operator-(const iField& rhs) { return sub(rhs); }
  iField& operator-(const T delta) { value(delta, false); return *this; } // lighten
  iField& operator--() { return value(-1); } // darken

  // might be overkill and that  should only do this directly in Buffer, but let's see
  iField& mix(const iField& rhs, std::function<T(T, T)> op, bool perByte = true) { // could auto this even or?
    T* otherData = rhs.get();
    if(perByte) {
      for(auto i=0; i<_size; i += sizeof(T))
        data[i] = op(data[i], otherData[i]);
    } else { // compare "total" value and use accordingly...
    }
    return *this;
  }
  iField& add(const iField& rhs) {
    return mix(rhs, [](T l, T r) {
        // TODO OR pass limits on creation (eg for float clamped to unity)
        return (T)std::clamp((T2)((T2)l + (T2)r), (T2)LIM::min(), (T2)LIM::max());
      });
  }
  iField& sub(const iField& rhs) {
    return mix(rhs, [](T l, T r) {
        return (T)std::clamp((T2)((T2)l - (T2)r), (T2)LIM::min(), (T2)LIM::max());
      });
  }
  iField& avg(const iField& rhs, bool skipZero = true) {
    auto op = [skipZero](T l, T r) {
      auto c = (T2)l + r;
      bool gotZero = (c == l || c == r);
      T div = skipZero && gotZero? 1: 2;
      return (T)(c / div);
    };
    return mix(rhs, op);
  }
  iField& htp(const iField& rhs, bool perByte = true) {
    if(!perByte) {
      if(rhs.average() > average()) *this = rhs;
      return *this;
    } else // else return *this; //thats what lotp does
      return mix(rhs, [](T l, T r) { return std::max(l, r); });
  }
  iField& ltp(const iField& rhs) {
    return (*this = rhs);
    // *this = rhs; return *this;
  }
  iField& lotp(const iField& rhs, bool perByte = true) {
    if(!perByte) {
      if(rhs.average() < average()) *this = rhs;
      return *this;
    } else
      return mix(rhs, [](T l, T r) { return std::min(l, r); });
  }

  //dangerous naming avg/average. getStrength() or something?
  T average() const { //average value of field elements or like?
    T2 cum = 0;
    for(auto p=0; p<_size; p++) cum += data[p];
    return (cum / _size);
  }
  iField& value(T adjustment) { //still 255 range, but +/-...
    for(auto p=0; p<_size; p++)
      data[p] = std::clamp((T2)((T2)data[p] + adjustment), (T2)LIM::min(), (T2)LIM::max());
    return *this;
  }
  iField& value(float absolute, float dummyParam) {
    for(auto p=0; p<_size; p++)
      data[p] = absolute * LIM::max();
    return *this;
  }
  iField& value(T delta, bool higher) {//lighten or darken as appropriate...
    T2 adjust = higher? delta: -(T2)delta;
    for(auto p=0; p<_size; p++)
      data[p] = std::clamp((T2)((T2)data[p] + adjust), (T2)LIM::min(), (T2)LIM::max());
    return *this;
  }


  size_t size() const { return _size; }
  T* get(uint8_t offset = 0) const { return data + offset; } //call oob or something otherwise.
  
  iField& setCopy(const T* newData) { //set data by copy. also need a copy constructor
    for(auto i=0; i<_size; i += sizeof(T)) data[i] = newData[i];
    return *this;
  }
  iField& setCopy(const iField& newData) { return setCopy(newData.get()); } //set data by copy. also need a copy constructor
  iField& setPtr(T* bufferStart, uint16_t fieldIndex) { //set ptr, or copy block
    data = bufferStart + fieldIndex * _size;
    return *this;
  }
  iField& setSubField(T value, uint16_t index) { //set subfield. dunno I mean field[2] = 8 yeah?
    data[index] = value; return *this;
  }

  // per-subpixel a/r also good...
  // XXX update for T
  void blend(const iField& target, uint8_t progress);
  // void blend(const iField& target, uint8_t progress) {
  //   uint16_t targetWeight = progress + 1,
  //            ourWeight = 257 - targetWeight;
  //   for(uint8_t p = 0; p < _size; p++) {
  //     data[p] = ((data[p] * ourWeight) + (target[p] * targetWeight)) >> 8; // works because rolls over I guess
  //   }
  // }
  enum AlphaMode { MIX = 0, WEIGHT };
  /* void blend(const iField& target, float progress = 0.5f, uint8_t alphaMode = MIX) { */
    /* if(alphaMode == MIX) { // alpha = (alpha * progress) + target.alpha * (1.0f - progress); */
    /* } else if(alphaMode == WEIGHT) { } // XXX a blend weighting by (not merely mixing) alpha */
  void blend(const iField& target, float progress = 0.5f) {
    /* progress = std::clamp(progress, 0.0f, 1.0f); //tho past-1.0 should also work eventually */
    for(uint8_t p = 0; p < _size; p++) {
      data[p] += ((target[p] - data[p]) * progress); // works because rolls over I guess
    }
  }
  void blend(const iField& x1, const iField& y0, const iField& y1, float x = 0.5f, float y = 0.5f) {
    float wx0 = (1.0f - x) * (1.0f - y);
    float wx1 =         x  * (1.0f - y);
    float wy0 = (1.0f - x) * y;
    float wy1 =         x  * y;
    for(uint8_t p = 0; p < _size; p++) {
      data[p] = data[p] * wx0 + x1[p] * wx1 + y1[p] * wy0 + y1[p] * wy1;
    }
    // alpha = alpha * wx0 + x1.alpha * wx1 + y1.alpha * wy0 + y1.alpha * wy1;
  }
  static iField blend(const iField& one, const iField& two, float progress = 0.5f, uint8_t alphaMode = MIX) {
    progress = std::clamp(progress, 0.0f, 1.0f); //tho past-1.0 should also work eventually
    iField blended = iField(one.size());
    for(uint8_t p = 0; p < one.size(); p++) {
      blended.setSubField(one[p] + progress * (uint16_t(two[p]) - uint16_t(one[p])), p); // works because rolls over I guess
    }
    return blended;
  }
  static iField blend(const iField& x0, const iField& x1, const iField& y0, const iField& y1, float x = 0.5f, float y = 0.5f) {
    float wx0 = (1.0f - x) * (1.0f - y);
    float wx1 =         x  * (1.0f - y);
    float wy0 = (1.0f - x) * y;
    float wy1 =         x  * y;
    iField blended = iField(x0.size());
    for(uint8_t p = 0; p < x0.size(); p++) {
      blended.setSubField(x0[p] * wx0 + (x1[p] * wx1) + (y1[p] * wy0) + (y1[p] * wy1), p);
    }
    return blended;
  }
};

using Field = iField<uint8_t, int16_t>;
using Field16 = iField<uint16_t, int32_t>;
using FieldF = iField<float>;
// so w both Field and Buffer growing in parallell and mostly overlapping
// and Field is just a scaled -down tiny buffer w fieldCount as fieldSize...
// sort further code reuse.
// tho that was orig plan like most impl in Field which Buffers work with... might not work out like that tho...
