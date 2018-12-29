#pragma once

#include <limits>
#include <Arduino.h>

// class Field { // make template? should be as lean as possible packing bits, ie BlendMode fits in half a byte...
//   // so a field of fields probably makes more sense than individual objects
//   // no premature optimization tho, make it fancy then trim later. But need one per field _per incoming stream_, plus end dest... lots
//   // same type should store value + metadata, whether field is single float or 4-byte pixel
//   // BlendMode mode;
//   uint8_t size;
//   int16_t alpha;
// };
enum FieldSize: uint8_t {
  SINGLE = 1, DMX = 1, BYTE = 1, WHITE = 1, UV = 1, //etc
  DMX_FINE = 2, TWO_BYTE = 2,
  RGB = 3, RGBW = 4
};

enum class Blend: uint8_t {
  Overlay = 0, Htp, Lotp, Average, Add, Subtract, Multiply, Divide
}; // not suitable tho?: "SCALE" - like live "modulate", would need extra var setting zero-point (1.0 = existing value is maximum possible, 0.5 can reach double etc). Putting such meta-params on oscillators etc = likely exciting

// struct FieldFlags { //in case need optimize to pack individual flags/whatever into common bitfield, then would put in Buffer...
//   uint8_t* data;
//   FieldFlags(uint16_t fieldCount): data(new uint8_t[fieldCount / 2]) { //assuming 8 modes = 4 bits = 2 fields per byte
//       //code to set/read individual indexes of bitfield
//   }
// };

