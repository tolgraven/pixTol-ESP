#pragma once

#include "config.h"
#include "envelope.h"
#include "field.h"
#include "color.h"


// template<typename T>
class Buffer {
  public:
    Buffer(): Buffer("Buffer", 1, 512) {}
    Buffer(const String& id, uint8_t _fieldSize, uint16_t fieldCount):
      _id(id), _fieldSize(_fieldSize), _fieldCount(fieldCount), ownsData(true)
      , data(new uint8_t[length()]{0}) {
      // data = new uint8_t[length()]{0};
    }
    Buffer(const String& id, uint8_t fieldSize, uint16_t fieldCount, uint8_t* dataPtr):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount), data(dataPtr), ownsData(true) {
    }
    ~Buffer() { if(ownsData) delete[] data; }

    const String& id() const { return _id; }
    uint8_t fieldSize(uint8_t newFieldSize = 0) {
      if(!newFieldSize) return _fieldSize;
      else {
        _fieldSize = newFieldSize;
        return _fieldSize;
      }
    }
    uint16_t fieldCount(uint16_t newFieldCount = 0) {
      if(!newFieldCount) return _fieldCount;
      else {
        _fieldCount = newFieldCount;
        return _fieldCount;
      }
    }
    uint16_t length() { return _fieldSize * _fieldCount; }

    uint8_t* outOfBounds() {
      LN.logf(__func__, LoggerNode::ERROR, "Buffer %s lacks valid buffer ptr, or out of range", _id.c_str());
      return nullptr;
    }

    void set(uint8_t* newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true) {
      copyLength == length()? copyLength: length(); //use internal size unless specified
      if(ownsData) memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
      else data = newData; // bc target shouldnt be offset... but needed for if buffer arrives in multiple dmx unis or whatever...  but make sure "read-offset" done in-ptr instead...

      updatedBytes += copyLength;
      if(updatedBytes >= length() && autoLock) lock();
      else dirty = true; //XXX no good, might have updated with some garbage prev...
    }
    void set(Buffer& from, uint16_t length = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true) {
      set(from.get(), length, readOffset, writeOffset, autoLock);
    }

    void setField(uint16_t index, uint8_t* newData) {
      if(index < length()) memcpy(data + index * _fieldSize, newData, _fieldSize);
      updatedBytes += _fieldSize;
    }

    void lock() { // finalize keyframe?, at that point can be flushed or interpolated against
      dirty = false;
      updatedBytes = 0;
    }

    // workflow becomes keep two buffer objects, "current" and "target", continously pass target to current to interpolate?  Or just do once each keyframe, save diff and reapply
    // Also save buffers continously and mix them together while rotating and manipulating each continousy... =)
    bool canGet() { return dirty; }

    uint8_t* get(uint16_t offset = 0) {
      if(!data || offset >= length()) return outOfBounds();
      return data + offset;
    }
    uint8_t* getField(uint16_t index) {
      if(!data || index >= _fieldCount) return outOfBounds();
      return data + index * _fieldSize;
    }

    // Buffer<& difference(Buffer& other, float fraction) const { // uh oh how handle negative?
    // Buffer<int>& difference(Buffer& other, float fraction) const {
    // DiffBuffer& difference(Buffer& other, float fraction) const {
    //   uint8_t* otherData = other.get();
    //   DiffBuffer diffBuf(_fieldSize, _fieldCount);
    //   for(uint16_t i=0; i < _fieldCount; i++) {
    //     for(uint8_t j=0; j < _fieldSize; j++) {
    //       int value = data[i+j] - otherData[i+j]; //value < 0? 0: value > 255? 255: value;
    //       diffBuf.set(&value, i+j);
    //     }
    //   }
    //   return diffBuf;
    // }
    void add(Buffer& other) {
      uint8_t* otherData = other.get();
      for(uint16_t i=0; i < length(); i++) {
        data[i] = data[i] + otherData[i] <= 255? data[i] + otherData[i]: 255;
      }
    }
    void sub(Buffer& other) {
      uint8_t* otherData = other.get();
      for(uint16_t i=0; i < length(); i++) {
        data[i] = data[i] - otherData[i] >= 0? data[i] - otherData[i]: 0;
      }
    }
    void average(Buffer& other) {
      uint8_t* otherData = other.get();
      for(uint16_t i=0; i < length(); i++) {
        uint16_t temp = data[i] + otherData[i];
        data[i] = temp/2;
      }
    }

    uint8_t averageInBuffer(uint16_t startField = 0, uint16_t endField = MILLION) { //calculate avg brightness for buffer. Or put higher and averageValue?
      //XXX offload value calc to field? bc diff fieldtypes might have more advanced scaling than linear 0-255?
      uint32_t tot = 0;
      endField = endField <= length()? endField: length();
      for(uint16_t byte = startField * _fieldCount; byte < endField; byte++) tot += byte;
      return tot / endField - startField;
    } // or static/elsewhere util fn passing bufptr, len, fieldsize?

  protected:
    String _id;
    uint8_t _fieldSize;
    uint16_t _fieldCount;
    bool ownsData;
    uint16_t updatedBytes = 0;
    uint8_t* data = nullptr;
    bool dirty = true;
  private:
};

class PixelBuffer: public Buffer {
  private:
    uint8_t* targetData = nullptr;
    Color* pixelData = nullptr;
    Color* targetPixelData = nullptr; //each Color's data* points to specific section of raw buffer, so no additional overhead apart from alpha
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels):
      Buffer("PixelBuffer", bytesPerPixel, numPixels),
      targetData(new uint8_t[length()]{0}) {}

    void setTarget();

    void blendWith(const PixelBuffer& buffer, float amount) { } // later add methods like overlay, darken etc
    // void blendUsingEnvelope(uint8_t* data, BlendEnvelope& e, float progress = 1.0); // should go here eventually
    // ^^ so since will keep buffers last, target, and current (interpolated, to output)
    // sep, this shouldn't mod in-place but return a new PixelBuffer.
    // but if targetData already in here, no need pass data at least. Just set core data, targetData,
    // call this getting new buffers...
    PixelBuffer& getInterpolated(BlendEnvelope& e, float progress = 1.0); // should go here eventually

    PixelBuffer& getScaled(uint16_t numPixels); // scale up or down... later xy etc

    Color averageColor(int startPixel = -1, int endPixel = -1) {} // -1 = no subset


  //based on adafruit, interpolate between two input buffers, gamma- and color-correcting the interpolated result with 16-bit dithering
  //XXX how make this work with overshooting? compute delta for average timeBetweenFrames?
  void interpolate(uint8_t *was, uint8_t *target, uint8_t progress);
  void tickInterpolation(uint32_t elapsed); // scheduler should keep track of timing, just tell how long since last tick...
};
