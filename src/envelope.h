#pragma once

#include <Arduino.h>
#include <LoggerNode.h>

// actual elope generator for impulse response
class ADSREnvelope {
	enum { ATTACK, DECAY, SUSTAIN, RELEASE, IDLE };
};

enum EnvStage: uint8_t { AT = 0, RE }; //should be dynamic and that...
// template<typename T>
class BlendEnvelope { // simple "envelope" to control how much an incoming value should affect the current
  private:
    String _id;
    bool invert; // no invert = high-is-low
    float divisor[RE+1] = {1.0}; // float attackDivisor = 1.0, releaseDivisor = 1.0; //XXX fix for float
    float _baseline = 1.0; //0.5 // default no effect. the value at 0 attack/release. Later move away from envelope to renderer?
    // float lastProgress = 0; // will need 16bit to work tho
    float value[RE+1] = {0.0}; // float attack = 0.0, release = 0.0; // now as fraction. later fix so can take ms, seconds or whatever
    // float alpha = 1.0; // to scale total effect at once?

    void adjustForDirection() {
      if(invert) {
        for(uint8_t es = AT; es <= RE; es++) value[es] = 1.0 - value[AT];
      }
      // LN.logf(__func__, LoggerNode::DEBUG, "divisors %f %f, values %f %f", divisor[AT], divisor[RE], value[AT], value[RE]);
    }
  public:
//
    //BlendEnvelope(const String& id = "BlendEnvelope", float divisors[RE+1] = (float[]){1.0, 1.0}, bool invert = false, float baseline = 1.0):
    BlendEnvelope(const String& id = "BlendEnvelope", float at = 1.0, float re = 1.0, bool invert = false, float baseline = 1.0):
      _id(id), invert(invert), _baseline(baseline) {
        value[AT] = at; value[RE] = re;
        for(uint8_t es = AT; es <= RE; es++) divisor[es] = divisor[es] > 1.0? divisor[es]: 1.0;
        // divisor[AT] = attackDivisor > 1.0? attackDivisor: 1.0;
        // divisor[RE] = releaseDivisor > 1.0? releaseDivisor: 1.0;
        adjustForDirection();
        LN.logf("New instance", LoggerNode::DEBUG, "BlendEnvelope %s, base %f, div %f/%f",
            _id.c_str(), _baseline, divisor[AT], divisor[RE]);
    }

    void set(uint8_t a, uint8_t r) {
      value[AT] = _baseline - _baseline * ((float)a/(255 * divisor[AT])); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      value[RE] = _baseline - _baseline * ((float)r/(255 * divisor[RE])); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      // value[AT] = (float)a/(255 * divisor[AT]); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      // value[RE] = (float)r/(255 * divisor[RE]); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      if(value[AT] > 1.0 || value[AT] < 0.0)
        LN.logf(__func__, LoggerNode::DEBUG, "wtf attack %f", value[AT]);
      adjustForDirection();
    }
    void set(float a, float r) {
      value[AT] = _baseline - _baseline * (a / divisor[AT]); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      value[RE] = _baseline - _baseline * (r / divisor[RE]);
      adjustForDirection();
    }
    // void set(float values[RE+1]) {
    // void set(float at = 1.0, float re = 1.0) {
    //   value[AT] = at; value[RE] = re;
    //   for(uint8_t es = AT; es <= RE; es++)
    //     value[es] = _baseline - _baseline * (value[es] / divisor[es]); // dont ever quite arrive like this, two runs at max = 75% not 100%...
    // }
    float A(float progress) { return value[AT] * progress; } //lol i was dividing...
    float R(float progress) { return value[RE] * progress; }
    float get(EnvStage es, float progress) { return value[es] * progress; }

    float interpolate(float current, float target, float progress, float min = 0, float max = 1) {
      if(current == target) return current;
      bool higher = target > current;
      float result = current + (target - current) * (higher? A(progress): R(progress));
      // LN.logf(__func__, LoggerNode::DEBUG, "Current %d, result %f, higher %s", value[AT], higher? "true": "false");
      return result > max? max: result < min? min: result;
    }

    // this should be templatized obvs? uint8_t yo
    // float interpolate(uint32_t micros, float current, float target, float min = 0, float max = 1) {
    //   // switch(currentStage) // for impulse one... something like mozzi
    //   if(current == target) return current;
    //   bool higher = target > current;
    //   float progress = micros / keyFrameInterval; // so, can overshoot... but again that best done by calc delta and just applying? depends on buffer size
    //
    //   float result = current + (target - current) * (higher? A(): R());
    //   return result > max? max: result < min? min: result;
    // }
};