// for float, generally assume normalized 0-1. But still have instances (eg gain) where outside this required. How deal best?
template<class T> class iField {
  protected:
  T* data = nullptr;
  uint8_t _size = 0; // uint8_t _size;
  bool ownsData = false; //dumb to have in each, but for now...
  Blend mode = Blend::Overlay; // could also have pointers for both alpha and mode, so optional and then
  //  always returning values 1.0 / OVERLAY? or better yet "DEFER" = Buffer-wide setting handles...
  float alpha = 1.0; //uint8_t alpha = 255; //switch to int if too slow.
  // OR
  // struct Alpha {
  //   private:
  //     uint8_t alpha = 255;
  //     Blend mode = Blend::OVERLAY;
  //     // OR
  //     // using blendFlags_t = uint16_t; and *fancy math to use four bits for mode flag and 12 for alpha??*
  //   Blend(float fractionAlpha = 1.0, Blend mode = Blend::Overlay): mode(mode) {
  //     fractionAlpha = fractionAlpha >= 1.0? 1.0: fractionAlpha <= 0.0? 0.0: fractionAlpha;
  //     alpha = fractionAlpha*255;
  //   }
  // }; Alpha alpha;

  public:
  iField() {}
  iField(uint8_t size): iField(new T[size]{0}, size, 0, 1.0, true) {}
  // iField(iBuffer<T>& buffer, uint8_t index, float alpha = 1.0, bool copy = false):
  //   iField(buffer.get(), buffer.fieldSize(), index, alpha, copy) {}
  iField(T* bufferStart, uint8_t size, uint8_t fieldIndex = 0, float alpha = 1.0, bool copy = false):
    _size(size), alpha(alpha), ownsData(copy) {
      set(bufferStart, fieldIndex);
  }
  ~iField() { if(ownsData) delete[] data; }; //how goz dis when we dont created array, just using ptr?

  using most = std::numeric_limits<T>::max;
  using least = std::numeric_limits<T>::lowest;
  using isSigned = std::numeric_limits<T>::is_signed; //use to go different paths yeah

  bool operator==(const iField& rhs) const { return memcmp(rhs.get(), data, _size); } //XXX add alpha check n shite
  bool operator!=(const iField& rhs) const { return !(*this == rhs); }
  bool operator<(const iField& rhs)  const { return (average() < rhs.average()); }
  bool operator<=(const iField& rhs) const { return (average() <= rhs.average()); }
  bool operator>(const iField& rhs)  const { return (average() > rhs.average()); }
  bool operator>=(const iField& rhs) const { return (average() >= rhs.average()); }

  iField& operator+(const T delta) { value(delta, true); return *this; } // lighten
  iField& operator+(const iField& rhs) {
    T field[_size];
    // for(auto p=0; p<_size; p++) field[p] = constrain((int)data[p] + *other.get(p), 0, most()); //XXX cant be casting to int if dealing with whatever else...
    for(auto p=0; p<_size; p++) field[p] = constrain((int)data[p] + *other.get(p), 0, most()); //so here, would go !isSigned()? isInt()? cast one size up
    return *(new iField(field, _size));
  }
  iField& operator++() { return value(1); } // lighten. how do for like float tho? inc 0.01 or something?
  // WRONG, cant change in-place... Field& operator-(const uint8_t delta) { return value(delta, false); } // darken
  // Field& operator-(const uint8_t delta) {  } // darken
  iField& operator-(const iField& rhs) {
    T field[_size];
    for(auto p=0; p<_size; p++) field[p] = constrain((int)data[p] - *other.get(p), 0, most());
    return *(new iField(field, _size)); //surely dumb and dangerous memleak fiesta??
  }
  iField& operator-(T rhs) {
  }
  iField& operator--() { return value(-1); } // darken


  // might be overkill and that should do this directly in Buffer, but let's see
  // XXX should still work for uint8_t so no rush, but these need to be generalized for other datatypes
  iField& mix(iField& rhs, std::function<T(T, T)> op, bool perByte = true) { // could auto this even or?
    T* otherData = rhs.get();
    if(perByte) for(uint8_t i=0; i<_size; i++) data[i] = op(data[i], otherData[i]);
    else { // compare "total" value and use accordingly...
    } return *this;
  }
  iField& add(iField& rhs) {
    return mix(rhs, [](T a, T b) { return constrain((int)a + b, 0, most()); }); //XXX here for example
  }
  iField& sub(iField& rhs) {
    return mix(rhs, [](T a, T b) { return constrain((int)a - b, 0, most()); }); //XXX
  }
  iField& avg(iField& rhs, bool ignoreZero = true) {
    return mix(rhs, [ignoreZero](T a, T b) {
      T divisor = 2;
      // if(ignoreZero && (a + b == a || a + b == b))
      if(ignoreZero && (!a || !b))
        divisor = 1;
      return (T)(((int)a + b) / divisor); }); //XXX
  }
  iField& htp(iField& rhs, bool perByte = true) {
    if(!perByte) {
      T a = average(), b = rhs.average();
      if(b > a) set(rhs.get());
    }
    else return mix(rhs, [](T a, T b) { return max(a, b); });
  }
  iField& ltp(iField& rhs) { return set(rhs.get()); } //eh, i guess? return rightmost, for lack of timing metadata
  iField& lotp(iField& rhs, bool perByte = true) {
    if(!perByte) {
      T a = average(), b = rhs.average();
      if(b < a) return set(rhs.get());
      else return *this;
    }
    else return mix(rhs, [](T a, T b) { return min(a, b); });
  }

  T average() const { //average value of field elements or like?
    uint16_t cum = 0;
    for(auto p=0; p<_size; p++) cum += data[p];
    return (cum / _size);
  }
  iField& value(int adjust) { //still 255 range, but +/-...
    for(auto p=0; p<_size; p++) data[p] = constrain((int)data[p] + adjust, 0, most()); //XXX
    return *this;
  }
  iField& value(float absolute) {
    for(auto p=0; p<_size; p++) data[p] = absolute * most(); //XXX
    return *this;
  }
  iField& value(T delta, bool higher) {//lighten or darken as appropriate...
    int adjust = higher? delta: -(int)delta;
    for(auto p=0; p<_size; p++) data[p] = constrain((int)data[p] + adjust, 0, most()); //XXX
    return *this;
  }


  T* get(uint8_t offset = 0) const {
    if(offset > 0 && offset < _size) return data + offset;
  }
  iField& set(iBuffer<T>& buffer, uint8_t fieldIndex) { //copy block
    if(fieldIndex < buffer.fieldCount())
      set(buffer.get(), fieldIndex);
  }
  iField& set(T* newData, uint8_t fieldIndex = 0) { //copy block. 0 default fieldindex so can also do offset, if any, on ptr...
    if(ownsData) if(!data) data = new T[_size];
    memcpy(data, newData + fieldIndex * sizeof(T) * _size, sizeof(T) *_size);
    return *this;
  }
  iField& set(uint8_t subField, T value) {
    if(subField < _size) data[subField] = value;
    return *this;
  }
  iField& point(T* bufferStart) { if(bufferStart) data = bufferStart; } //set ptr
  uint8_t size(uint8_t size = 0) {
    if(size) _size = size;
    return _size;
  } // XXX ensure we can actually access the underlying memory if expanding size, tho easier said than done...
    // also if eg changing a bunch of fields to represent an underlyiong buffer differently,
    // starting data ptrs would have to be updated as well.


  enum AlphaMode { MIX = 0, WEIGHT };
  void blend(const iField& target, float progress = 0.5, uint8_t alphaMode = MIX) {
    progress = constrain(progress, 0.0, 1.0); //tho past-1.0 should also work eventually
    if(alphaMode == MIX) {
      alpha = (alpha * progress) + target.alpha * (1.0 - progress);
    } else if(alphaMode == WEIGHT) {
    } // XXX a blend weighting by (not merely mixing) alpha
    // ^^ wtf does mean??
    for(uint8_t p = 0; p < _size; p++) {
      data[p] = data[p] + ((*target.get(p) - data[p]) * progress); // works because rolls over I guess
    }
  }
  void blend(const iField& x1, const iField& y0, const iField& y1, float x = 0.5, float y = 0.5) {
    float wx0 = (1.0f - x) * (1.0f - y);
    float wx1 = x * (1.0f - y);
    float wy0 = (1.0f - x) * y;
    float wy1 = x * y;
    for(uint8_t p = 0; p < _size; p++) {
      data[p] = data[p] * wx0 + *x1.get(p) * wx1 + *y1.get(p) * wy0 + *y1.get(p) * wy1;
    }
    alpha = alpha * wx0 + x1.alpha * wx1 + y1.alpha * wy0 + y1.alpha * wy1;
  }
};

using Field    = iField<uint8_t>;
using Field8   = iField<uint8_t>;
using Field16  = iField<uint16_t>;
using FieldI   = iField<int>;
using FieldF   = iField<float>;
using FieldS   = iField<String>; //I mean yeah, no. Or could have with size forced 1 and something hmma dunno?
