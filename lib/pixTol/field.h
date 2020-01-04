#pragma once

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
class Field {
  protected:
  uint8_t* data = nullptr;
  uint8_t _size = 0; // uint8_t _size;
  float alpha = 1.0; //uint8_t alpha = 255; //switch to int if too slow.
  bool ownsData = false; //dumb to have in each, but for now...
  Blend mode = Blend::Overlay; // could also have pointers for both alpha and mode, so optional and then
  //  always returning values 1.0 / OVERLAY? or better yet "DEFER" = Buffer-wide setting handles...

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
  /* Field() {} */
  Field(uint8_t size): Field(new uint8_t[size], size) { } // noo go gets to messy if we fuck with own allocs or smart ptrs
  Field(uint8_t* bufferStart, uint8_t size, uint16_t fieldIndex = 0, float alpha = 1.0, bool copy = false):
    _size(size) {
      setPtr(bufferStart, fieldIndex);
  }
  Field(const Field& rhs): Field(rhs.get(), rhs.size()) {  }
  ~Field() {}; // ~Field() { if(ownsData) delete[] data; }; //how goz dis when we dont created array, just using ptr?

  bool operator==(const Field& rhs) const { return memcmp(rhs.get(), data, _size); } //XXX add alpha check n shite
  bool operator!=(const Field& rhs) const { return !(*this == rhs); }
  bool operator<(const Field& rhs) const { return (average() < rhs.average()); }
  bool operator<=(const Field& rhs) const { return (average() <= rhs.average()); }
  bool operator>(const Field& rhs) const { return (average() > rhs.average()); }
  bool operator>=(const Field& rhs) const { return (average() >= rhs.average()); }

  uint8_t* operator[](uint8_t index) { return get(index); } //eh prob not so reasonable heh. but if want to be able to do f[0] = 5; f[1] = 30;
  Field& operator=(const Field& rhs) { setCopy(rhs.get()); return *this; } // lighten
  Field& operator+(const uint8_t delta) { value(delta, true); return *this; } // lighten
  /* Field& operator+(const Field& other) { */
  /*   uint8_t field[_size]; */
  /*   for(auto p=0; p<_size; p++) field[p] = constrain((int)data[p] + *other.get(p), 0, 255); */
  /*   return *(new Field(field, _size)); //XXX surely dumb and dangerous memleak fiesta?? */
  /* } */
  Field& operator++() { return value(1); } // lighten
  // WRONG, cant change in-place... Field& operator-(const uint8_t delta) { return value(delta, false); } // darken
  // Field& operator-(const uint8_t delta) {  } // darken
  //
  /* Field& operator-(const Field& other) { */
  /*   uint8_t field[_size]; */
  /*   for(auto p=0; p<_size; p++) field[p] = constrain((int)data[p] - *other.get(p), 0, 255); */
  /*   return *(new Field(field, _size)); //XXX surely dumb and dangerous memleak fiesta?? */
  /*   //return *Field(field, _size); */
  /* } */
  Field& operator--() { return value(-1); } // darken

  // might be overkill and that should do this directly in Buffer, but let's see
  Field& mix(Field& other, std::function<uint8_t(uint8_t, uint8_t)> op, bool perByte = true) { // could auto this even or?
    uint8_t* otherData = other.get();
    if(perByte) {
      for(auto i=0; i<_size; i++) data[i] = op(data[i], otherData[i]);
    } else { // compare "total" value and use accordingly...
    }
    return *this;
  }
  Field& add(Field& other) {
    return mix(other, [](uint8_t a, uint8_t b) { return constrain((int)a + b, 0, 255); });
  }
  Field& sub(Field& other) {
    return mix(other, [](uint8_t a, uint8_t b) { return constrain((int)a - b, 0, 255); });
  }
  Field& avg(Field& other, bool ignoreZero = true) {
    return mix(other, [ignoreZero](uint8_t a, uint8_t b) {
      uint8_t divisor = ignoreZero && (a + b == a || a + b == b)? 1: 2;
      return (uint8_t)(((int)a + b) / divisor); });
  }
  Field& htp(Field& other, bool perByte = true) {
    if(!perByte) {
      uint8_t a = average(), b = other.average();
      if(b > a) setCopy(other.get());
    }
    else return mix(other, [](uint8_t a, uint8_t b) { return max(a, b); });
  }
  Field& ltp(Field& other) {
    return setCopy(other.get()); //eh, i guess? return rightmost, for lack of timing metadata
  }
  Field& lotp(Field& other, bool perByte = true) {
    if(!perByte) {
      uint8_t a = average(), b = other.average();
      if(b < a) return setCopy(other.get());
      else return *this;
    }
    else return mix(other, [](uint8_t a, uint8_t b) { return min(a, b); });
  }

  uint8_t average() const { //average value of field elements or like?
    uint16_t cum = 0;
    for(auto p=0; p<_size; p++) cum += data[p];
    return (cum / _size);
  }
  Field& value(int adjustment) { //still 255 range, but +/-...
    for(auto p=0; p<_size; p++) data[p] = constrain((int)data[p] + adjustment, 0, 255);
    return *this;
  }
  Field& value(float absolute) {
    for(auto p=0; p<_size; p++) data[p] = absolute * 255;
    return *this;
  }
  Field& value(uint8_t delta, bool higher) {//lighten or darken as appropriate...
    int adjust = higher? delta: -(int)delta;
    for(auto p=0; p<_size; p++) data[p] = constrain((int)data[p] + adjust, 0, 255);
    return *this;
  }


  uint8_t* get(uint8_t offset = 0) const {
    return data + offset; //call oob or something otherwise.
  }
  Field& setCopy(uint8_t* newData) { //set data by copy. also need a copy constructor
    for(auto i=0; i<_size; i++) data[i] = newData[i];
    return *this;
  }
  Field& setPtr(uint8_t* bufferStart, uint16_t fieldIndex) { //set ptr, or copy block
    data = bufferStart + fieldIndex * _size;
    return *this;
  }
  Field& setSubField(uint8_t value, uint16_t index) { //set subfield
    data[index] = value;
    return *this;
  }
  uint8_t size() const { return _size; }


  void blend(const Field& target, uint8_t progress) {
    uint16_t targetWeight = progress + 1,
             ourWeight = 257 - targetWeight;
    for(uint8_t p = 0; p < _size; p++) {
      data[p] = ((data[p] * ourWeight) + (*target.get(p) * targetWeight)) >> 8; // works because rolls over I guess
    }
  }
  enum AlphaMode { MIX = 0, WEIGHT };
  /* void blend(const Field& target, float progress = 0.5, uint8_t alphaMode = MIX) { */
    /* if(alphaMode == MIX) { // alpha = (alpha * progress) + target.alpha * (1.0 - progress); */
    /* } else if(alphaMode == WEIGHT) { } // XXX a blend weighting by (not merely mixing) alpha */
  void blend(const Field& target, float progress = 0.5) {
    /* progress = constrain(progress, 0.0, 1.0); //tho past-1.0 should also work eventually */
    for(uint8_t p = 0; p < _size; p++) {
      /* data[p] = data[p] + ((*target.get(p) - (int)data[p]) * progress); // works because rolls over I guess */
      data[p] = data[p] + (*target.get(p) - data[p]) * progress; // works because rolls over I guess
    }
  }
  void blend(const Field& x1, const Field& y0, const Field& y1, float x = 0.5, float y = 0.5) {
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

