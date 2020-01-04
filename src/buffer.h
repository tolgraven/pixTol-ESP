#pragma once

// TODO turn into lib
#include <vector>
#include <limits>
#include <field.h>
#include "envelope.h"
#include "color.h"

/* class Thingy { */
/*     String _id, _type; //or name? want for eg artnet, sacn, strip, shorter stuff not full desc */
/*     uint8_t _fieldSize; */
/*     uint16_t _fieldCount; */
/* }; */
// will reasonably want:
// uint8_t, uint16_t, int (for deltas), float
// likely gotta mod lots for anything but bytes to actually function tho...
template<class T> class iBuffer { //rename tBuffer?
  public:
    iBuffer(T* dataPtr, uint16_t fieldCount):
      iBuffer("Buffer", 1, fieldCount, dataPtr) {}

    iBuffer(iBuffer& buffer, uint16_t numFields, uint16_t offset = 0, bool copy = true):
      iBuffer(buffer.id(), buffer.fieldSize(), numFields, buffer.get(offset), copy) {}

    iBuffer(const String& id, uint8_t fieldSize, uint16_t fieldCount,
           T* dataPtr = nullptr, bool copy = false):
      _id(id), _fieldSize(fieldSize), _fieldCount(fieldCount), ownsData(!dataPtr || copy),
      data(dataPtr) {
        if(!data || ownsData) {
          data = new T[length()]; // complains about non-const, also 0 might not work for all Ts anyways alloc mem if not passed valid ptr
          memset(data, 0, lengthBytes());
        }
        if(dataPtr && copy)
          memcpy(data, dataPtr, lengthBytes());
    }
    virtual ~iBuffer() {
      setDithering(false);
      if(ownsData) delete[] data;
    }

    const String& id() const { return _id; }
    uint8_t fieldSize() const { return _fieldSize; }
    void setFieldSize(uint8_t newFieldSize) { _fieldSize = newFieldSize; } // XXX adjust fields...
    uint16_t fieldCount() const { return _fieldCount; }
    void setFieldCount(uint16_t newFieldCount) { _fieldCount = newFieldCount; }
    uint16_t length() const { return _fieldSize * _fieldCount; } // should be called totalSize or sizeInUnits or some shit dunno
    uint16_t lengthBytes() const { return length() * sizeof(T); }

    // fuck these tho
    void logProperties() const {
      /* lg.logf(__func__, Log::DEBUG, "Buffer %s: fieldSize %u, fieldCount %u, length %u\n", */
      /*         _id.c_str(), _fieldSize, _fieldCount, length()); */
    }
    void outOfBounds() const {
      /* lg.logf(__func__, Log::ERROR, "Buffer %s lacks data, or out of range.\n",_id.c_str()); */
      logProperties();
    }

    void setCopy(const T* const newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0);
    void setCopy(const iBuffer& newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0);

    void setPtr(T* const newData, uint16_t readOffset = 0);
    void setPtr(const iBuffer& newData, uint16_t readOffset = 0);

    void set(T* const newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true);
    void set(const iBuffer& from, uint16_t bytesLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true);

    void setByField(const iBuffer& from, uint16_t numFields = 0, uint16_t readOffsetField = 0, uint16_t writeOffsetField = 0, bool autoLock = true);
    void setField(const Field& source, uint16_t fieldIndex); //XXX also set alpha etc - really copy entire field, but just grab data at ptr

    T* get(uint16_t byteOffset = 0) const; //does raw offset make much sense ever? still, got getField.get() so...
    Field getField(uint16_t fieldIndex) const; // i mean untrue bc prob get field to mod something but eh.

    /* Field& adjustField(int fieldIndex, const String& action, T value) { */
    /*   return Field(data, fieldSize(), fieldIndex); */ /*   // XXX */
    /* } */
    void fill(const Field& source, uint16_t from = 0, uint16_t to = 0);
    void gradient(const Field& start, const Field& end, uint16_t from = 0, uint16_t to = 0);

    void rotate(int count, uint16_t first = 0, uint16_t last = 0);
    void shift(int count, uint16_t first = 0, uint16_t last = 0);
    void rotate(int fields) { }
    void rotate(float fraction) { rotate(fraction / fieldCount()); }

    void setDithering(bool on = true);
    /* void setSourceSubFieldOrder(uint8_t subFields[]) { */
    /*   delete sourceSubFieldOrder; */
    /*   sourceSubFieldOrder = new uint8_t[fieldSize()]; */
    /*   memcpy(sourceSubFieldOrder, subFields, fieldSize()); */
    /* } */
    void setDestinationSubFieldOrder(uint8_t subFields[]) {
      delete destinationSubFieldOrder;
      destinationSubFieldOrder = new uint8_t[fieldSize()];
      memcpy(destinationSubFieldOrder, subFields, fieldSize());
    }
    uint8_t subFieldForIndex(uint8_t sub) {
      if(!destinationSubFieldOrder) return sub;
      return destinationSubFieldOrder[sub];
    }


    void lock(bool state = true); // finalize keyframe?, at that point can be flushed or interpolated against
    bool ready() const {
      return true; /* return dirty; */ // havent actually thought through or properly sorted it, so for now... shouldnt be fucking needed we're not multi threaded lol
    }

    void setGain(float gain) { _gain = max(0.0f, gain); } // 0-1 acts like brightness/dimmer, >1 drives values higher...
    float getGain() const { return _gain; } // 0-1 acts like brightness/dimmer, >1 drives values higher...
    void applyGain();
    // iBuffer& flip(bool state = true) { _flip = state; return *this; }
    // iBuffer<& difference(iBuffer& other, float fraction) const { // uh oh how handle negative?
    // iBuffer<int>& difference(iBuffer& other, float fraction) const {
    // DiffBuffer& difference(iBuffer& other, float fraction) const;

    // nice this! will need equiv over Field/Color later.  XXX and fixem for T... but mainly just defer to Field
    void mix(iBuffer& other, std::function<T(T, T)> op) { // could auto this even or?
      T* otherData = other.get();
      for(auto i=0; i < length(); i++) data[i] = op(data[i], otherData[i]);
    }
    void mix(iBuffer& other, std::function<T(T, T, float)> op, float progress) { // could auto this even or?
      T* otherData = other.get();
      for(auto i=0; i < length(); i++) data[i] = op(data[i], otherData[i], progress);
    }
    // iBuffer& operator+(const iBuffer& other) {
    // but how determine, like uint8_t we use int16_t to avoid rollover, uint16_t would need int32_t...  for now: int32_t, but so wasteful...
    void add(iBuffer& other) {
      mix(other, [](T a, T b) {
        return (T)constrain((int32_t)a + (int32_t)b, 0, maxValue);
      });
    }
    // iBuffer& addBounce(iBuffer& other) { return mix(other, [](T a, T b) {
    //     return (T)constrain((int32_t)a + (int32_t)b, 0, maxValue); }); }
    void sub(iBuffer& other) {
      mix(other, [](T a, T b) {
        return (T)constrain((int32_t)a - (int32_t)b, 0, maxValue);
      });
    }
    // void subBounce(iBuffer& other) { return mix(other, [](T a, T b) {
    //     return (T)constrain((int32_t)a - (int32_t)b, 0, maxValue); }); }
    void avg(iBuffer& other, bool ignoreZero = true) {
      mix(other, [ignoreZero](T a, T b) {
        // return (T)(((int32_t)a + (int32_t)b) / 2); }); } //watch for overflows, got me once...
        int32_t sum = a + b;
        if(ignoreZero && (sum == a || sum == b))
          return (T)sum;
        return (T)(sum / 2);
        /* if(ignoreZero && (a + b == a || a + b == b)) */
        /*   divisor = 1; */
        /* return (T)(((int)a + b) / divisor); */
      });
    }
    void htp(iBuffer& other) { return mix(other, [](T a, T b) { return max(a, b); }); }
    void ltp(iBuffer& other) { return mix(other, [](T a, T b) { return b; }); }
    void lotp(iBuffer& other, bool ignoreZero = true) { // only for values above 0, or would always end at zero when blending with init frame of 0. Tho if was perfectly lining up sacn+artnet+whatever else and doing everything frame by frame, sure. but yeah nah.
      mix(other, [ignoreZero](T a, T b) {
        if(!ignoreZero || (a && b)) return min(a, b); //both above zero
        if(a && !b) return a;
        if(!a && b) return b;
        return a; //0
      });
    }
    /* T averageInBuffer(uint16_t startField = 0, uint16_t endField = 0); //calculate avg brightness for buffer. Or put higher and averageValue? */
    void blendUsingEnvelopeB(const iBuffer& origin, const iBuffer& target, float progress, BlendEnvelope* e = nullptr);
    void interpolate(const iBuffer& origin, const iBuffer& target, float progress, BlendEnvelope* e = nullptr);
    /* void interpolate(iBuffer& origin, iBuffer& target, uint8_t progress); */

  protected:
    String _id;
    uint8_t _fieldSize;
    uint16_t _fieldCount;
    bool ownsData;
    T* data = nullptr;
    T* _residuals = nullptr;
    uint8_t* sourceSubFieldOrder = nullptr;
    uint8_t* destinationSubFieldOrder = nullptr;
    const static uint32_t maxValue = std::numeric_limits<T>::max();
    float alpha = 1.0; //or weight, or something...
    float _gain = 1.0;
    /* T blackPoint = 6 * (maxValue / 255); */
    T blackPoint = 4;
    bool dirty = false;
    uint16_t updatedBytes = 0; //rename updatedFields?
  private:
};

