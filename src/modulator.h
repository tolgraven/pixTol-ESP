#pragma once

#include <Arduino.h>

/*  Effect class provides interface for taking data and applying function to it - from source to same-size target, only target,
 *  etc etc.
 *  Data generally from current + incoming keyframe PixelBuffer, but could be any value (including that of a Modulator)
 *
 *  Can be applied at any stage of pipeline, eg
 *  - INPUT: scale to different size, sep fn chs from pixel data, adapt geometry...
 *  - BUFFER: gain, attack/release, rotate, hue, etc
 *  - OUTPUT: dimmer, strobe, color correction, flip, map geometry
 *
 *  Strobe toggles shutter on and off, so Strip exposes that and modulator does callback etc
 *  no extra render callbacks, just flip it and it happens next possible frame
 *
 *  Dimmer simply brightness. But dimmer att/rls (baked into same modulator?) complicates
 *
 * Rotate and hue etc seem tricky from outside, but expose strip methods and eh
 *  */
// template<typename T> using ToCallback     = std::function<bool(T* buffer, uint16_t length, float value, float amount)>;
// template<typename T> using FromToCallback = std::function<bool(T* buffer, uint16_t length, const T* target, float value, float amount)>;
// template<typename T> using ResizeCallback = std::function<bool(T* buffer, uint16_t length, const T* target, uint16_t targetLength, float value, float amount)>;

// float value = 1.0f, float amount = 1.0f    uint16_t length = 1,
typedef std::function<bool
(uint8_t* buffer, uint16_t length, float value, float amount)> DataToCallback;
typedef std::function<bool
(uint8_t* buffer, uint16_t length, const uint8_t* target, float value, float amount)> DataFromToCallback;
typedef std::function<bool
(uint8_t* buffer, uint16_t length, const uint8_t* target, uint16_t targetLength, float value, float amount)> DataResizeCallback;

typedef std::function<bool
(float* buffer, uint16_t length, float value, float amount)> FloatToCallback;
typedef std::function<bool
(float* buffer, uint16_t length, const float* target, float value, float amount)> FloatFromToCallback;
typedef std::function<bool
(float* buffer, uint16_t length, const float* target, uint16_t targetLength, float value, float amount)> FloatResizeCallback;


// template<typename T>
class Modulator {
public:
  Modulator(uint8_t id, const String& name, float initialValue, uint16_t length):
   id(id), name(name), value(initialValue), currentLength(length) {}

  virtual bool apply(float amount = 1.0f) = 0; // return false if eg data pointer invalid, outside responsibility to make sure modulator gets valid data (think NeoPixelBus buffer pointer which can change)
  virtual Modulator& setCurrent(uint8_t* currentptr) = 0;
  virtual Modulator& setCurrent(float* currentptr)  = 0;
  virtual Modulator& setTarget(uint8_t* targetptr)  = 0; 
  virtual Modulator& setTarget(float* targetptr)  = 0;  
  virtual Modulator& setTargetLength(uint16_t length) = 0;
  Modulator& setLength(uint16_t length) { currentLength = length; } //sets len current/buffer we are modding
  uint16_t getLength() { return currentLength; }

  float value = 0.0f; // generally 0.0-1.0, neg values should also be valid, some would accept higher values (eg gain mod)// might want more than simple single value tho?
protected:
  uint8_t* currentBuffer = nullptr;
  uint16_t currentLength = 0;
  String name;
  uint8_t id;

private:
  static std::vector<Modulator*> mods; //gather all here so can keep track of shit
};

class ModulatorFromTo: public Modulator {
public:
  ModulatorFromTo(uint8_t id, const String& name, float initialValue, uint16_t length, DataFromToCallback callback):
   Modulator(id, name, initialValue, length), effect(callback) {

   }
  virtual bool apply(float amount = 1.0f) {
   return effect(currentBuffer, currentLength, targetBuffer, value, amount);
  }
  virtual Modulator& setCurrent(uint8_t* currentptr) {
   currentBuffer = currentptr;
   return *this;
  }
  virtual Modulator& setTarget(uint8_t* targetptr) {
   targetBuffer = targetptr;
   return *this;
  }
  virtual Modulator& setTargetLength(uint16_t length) {
   targetLength = length;
   return *this;
  }

private:
  const uint8_t* targetBuffer = nullptr;
  uint16_t targetLength = 0;

  DataToCallback effect;
  // FromToCallback<uint8_t> effect;
};

class ModulatorTo: public Modulator {
public:
  ModulatorTo(const String& name, float initialValue, DataToCallback callback);
  virtual bool apply(float amount = 1.0f) {
   return effect(currentBuffer, currentLength, value, amount);
  }
  virtual Modulator& setCurrent(uint8_t* currentptr) { currentBuffer = currentptr; }

private:

  DataToCallback effect;
};

class ModulatorResize: public Modulator {
public:
  ModulatorResize(const String& name, float initialValue, DataResizeCallback callback);
  virtual bool apply(float amount = 1.0f) {
   return effect(currentBuffer, currentLength, targetBuffer, targetLength, value, amount);
  }
  virtual Modulator& setCurrent(uint8_t* currentptr) { currentBuffer = currentptr; }
  virtual Modulator& setTarget(uint8_t* targetptr)  { targetBuffer = targetptr; }   
  virtual Modulator& setTargetLength(uint16_t length) { targetLength = length; }

private:
  const uint8_t* targetBuffer = nullptr;
  uint16_t targetLength = 0;

  DataResizeCallback effect;
};

class ModulatorFloat: public Modulator {
public:
  ModulatorFloat(const String& name, float initialValue, DataResizeCallback callback);
  virtual bool apply(float amount = 1.0f) {
   return effect(currentFloat, currentLength, targetFloat, targetLength, value, amount);
  }
  virtual Modulator& setCurrent(float* currentptr) { currentFloat = currentptr; }
  virtual Modulator& setTarget(float* targetptr) { targetFloat = targetptr; }
  virtual Modulator& setTargetLength(uint16_t length) { targetLength = length; }

private:
  float* currentFloat = nullptr;
  const float* targetFloat = nullptr;
  uint16_t targetLength = 0;

  FloatFromToCallback effect;
};
