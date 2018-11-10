#pragma once

// class Field { // make template? should be as lean as possible packing bits, ie BlendMode fits in half a byte...
//   // so a field of fields probably makes more sense than individual objects
//   // no premature optimization tho, make it fancy then trim later. But need one per field _per incoming stream_, plus end dest... lots
//   // same type should store value + metadata, whether field is single float or 4-byte pixel
//   // BlendMode mode;
//   uint8_t size;
//   int16_t alpha;
// };
enum class FieldSize: uint8_t {
  SINGLE = 1, DMX = 1, BYTE = 1, WHITE = 1, UV = 1, //etc
  DMX_FINE = 2, TWO_BYTE = 2,
  RGB = 3, RGBW = 4
};

enum class Blend {
  OVERLAY = 0, HTP, LOTP, AVERAGE, ADD, SUBTRACT, MULTIPLY, DIVIDE
}; // not suitable tho?: "SCALE" - like live "modulate", would need extra var setting zero-point (1.0 = existing value is maximum possible, 0.5 can reach double etc). Putting such meta-params on oscillators etc = likely exciting

struct FieldFlags { //in case need optimize to pack individual flags/whatever into common bitfield, then would put in Buffer...
  uint8_t* data;
  FieldFlags(uint16_t fieldCount): data(new uint8_t[fieldCount / 2]) { //assuming 8 modes = 4 bits = 2 fields per byte
      //code to set/read individual indexes of bitfield
  }
};

class Field {
  uint8_t* data = nullptr;
  FieldSize size; // uint8_t size;
  float alpha = 1.0; //uint8_t alpha = 255; //switch to int if too slow.
  Blend mode = Blend::OVERLAY; // could also have pointers for both alpha and mode, so optional and then
  //  always returning values 1.0 / OVERLAY? or better yet "DEFER" = Buffer-wide setting handles...

  // OR
  // struct Alpha {
  //   private:
  //     uint8_t alpha = 255;
  //     Blend mode = Blend::OVERLAY;
  //     // OR
  //     // using blendFlags_t = uint16_t; and *fancy math to use four bits for mode flag and 12 for alpha??*
  //   Blend(float fractionAlpha = 1.0, Blend mode = OVERLAY): mode(mode) {
  //     fractionAlpha = fractionAlpha >= 1.0? 1.0: fractionAlpha <= 0.0? 0.0: fractionAlpha;
  //     alpha = fractionAlpha*255;
  //   }
  // };
  // Alpha alpha;

  public:
  Field(uint8_t value, float alpha = 1.0):
    Field(&value, 1, 1.0, true) {}
  // Field(uint8_t* field, uint8_t size, float alpha = 1.0, bool copy = false):
  Field(uint8_t* data, FieldSize size, float alpha = 1.0, bool copy = false):
    size(size), alpha(alpha) {
    if(copy) {
      data = new uint8_t[size];
      memcpy(data, data, size);
    } else {
      data = data;
    }
  }
  ~Field() { delete[] data; }; //how goz dis when we dont created array, just using ptr?

  bool operator==(const Field& field) const { return memcmp(field.get(), data, size); };
  bool operator!=(const Field& field) const { return !(*this == field); };
  Field& operator+(const uint8_t delta) { value(delta, true); return *this; } // lighten
  Field& operator+(const Field& other) {
    // uint8_t field[size];
    uint8_t* field = new uint8_t[size];
    for(uint8_t p = 0; p < size; p++) {
      int cum = data[p] + other.get()[p];
      field[p] = cum > 255? 255: cum < 0? 0: cum;
    }
    return Field(field, size);
  }
  Field& operator-(const uint8_t delta) { value(delta, false); return *this; } // darken
  Field& operator-(const Field& other) {
    uint8_t* field = new uint8_t[size];
    for(uint8_t p = 0; p < size; p++) {
      int cum = data[p] - other.get()[p];
      field[p] = cum > 255? 255: cum < 0? 0: cum;
    }
    return Field(field, size);
  }

  uint8_t value() const {
    uint16_t cum = 0;
    for(uint8_t p = 0; p < size; p++)
      cum += data[p];
    return (cum / size);
  }
  Field& value(int adjust) { //still 255 range, but +/-...
    for(uint8_t p = 0; p < size; p++) {
      int sum = data[p] + adjust;
      data[p] = sum > 255? 255: sum < 0? 0: sum;
    }
    return *this;
  }
  void value(float absolute) {
    for(uint8_t p = 0; p < size; p++)
      data[p] = absolute*255; //(uint8_t)
  }
  uint8_t value(uint8_t delta, bool higher); //lighten or darken as appropriate...

  uint8_t* get(uint8_t offset = 0) const {
    if(offset > 0 && offset < size) return data + offset; //data[subPixel];
  }

  enum AlphaMode { MIX = 0, WEIGHT };
  void blend(const Field& target, float progress = 0.5, uint8_t alphaMode = MIX) {
    progress = progress > 1.0? 1.0: progress < 0.0? 0.0: progress;
    if(alphaMode == MIX) {
      alpha = (alpha * progress) + target.alpha * (1.0 - progress);
    } else if(alphaMode == WEIGHT) {
    } // XXX a blend weighting by (not merely mixing) alpha
    // ^^ wtf does mean??
    for(uint8_t p = 0; p < size; p++) {
      data[p] = data[p] + ((*target.get(p) - data[p]) * progress); // works because rolls over I guess
    }
  }
  void blend(const Field& x1, const Field& y0, const Field& y1, float x = 0.5, float y = 0.5) {
    float wx0 = (1.0f - x) * (1.0f - y);
    float wx1 = x * (1.0f - y);
    float wy0 = (1.0f - x) * y;
    float wy1 = x * y;
    for(uint8_t p = 0; p < size; p++) {
      data[p] = data[p] * wx0 + *x1.get(p) * wx1 + *y1.get(p) * wy0 + *y1.get(p) * wy1;
    }
    alpha = alpha * wx0 + x1.alpha * wx1 + y1.alpha * wy0 + y1.alpha * wy1;
  }
};