using Buffer    = iBuffer<uint8_t>;
using Buffer8   = iBuffer<uint8_t>;
/* using BufferI   = iBuffer<int>; */
/* using Buffer16  = iBuffer<uint16_t>; */
/* using BufferF   = iBuffer<float>; */

// question is I guess, do we truly need PixelBuffer if Buffer contains Fields, which can be Colors,
// and which themselves handle blending operations?
// Usecase prob gamma shit and geometry whatever..
class PixelBuffer: public Buffer {
  public:
    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataPtr = nullptr):
      Buffer("PixelBuffer", bytesPerPixel, numPixels, dataPtr) {}
    PixelBuffer(Buffer& buffer):
      Buffer("PixelBuffer " + buffer.id(), buffer.fieldSize(), buffer.fieldCount(), buffer.get()) {}

    /* Color setPixelColor(uint16_t pixel, const Color& color); */
    /* Color getPixelColor(uint16_t pixel); */

    PixelBuffer& getScaled(uint16_t numPixels); // scale up or down... later xy etc
    //Color averageColor(int startPixel = -1, int endPixel = -1) {} // -1 = no subset

  //XXX how make this work with overshooting? compute delta for average timeBetweenFrames?
  GammaCorrection* gamma = nullptr;
  void interpolate(PixelBuffer* origin, PixelBuffer* target, float progress, GammaCorrection* gamma);
};
