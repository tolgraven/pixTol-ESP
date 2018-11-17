#pragma once

#include <vector>
#include "config.h"
#include "envelope.h"
#include "field.h"
#include "color.h"
#include <NeoPixelBrightnessBus.h> //get rid asap, for unit testing...

// template<typename T>
class Buffer {
  public:
    Buffer(): Buffer("Buffer", 1, 512) {}
    Buffer(uint8_t* dataPtr, uint16_t fieldCount):
      Buffer("Buffer", 1, fieldCount, dataPtr) {}
    Buffer(Buffer& buffer, uint16_t numFields, uint16_t offset = 0):
      Buffer(buffer.id(), buffer.fieldSize(), numFields, buffer.get(offset), true) {}
    Buffer(const String& id, uint8_t fieldSize, uint16_t fieldCount,
           uint8_t* dataPtr = nullptr, bool copy = false):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount),
      // data(dataPtr), ownsData(!dataPtr) { //, field(new Field[fieldCount](data, fieldSize)) {
      // data(dataPtr), ownsData(!dataPtr), field(std::vector<Field*>(fieldCount, fieldSize)) {
      data(dataPtr), ownsData(!data || copy) {
        if(ownsData) {
          LN.logf(__func__, LoggerNode::DEBUG, "Buffer %s in charge of its data, fieldSize %u, fieldCount %u, length %u",
              _id.c_str(), _fieldSize, _fieldCount, length());
          data = new uint8_t[length()]{0};
        }
        if(dataPtr && copy) memcpy(data, dataPtr, length());
        // if(!length()) {
        //     LN.logf(__func__, LoggerNode::ERROR, "Buffer %s lacks valid length: %u. Setting fieldSize to 1...", _id.c_str(), length());
        //     _fieldSize = 1;
        //     // ^ XXX figure out why the fuck...
      LN.logf(__func__, LoggerNode::DEBUG, "Buffer %s deleted.", _id.c_str());
        // }

        // field = new Field[fieldCount];
        // for(auto i=0; i<fieldCount; i++) {
        //   field[i].size(fieldSize);
        //   field[i].set(data, i);
        // }
        // field = new std::vector<Field>(fieldCount, Field(fieldSize));
        // for(auto i=0; i<_fieldCount; i++) (*field)[i].set(data, i);

        // vfield.reserve(fieldCount);
        // for(auto i=0; i<_fieldCount; i++) vfield.emplace_back(data, fieldSize, i);

    }
    ~Buffer() {
      LN.logf(__func__, LoggerNode::DEBUG, "Buffer %s deleted.", _id.c_str());
      if(ownsData) delete[] data;
      // vfield.clear();
      // if(field) { delete[] field; }
    }

    const String& id() const { return _id; }
    uint8_t fieldSize(uint8_t newFieldSize = 0) {
      if(newFieldSize) _fieldSize = newFieldSize; // XXX adjust fields...
      return _fieldSize;
    }
    uint16_t fieldCount(uint16_t newFieldCount = 0) {
      if(newFieldCount) _fieldCount = newFieldCount;
      return _fieldCount;
    }
    uint16_t length() { return _fieldSize * _fieldCount; }

    void logProperties() {
      LN.logf(__func__, LoggerNode::DEBUG, "Buffer %s: fieldSize %u, fieldCount %u, length %u",
          _id.c_str(), _fieldSize, _fieldCount, length());
    }
    // uint8_t* outOfBounds() {
    void outOfBounds() {
      LN.logf(__func__, LoggerNode::ERROR, "Buffer s lacks data, or out of range.");
      logProperties();
      // return nullptr;
    }

    // dun work doing partial updates unless copying, obviously.
    Buffer& set(uint8_t* newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true) {
      copyLength = copyLength && copyLength < length()? copyLength: length(); //use internal size unless specified
      if(ownsData) memcpy(data + writeOffset, newData + readOffset, copyLength); // watch out offset, if using to eg skip past fnChannels then breaks
      else {
        uint8_t* writeAt = data + writeOffset;
        writeAt = newData + readOffset; // bc target shouldnt be offset... but needed for if buffer arrives in multiple dmx unis or whatever...  but make sure "read-offset" done in-ptr instead...
      }

      updatedBytes += copyLength; //or we count in fields prob?
      if(updatedBytes >= length() && autoLock) lock();
      else dirty = true; //XXX maybe no good, might have updated with some garbage prev and redone?
      return *this;
    }
    Buffer& set(Buffer& from, uint16_t length = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true) {
      set(from.get(), length, readOffset, writeOffset, autoLock);
      return *this;
    }

    void setField(const Field& source, uint16_t fieldIndex) { //XXX also set alpha etc - really copy entire field, but just grab data at ptr
      if(fieldIndex >= _fieldCount) return;
      memcpy(data + fieldIndex * _fieldSize, source.get(), _fieldSize);
      updatedBytes += _fieldSize;
    }

    void lock() { // finalize keyframe?, at that point can be flushed or interpolated against
      dirty = false;
      updatedBytes = 0;
    }

    bool ready() { return dirty; }

    uint8_t* get(uint16_t offset = 0) {
      if(!data || offset >= length()) outOfBounds();
      return data + offset;
    }
    // Field& getField(uint16_t fieldIndex) {
    //   if(!data || fieldIndex >= _fieldCount) fieldIndex = 0;
    //   // return (*field)[fieldIndex];
    //   // return field[fieldIndex];
    //   return vfield[fieldIndex];
    // }

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

    // nice this! will need equiv over Field/Color later.
    Buffer& mix(Buffer& other, std::function<uint8_t(uint8_t, uint8_t)> op) { // could auto this even or?
      uint8_t* otherData = other.get();
      for(auto i=0; i < length(); i++) data[i] = op(data[i], otherData[i]);
      return *this;
    }
    // Buffer& operator+(const Buffer& other) {
    Buffer& add(Buffer& other)  { return mix(other, [](uint8_t a, uint8_t b) {
        return (uint8_t)constrain((int)a + (int)b, 0, 255); }); }
    Buffer& sub(Buffer& other)  { return mix(other, [](uint8_t a, uint8_t b) {
        return (uint8_t)constrain((int)a - (int)b, 0, 255); }); }
    Buffer& avg(Buffer& other)  { return mix(other, [](uint8_t a, uint8_t b) {
        return (uint8_t)((int)a + (int)b) / 2; }); }
    Buffer& htp(Buffer& other)  { return mix(other, [](uint8_t a, uint8_t b) {
        return max(a, b); }); }
    Buffer& ltp(Buffer& other)  { return mix(other, [](uint8_t a, uint8_t b) {
        return b; }); }
    Buffer& lotp(Buffer& other) { // only for values above 0, or would always end at zero when blending with init frame of 0. Tho if was perfectly lining up sacn+artnet+whatever else and doing everything frame by frame, sure. but yeah nah.
      return mix(other, [](uint8_t a, uint8_t b) {
        if(a && b) return min(a, b); //both above zero
        else if(a && !b) return a;
        else if(!a && b) return b;
        else return a;
        });
    }

    uint8_t averageInBuffer(uint16_t startField = 0, uint16_t endField = MILLION) { //calculate avg brightness for buffer. Or put higher and averageValue?
      uint32_t tot = 0;
      endField = endField <= length()? endField: length();
      // for(auto& f: vfield) tot += f.average();
      // for(auto f: field) tot += f->average();
      // for(auto i=0; i<_fieldCount; i+=_fieldSize) tot += (*f)[i].average();
      // for(auto i=0; i<_fieldCount; i+=_fieldSize) tot += f[i].average();
      return tot / (endField - startField);
    }

  protected:
    String _id;
    uint8_t _fieldSize;
    uint16_t _fieldCount;
    bool ownsData;
    uint16_t updatedBytes = 0;
    uint8_t* data = nullptr;
    // Field* field = nullptr;
    // std::vector<Field>* field;
    // std::vector<Field> vfield; // empty
    float alpha = 1.0; //or weight, or something...
    bool dirty = true;
  private:
};


