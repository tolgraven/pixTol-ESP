#pragma once

#include <NeoPixelBrightnessBus.h>
#include "renderstage.h"
#include "strip.h"

typedef iStripDriver iBufferDriver;
typedef StripRGB BufferRGB;
typedef StripRGBW BufferRGBW;

class Field { // make template? should be as lean as possible packing bits, ie BlendMode fits in half a byte...
  // so a field of fields probably makes more sense than individual objects
  // no premature optimization tho, make it fancy then trim later. But need one per field _per incoming stream_, plus end dest... lots
  // same type should store value + metadata, whether field is single float or 4-byte pixel
  enum BlendMode {
    OVERLAY = 0, HTP, LOWTP, SCALE, AVERAGE, ADD, SUBTRACT, MULTIPLY, DIVIDE
  }; // "SCALE" - like live "modulate", would need extra var setting zero-point (1.0 = existing value is maximum possible, 0.5 can reach double etc). Putting such meta-params on oscillators etc = likely exciting
  BlendMode mode;
  uint8_t size; //recycle enum from strip tho
  int16_t amount; //float prob, but slow?
};

// template<typename T>
class Buffer: public RenderStage {
  public:
    Buffer(): Buffer("Buffer", 1, 512) {}
    Buffer(const String& id, uint8_t fieldSize, uint16_t fieldCount):
      RenderStage(id, fieldSize, fieldCount), ownsData(true) {
      data = new uint8_t[dataLength];
      // tricky is i guess a buffer ought both to handle its own data, and just having
      // it point to some other location where it's already stored for whatever reason
    }
    Buffer(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t* dataPtr):
      RenderStage(id, fieldSize, fieldCount), data(dataPtr), ownsData(false) {
    }
    ~Buffer() { if(ownsData) delete[] data; }

    void set(uint8_t* incomingData, uint16_t length=512, uint16_t offset=0) {
      if(ownsData) {
          memcpy(&incomingData[offset], &data[offset], length); //or just set pointer but then we're just a passive observer
      } else {
        data = incomingData;
      }
      updatedBytes += length;
      if(updatedBytes >= dataLength) {
        finalize();
      } else {
        dirty = true;
      }

    }
    void finalize() { // lock in keyframe, at that point can be flushed or interpolated against
      dirty = false;
      updatedBytes = 0;
    }
    // hopefully ok performance - workflow becomes keep two
    // buffer objects, "current" and "target", continously pass target to current to interpolate?
    // Or just do once each keyframe, save diff and reapply
    // Also save buffers continously and mix them together while rotating and manipulating each continousy... =)
    bool canGet() { return dirty; }
    uint8_t* get(uint16_t* length, uint16_t offset=0) { // length=0 should automatically get full buffer, passed pointer is updated with actual
      if(!data || offset > dataLength) return nullptr;
      *length = dataLength;
      return &data[offset];
    }

  protected:
    uint8_t* data = nullptr;
    bool ownsData;
    bool dirty = true;
    uint16_t updatedBytes = 0;
};


class PixelBuffer: public Buffer {
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels): //
      Buffer("PixelBuffer", bytesPerPixel, numPixels) {
    /* } */
    /* PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataptr=nullptr): //  */
    /* PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels): // */
    /*   Buffer("some ID like", bytesPerPixel, numPixels, nullptr) { */

      driver = new StripRGBW(125);
      /* if(dataptr == nullptr) { */
      bufdata = driver->Pixels();
      /* } */
    }

    void blendWith(const PixelBuffer& buffer, float amount) {
    } // later add methods like overlay, darken etc

    PixelBuffer difference(const PixelBuffer& buffer, float fraction) const {
      /* uint16_t* buflen; */
      uint8_t* otherData = buffer.get();
      PixelBuffer diff(fieldSize, fieldCount); // = PixelBuffer(fieldSize, fieldCount);
      uint8_t* diffData = diff.get();
      for(uint16_t i=0; i < dataLength; i++) {
        diffData[i] = bufdata[i] - otherData[i];
      }
      return diff;

    }
    void add(const PixelBuffer& buffer) {
      uint8_t* otherData = buffer.get();
      for(uint16_t i=0; i < this->dataLength; i++) {
        bufdata[i] += otherData[i];
      }
    }
    // all below should actually be modulators, but do we wrap them like this for easy access? maybe
    // void rotate(int x, int y=0);
    // void shift(int x, int y=0);
    // void adjustHue(int degrees);
    // void adjustSaturation(int amount);
    // void adjustLightness(int amount); //makes sense?
    // void setGain(float amount=1.0f); // like brightness but at buffer (not strip) level, plus can go higher than full, to overdrive pixels...
    // void fill(ColorObject color);
    // void setGeometry(int16_t* howeveryoudoit);
    //
    // PixelBuffer operator+(const PixelBuffer& b);
    // PixelBuffer operator-(const PixelBuffer& b); // darken?
    // PixelBuffer operator*(const PixelBuffer& b); // average?
    // PixelBuffer operator/(const PixelBuffer& b); //
  private:
    /* iBufferDriver* driver; */
    iStripDriver* driver = nullptr;
};

