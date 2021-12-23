#pragma once

#include <vector>
#include <limits>
#include <type_traits>
#include <valarray>

#include <field.h>
#include "base.h"
#include "envelope.h"

namespace tol {

// TODO basic subclass for holding a large buffer such as nultiple dmx universes
// in continous memory
// and containing Buffer objects merely acting as views.
// as such should disallow setPtr etc on those...

template<class T, class T2 = T> // For integer types, T2 should generally be the next larger signed one. uint8_t -> int16_t etc.
class iBuffer: public Named, public ChunkedContainer { //rename ValBuffer? lots of overlap w (and inefficiences compared to) valarray.
  using Buffer = iBuffer<T, T2>;                       //but when dont own the data, what can we do...
  public:                                              // but then that's honestly not super common and can be made less so.
                                                       // slices also map nicely onto our chunks, plus easier do stuff like "do op x on all white pixels"
                                                       // but then most interesting stuff is here already prob?
    iBuffer() = default;
    iBuffer(const String& id, uint8_t fieldSize, uint16_t fieldCount,
            T* const dataPtr = nullptr, bool copy = false):
      Named(id, "Buffer"), ChunkedContainer(fieldSize, fieldCount),
      ownsData(!dataPtr || copy),
      data(!ownsData? dataPtr: nullptr) {

        if(!data) {
          data = new T[length()]; // complains about non-const, also 0 might not work for all Ts anyways alloc mem if not passed valid ptr
          memset(data, 0, lengthBytes());
        }
        if(dataPtr && copy) {
          setCopy(dataPtr, lengthBytes());
        }
    }
    virtual ~iBuffer() { // tho im not looking to inhering anymore right?
      setDithering(false);
      if(ownsData) delete[] data;
    }
    iBuffer(const iBuffer& buffer, uint16_t numFields = 0, uint16_t offset = 0, bool copy = false):
      iBuffer(buffer.id() + " clone", buffer.fieldSize(),
              numFields > 0?
                numFields + offset <= buffer.fieldCount()?
                  numFields:
                  numFields - offset:
                buffer.fieldCount() - offset,
              buffer.get(offset), copy) {}

    iBuffer(iBuffer&& buffer) = default; // eh ye nya

    T&        operator[](uint16_t index) const { return *get(index); } // a Field getter would make sense too tho I guess.
    iBuffer&  operator+(const iBuffer& rhs) { return add(rhs); }
    iBuffer&  operator+(T2 delta) { return value(delta); } // Field has some ops we should borrow
    iBuffer&  operator-(const iBuffer& rhs) { return sub(rhs); } // Field has some ops we should borrow
    iBuffer&  operator-(T2 delta) { return value(delta); } // Field has some ops we should borrow
    iBuffer&  operator=(const iBuffer& rhs) { setPtr(rhs); return *this; } // also set fsize/count?
    T*        operator*() const { return get(); } //does raw offset make much sense ever? still, got getField.get() so...
    operator  T*() const { return get(); }

    size_t printTo(Print& p) const override {
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
    Field getField(uint16_t fieldIndex) const { return Field(data, fieldSize(), fieldIndex); } //  // elide copy constructor - is straight

    iBuffer getSubsection(uint16_t startField, uint16_t endField = 0) const;
    using iBuffers = std::array<iBuffer, 2>;
    iBuffers slice(uint16_t at, bool copy = false) {
      return iBuffers{iBuffer(*this, at, 0, copy),
                      iBuffer(*this, 0, at, copy)};
    }
    iBuffer concat(const iBuffer& rhs);
    // iBuffer scaleLength(float fraction) {
    //   // compress or drag out. W some antialiasing etc.
    // }

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

    iBuffer& mix(const iBuffer& other, std::function<T(T, T)> op) {
      for(auto i=0; i < length(); i++)
        data[i] = op(data[i], *other.get(i)); // no way this inlined tho so would want loop inside lambda to actually use...
      return *this;
    }
    iBuffer& mix(const iBuffer& other, std::function<T(T, T, float)> op, float progress) { // could auto this even or?
      for(auto i=0; i < length(); i++)
        data[i] = op(data[i], *other.get(i), progress);
      return *this;
    }
    iBuffer& add(const iBuffer& other) {
      return mix(other, [this](T a, T b) {
        return (T)std::clamp(T2((T2)a + (T2)b), minValue, maxValue); });
    }
    iBuffer& addBounce(iBuffer& other) {
      return mix(other, [this](T a, T b) {
          T2 sum = a + b; // no need for these early casts no?
          if(sum > maxValue) sum = maxValue - sum;
        return (T)sum; });
    }

    // TODO obviously can be done much more efficient easily.
    // but even better, try esp-dsp lib that has optimized asm functions for arrays...
    iBuffer& sub(const iBuffer& other) {
      return mix(other, [this](T a, T b) {
        return (T)std::clamp(T2((T2)a - (T2)b), minValue, maxValue); });
    }
    iBuffer& subBounce(const iBuffer& other) {
      return mix(other, [](T a, T b) {
        return (T)std::abs((T2)a - (T2)b); });
    }

    iBuffer& avg(const iBuffer& other, bool ignoreZero = true) {
      return mix(other, [this, ignoreZero](T a, T b) -> T {
        T2 sum = a + b;
        if(ignoreZero && (!a || !b))
          return (T)sum;
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
        data[p] = std::clamp(T2(data[p] + adjustment), minValue, maxValue);
      return *this;
    }
    T averageInBuffer(uint16_t startField = 0, uint16_t endField = 0); //calculate avg brightness for buffer. Or put higher and averageValue?
    void blendUsingEnvelopeB(const iBuffer& origin, const iBuffer& target, float progress, BlendEnvelope* e = nullptr);
    void interpolate(const iBuffer& origin, const iBuffer& target, float progress);

  protected:
    bool ownsData = false; // q is whether shared_ptr can and if so should handle better.
    T* data = nullptr;     // Would be nice but feels like raw only real option, asking for trouble hooking up to ext libs...
    // std::valarray<T>* valData;
    T* _residuals = nullptr;
    T2 maxValue = std::numeric_limits<T>::max(), // static might have to go as well if might want some different ones for floats
       minValue = std::numeric_limits<T>::min(); //generally 0
    float alpha = 1.0; //or weight, or something...
    float _gain = 1.0;
    T blackPoint = 0.010f * maxValue,
      whitePoint = 0.95f  * maxValue,
      noDitherPoint = 0.020f * maxValue;
    bool _dirty = false;
    // uint16_t updatedBytes = 0; //rename updatedFields?
  private:
};

using Buffer    = iBuffer<uint8_t, int16_t>; // first is actual, second to avoid overflow on ops...
using Buffer16  = iBuffer<uint16_t, int32_t>;
using BufferF   = iBuffer<float>;

}
