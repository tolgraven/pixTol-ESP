#pragma once

// TODO turn into lib
#include <vector>
#include <limits>
#include <type_traits>

#include <field.h>
#include "base.h"
#include "envelope.h"


template<class T, class T2 = T> // For integer types, T2 should generally be the next larger signed one. uint8_t -> int16_t etc.
class iBuffer: public Named, public ChunkedContainer { //rename tBuffer?
  public:
    iBuffer(const String& id, uint8_t fieldSize, uint16_t fieldCount,
            T* const dataPtr = nullptr, bool copy = false):
      Named(id, "Buffer"), ChunkedContainer(fieldSize, fieldCount),
      ownsData(!dataPtr || copy), data(!ownsData? dataPtr: nullptr) {

        if(!data) {
          data = new T[length()]; // complains about non-const, also 0 might not work for all Ts anyways alloc mem if not passed valid ptr
          memset(data, 0, lengthBytes());
        }
        if(dataPtr && copy) {
          setCopy(dataPtr, lengthBytes());
        }
    }
    virtual ~iBuffer() {
      setDithering(false);
      if(ownsData) delete[] data;
    }
    // eh now sure what do afa  default copy/move constr stuff...
    iBuffer(const iBuffer& buffer, uint16_t numFields = 0, uint16_t offset = 0, bool copy = false):
      iBuffer(buffer.id() + " clone", buffer.fieldSize(),
              numFields > 0? numFields: buffer.fieldCount(),
              buffer.get(offset), copy) {}


    T&        operator[](uint16_t index) const { return *get(index); } //eh prob not so reasonable heh. but if want to be able to do f[0] = 5; f[1] = 30;
    iBuffer&  operator+(const iBuffer& rhs) { return add(rhs); }
    iBuffer&  operator+(T2 delta) { return value(delta); } // Field has some ops we should borrow
    iBuffer&  operator-(const iBuffer& rhs) { return sub(rhs); } // Field has some ops we should borrow
    iBuffer&  operator-(T2 delta) { return value(delta); } // Field has some ops we should borrow
    // iBuffer&  operator=(const iBuffer& rhs) { return *this; } // I guess? or delete? was invoking copy constructor when doing auto ... = ... But then that's an issue of other side I guess - returns a reference but then copied afterwards.
    iBuffer&  operator=(const iBuffer& rhs) = delete;
    // iBuffer&  operator*() {}
    T*  operator*() const { return get(); }; //does raw offset make much sense ever? still, got getField.get() so...

    virtual size_t printTo(Print& p) const override {
      return Named::printTo(p) + ChunkedContainer::printTo(p) + p.printf("this %p, data %p\n", this, this->get());
    }

    uint16_t lengthBytes() const { return length() * sizeof(T); }

    void setScale(T2 min, T2 max) { minValue = min; maxValue = max; } // auto set to range of datatype but manual for eg unit float

    void setCopy(const T* const newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0);
    void setCopy(const iBuffer& newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0);

    void setPtr(T* const newData, uint16_t readOffset = 0);
    void setPtr(const iBuffer& newData, uint16_t readOffset = 0);

    void set(T* const newData, uint16_t copyLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true);
    void set(const iBuffer& from, uint16_t bytesLength = 0, uint16_t readOffset = 0, uint16_t writeOffset = 0, bool autoLock = true);

    void setByField(const iBuffer& from, uint16_t numFields = 0, uint16_t readOffsetField = 0, uint16_t writeOffsetField = 0, bool autoLock = true);
    void setField(const Field& source, uint16_t fieldIndex); //XXX also set alpha etc - really copy entire field, but just grab data at ptr

    T* get(uint16_t unitOffset = 0) const { return this->data + unitOffset; } //does raw offset make much sense ever? still, got getField.get() so...
    Field getField(uint16_t fieldIndex) const { return Field(data, fieldSize(), fieldIndex); }; //  // elide copy constructor - is straight
    iBuffer getSubsection(uint16_t startField, uint16_t endField = 0) const;

    void fill(const Field& source, uint16_t from = 0, uint16_t to = 0);
    void gradient(const Field& start, const Field& end, uint16_t from = 0, uint16_t to = 0);

    void rotate(int count, uint16_t first = 0, uint16_t last = 0);
    void shift(int count, uint16_t first = 0, uint16_t last = 0);
    void rotate(float fraction) { rotate((int)(fraction * fieldCount())); }

    void setDithering(bool on = true);

    bool hasNew() { return dirty(); }
    bool dirty() { return _dirty; }
    void setDirty(bool state = true) { _dirty = state; } // nicer if ext cant fuck with but hardly possible

    void finishFrame() { setDirty(false); }
    void lock(bool state = true); // finalize keyframe?, at that point can be flushed or interpolated against
    // bool ready() const { return true; }/* return dirty; */ // havent actually thought through or properly sorted it, so for now... shouldnt be fucking needed we're not multi threaded lol

    void setGain(float gain) { _gain = max(0.0f, gain); } // 0-1 acts like brightness/dimmer, >1 drives values higher...
    float getGain() const { return _gain; } // 0-1 acts like brightness/dimmer, >1 drives values higher...
    void applyGain(); // since destructive / can only do once per refresh...
    // iBuffer& flip(bool state = true) { _flip = state; return *this; }
    // iBuffer& difference(iBuffer& other, float fraction) const { // uh oh how handle negative?
    // iBuffer<int>& difference(iBuffer& other, float fraction) const {
    // DiffBuffer& difference(iBuffer& other, float fraction) const;