class PixelBuffer: public Buffer {
  private:
    uint8_t* targetData = nullptr;
    Color* pixelData = nullptr;
    Color* targetPixelData = nullptr; //each Color's data* points to specific section of raw buffer, so no additional overhead apart from alpha
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataPtr = nullptr):
      Buffer("PixelBuffer", bytesPerPixel, numPixels, dataPtr)
      //, targetData(new uint8_t[length()]{0}) {} //memleak lul
       {}

    void setTarget();

    Color& setPixelColor(uint16_t pixel, const Color& color);
    Color& getPixelColor(uint16_t pixel);

    void blendWith(const PixelBuffer& buffer, float amount) { } // later add methods like overlay, darken etc
    // void blendUsingEnvelope(uint8_t* data, BlendEnvelope& e, float progress = 1.0); // should go here eventually
    // ^^ so since will keep buffers last, target, and current (interpolated, to output)
    // sep, this shouldn't mod in-place but return a new PixelBuffer.
    // but if targetData already in here, no need pass data at least. Just set core data, targetData,
    // call this getting new buffers...
    // PixelBuffer& getInterpolated(BlendEnvelope& e, float progress = 1.0); // should go here eventually
    void blendUsingEnvelope(PixelBuffer& origin, PixelBuffer& target, BlendEnvelope& e, float progress) { // XXX also pass fraction in case interpolating >2 frames
      int pixel = 0;
      // Buffer* result = new Buffer("current", _fieldSize, fieldCount);
      // float brightnessFraction = 255 / (brightness? brightness: 1); // ahah can't believe divide by zero got me

      for(int t=0; t<length(); t+=_fieldSize, pixel++) {
        // if(_fieldSize == 3) { // when this is moved to input->pixelbuffer stage there will be multiple configs: format of input (RGB, RGBW, HSL etc) and output (WS/SK). So all sources can be used with all endpoints.
        //   RgbColor targetColor = RgbColor(target[t+0], target[t+1], target[t+2]);
        //   RgbColor originColor = RgbColor(origin[t+0], origin[t+1], origin[t+2]);
        //   bool brighter = targetColor.CalculateBrightness() >
        //                   originColor.CalculateBrightness(); // handle any offset from lowering dimmer
        //   color = RgbColor::LinearBlend(originColor, targetColor,
        //           (brighter? e.A(progress): e.R(progress)));
        // } else
        if(_fieldSize == 4) {
          RgbwColor originColor = RgbwColor(*origin.get(t+0), *origin.get(t+1), *origin.get(t+2), *origin.get(t+3));
          RgbwColor targetColor = RgbwColor(*target.get(t+0), *target.get(t+1), *target.get(t+2), *target.get(t+3));
          bool brighter = targetColor.CalculateBrightness() > originColor.CalculateBrightness(); // handle any offset from lowering dimmer
          RgbwColor color = RgbwColor::LinearBlend(originColor, targetColor,
                  (brighter? e.A(progress): e.R(progress)));
          data[t+0] = color.G; // grbw? since writing dir to buf...
          data[t+1] = color.R;
          data[t+2] = color.B;
          data[t+3] = color.W;
          // result->set(&color.R, 1, 0, 0);
          // result->set(&color.G, 1, 0, 1);
          // result->set(&color.B, 1, 0, 2);
          // result->set(&color.W, 1, 0, 3);
        }
      }
    }

    PixelBuffer& getScaled(uint16_t numPixels); // scale up or down... later xy etc

    Color averageColor(int startPixel = -1, int endPixel = -1) {} // -1 = no subset


  //based on adafruit, interpolate between two input buffers, gamma- and color-correcting the interpolated result with 16-bit dithering
  //XXX how make this work with overshooting? compute delta for average timeBetweenFrames?
  void interpolate(uint8_t *was, uint8_t *target, uint8_t progress);
  void tickInterpolation(uint32_t elapsed); // scheduler should keep track of timing, just tell how long since last tick...
};