    // nice this! will need equiv over Field/Color later.  XXX and fixem for T... but mainly just defer to Field
    iBuffer& mix(const iBuffer& other, std::function<T(T, T)> op) { // could auto this even or?
      for(auto i=0; i < length(); i++)
        data[i] = op(data[i], other[i]); // no way this inlined tho so would want loop inside lambda to actually use...
      return *this;
    }
    iBuffer& mix(const iBuffer& other, std::function<T(T, T, float)> op, float progress) { // could auto this even or?
      for(auto i=0; i < length(); i++)
        data[i] = op(data[i], other[i], progress);
      // might then wanna swap float to T for actual calcs tho?
      return *this;
    }
    iBuffer& add(const iBuffer& other) {
      return mix(other, [this](T a, T b) {
        // return (T)std::clamp((T2)a + (T2)b, minValue, maxValue); });
        return (T)constrain((T2)a + (T2)b, minValue, maxValue); });
    }
    iBuffer& addBounce(iBuffer& other) {
      return mix(other, [this](T a, T b) {
          // T2 sum = (T2)a + (T2)b; // no need for these early casts no?
          T2 sum = a + b; // no need for these early casts no?
          if(sum > maxValue) sum = maxValue - sum;
        return (T)sum; });
    }

    iBuffer& sub(const iBuffer& other) {
      return mix(other, [this](T a, T b) {
        // return (T)std::clamp(a - (T2)b, minValue, maxValue); });
        return (T)std::clamp(T2(a - b), minValue, maxValue); }); // so uT - uT turns iT, then coerce to T2, uh
        // return (T)std::clamp((T2)a - (T2)b, minValue, maxValue); });
        // return (T)constrain((T2)a - (T2)b, minValue, maxValue); });
    }
    iBuffer& subBounce(const iBuffer& other) {
      return mix(other, [](T a, T b) {
        return (T)abs((T2)a - (T2)b); });
    }

    iBuffer& avg(const iBuffer& other, bool ignoreZero = true) {
      return mix(other, [this, ignoreZero](T a, T b) {
        T2 sum = a + b;
        if(ignoreZero && (sum == a || sum == b))
          return (T)std::clamp(sum, minValue, maxValue);
          // return (T)constrain(sum, minValue, maxValue);
        return (T)(sum / 2);
      });
    }
    iBuffer& htp(const iBuffer& other) { return mix(other, [](T a, T b) { return max(a, b); }); }
    iBuffer& ltp(const iBuffer& other) { return mix(other, [](T a, T b) { return b; }); } //haha dumbest equivalent of copy really
    iBuffer& lotp(const iBuffer& other, bool ignoreZero = true) { // only for values above 0, or would always end at zero when blending with init frame of 0. Tho if was perfectly lining up sacn+artnet+whatever else and doing everything frame by frame, sure. but yeah nah.
      return mix(other, [ignoreZero](T a, T b) {
        if(!ignoreZero || (a && b)) return min(a, b); //both above zero
        return ((a && !b)? a:   (!a && b)? b:   a);  });
    }
    iBuffer& value(T2 adjustment) {
      for(auto p=0; p < length(); p++)
        // data[p] = std::clamp((T2)data[p] + adjustment, minValue, maxValue);
        data[p] = std::clamp(T2(data[p] + adjustment), minValue, maxValue);
        // data[p] = constrain((T2)data[p] + adjustment, minValue, maxValue);
      return *this;
    }
    T averageInBuffer(uint16_t startField = 0, uint16_t endField = 0); //calculate avg brightness for buffer. Or put higher and averageValue?
    void blendUsingEnvelopeB(const iBuffer& origin, const iBuffer& target, float progress, BlendEnvelope* e = nullptr);
    void interpolate(const iBuffer& origin, const iBuffer& target, float progress, BlendEnvelope* e = nullptr);

  protected:
    bool ownsData;
    T* data = nullptr;
    T* _residuals = nullptr;
    T2 maxValue = std::numeric_limits<T>::max(); // static might have to go as well if might want some different ones for floats
    T2 minValue = std::numeric_limits<T>::min(); //generally 0
    float alpha = 1.0; //or weight, or something...
    float _gain = 1.0;
    /* T blackPoint = 6 * (maxValue / 255); */
    T blackPoint = 0.016f * maxValue, whitePoint = maxValue * 0.95f;
    bool _dirty = false;
    // uint16_t updatedBytes = 0; //rename updatedFields?
    // bool newData = false;
  private:
};

using Buffer    = iBuffer<uint8_t, int16_t>; // first is actual, second to avoid overflow on ops...
using Buffer16  = iBuffer<uint16_t, int32_t>;
using BufferF   = iBuffer<float>;

// question is I guess, do we truly need PixelBuffer if Buffer contains Fields, which can be Colors,
// and which themselves handle blending operations?
// Usecase prob gamma shit and geometry whatever..
//class PixelBuffer: public Buffer {
//  public:
//    PixelBuffer(uint8_t bytesPerPixel, uint16_t numPixels, uint8_t* dataPtr = nullptr):
//      Buffer("PixelBuffer", bytesPerPixel, numPixels, dataPtr) {}
//    PixelBuffer(Buffer& buffer):
//      Buffer("PixelBuffer " + buffer.id(), buffer.fieldSize(), buffer.fieldCount(), buffer.get()) {}

//    /* Color setPixelColor(uint16_t pixel, const Color& color); */
//    /* Color getPixelColor(uint16_t pixel); */

//    PixelBuffer& getScaled(uint16_t numPixels); // scale up or down... later xy etc
//    //Color averageColor(int startPixel = -1, int endPixel = -1) {} // -1 = no subset

//  //XXX how make this work with overshooting? compute delta for average timeBetweenFrames?
//  // GammaCorrection* gamma = nullptr;
//  // void interpolate(PixelBuffer* origin, PixelBuffer* target, float progress, GammaCorrection* gamma);
//};
